#include "InventoryComponent.h"
#include "ItemDefinition.h"
#include "../Save/PlayerProgressSaveGame.h"
#include "../VitalsReflection.h"
#include "TimerManager.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	DefaultTabCapacities.Add(EInventoryTab::Weapons, 12);
	DefaultTabCapacities.Add(EInventoryTab::Clothes, 16);
	DefaultTabCapacities.Add(EInventoryTab::Miscellaneous, 20);
	DefaultTabCapacities.Add(EInventoryTab::MissionItems, 8);
	DefaultTabCapacities.Add(EInventoryTab::Consumables, 12);

	Tabs.SetNum(InventoryTabs::Num);
	for (int32 i = 0; i < InventoryTabs::Num; ++i)
	{
		Tabs[i].Tab = static_cast<EInventoryTab>(i);
	}
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	for (FInventoryTabData& TabData : Tabs)
	{
		const int32* Capacity = DefaultTabCapacities.Find(TabData.Tab);
		TabData.MaxSlots = Capacity ? *Capacity : 8;
		TabData.Slots.SetNum(TabData.MaxSlots);
	}

	if (LoadProgress())
	{
		// Vital components (BPC_*) initialize their starting values in their own
		// BeginPlay; restore the saved values on the next tick, after all of them ran.
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UPlayerProgressSaveGame::LoadVitals(GetOwner());
		}));
	}
	else
	{
		GrantStartingItems();
	}
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Persist on level transition / quit. Explicit saves can also be triggered
	// from Blueprint via SaveProgress.
	if (EndPlayReason == EEndPlayReason::EndPlayInEditor ||
		EndPlayReason == EEndPlayReason::Quit ||
		EndPlayReason == EEndPlayReason::LevelTransition)
	{
		SaveProgress();
	}
	Super::EndPlay(EndPlayReason);
}

void UInventoryComponent::GrantStartingItems()
{
	for (const TPair<TObjectPtr<UItemDefinition>, int32>& Pair : StartingItems)
	{
		if (Pair.Key && Pair.Value > 0)
		{
			AddItem(Pair.Key, Pair.Value);
		}
	}
}

FInventoryTabData& UInventoryComponent::GetTabData(const EInventoryTab Tab)
{
	return Tabs[static_cast<int32>(Tab)];
}

const FInventoryTabData& UInventoryComponent::GetTabData(const EInventoryTab Tab) const
{
	return Tabs[static_cast<int32>(Tab)];
}

bool UInventoryComponent::IsValidSlotIndex(const EInventoryTab Tab, const int32 SlotIndex) const
{
	return GetTabData(Tab).Slots.IsValidIndex(SlotIndex);
}

int32 UInventoryComponent::GetTabCapacity(const EInventoryTab Tab) const
{
	return GetTabData(Tab).MaxSlots;
}

int32 UInventoryComponent::GetUsedSlotCount(const EInventoryTab Tab) const
{
	int32 Used = 0;
	for (const FInventorySlot& Slot : GetTabData(Tab).Slots)
	{
		if (!Slot.IsEmpty())
		{
			++Used;
		}
	}
	return Used;
}

const TArray<FInventorySlot>& UInventoryComponent::GetItemsInTab(const EInventoryTab Tab) const
{
	return GetTabData(Tab).Slots;
}

FInventorySlot UInventoryComponent::GetSlot(const EInventoryTab Tab, const int32 SlotIndex) const
{
	return IsValidSlotIndex(Tab, SlotIndex) ? GetTabData(Tab).Slots[SlotIndex] : FInventorySlot();
}

int32 UInventoryComponent::CountItem(const UItemDefinition* Item) const
{
	if (!Item)
	{
		return 0;
	}
	int32 Total = 0;
	for (const FInventorySlot& Slot : GetTabData(Item->Tab).Slots)
	{
		if (Slot.Item == Item)
		{
			Total += Slot.Quantity;
		}
	}
	return Total;
}

bool UInventoryComponent::CanAddItem(const UItemDefinition* Item, const int32 Quantity) const
{
	if (!Item || Quantity <= 0)
	{
		return false;
	}

	int32 Space = 0;
	for (const FInventorySlot& Slot : GetTabData(Item->Tab).Slots)
	{
		if (Slot.IsEmpty())
		{
			Space += Item->MaxStack;
		}
		else if (Slot.Item == Item)
		{
			Space += Item->MaxStack - Slot.Quantity;
		}
		if (Space >= Quantity)
		{
			return true;
		}
	}
	return false;
}

int32 UInventoryComponent::AddItem(UItemDefinition* Item, const int32 Quantity)
{
	if (!Item || Quantity <= 0)
	{
		return Quantity;
	}

	FInventoryTabData& TabData = GetTabData(Item->Tab);
	int32 Remaining = Quantity;

	// Fill existing stacks first.
	if (Item->MaxStack > 1)
	{
		for (int32 i = 0; i < TabData.Slots.Num() && Remaining > 0; ++i)
		{
			FInventorySlot& Slot = TabData.Slots[i];
			if (Slot.Item == Item && Slot.Quantity < Item->MaxStack)
			{
				const int32 Moved = FMath::Min(Remaining, Item->MaxStack - Slot.Quantity);
				Slot.Quantity += Moved;
				Remaining -= Moved;
				OnInventoryChanged.Broadcast(Item->Tab, i);
			}
		}
	}

	// Then occupy empty slots.
	for (int32 i = 0; i < TabData.Slots.Num() && Remaining > 0; ++i)
	{
		FInventorySlot& Slot = TabData.Slots[i];
		if (Slot.IsEmpty())
		{
			const int32 Moved = FMath::Min(Remaining, Item->MaxStack);
			Slot.Item = Item;
			Slot.Quantity = Moved;
			Slot.InstanceId = FGuid::NewGuid();
			Remaining -= Moved;
			OnInventoryChanged.Broadcast(Item->Tab, i);
		}
	}

	return Remaining;
}

int32 UInventoryComponent::RemoveItem(UItemDefinition* Item, const int32 Quantity)
{
	if (!Item || Quantity <= 0)
	{
		return 0;
	}

	FInventoryTabData& TabData = GetTabData(Item->Tab);
	int32 Removed = 0;

	for (int32 i = 0; i < TabData.Slots.Num() && Removed < Quantity; ++i)
	{
		FInventorySlot& Slot = TabData.Slots[i];
		if (Slot.Item == Item)
		{
			const int32 Take = FMath::Min(Quantity - Removed, Slot.Quantity);
			Slot.Quantity -= Take;
			Removed += Take;
			if (Slot.Quantity <= 0)
			{
				Slot.Clear();
			}
			OnInventoryChanged.Broadcast(Item->Tab, i);
		}
	}
	return Removed;
}

int32 UInventoryComponent::RemoveFromSlot(const EInventoryTab Tab, const int32 SlotIndex, const int32 Quantity)
{
	if (!IsValidSlotIndex(Tab, SlotIndex) || Quantity <= 0)
	{
		return 0;
	}

	FInventorySlot& Slot = GetTabData(Tab).Slots[SlotIndex];
	if (Slot.IsEmpty())
	{
		return 0;
	}

	const int32 Take = FMath::Min(Quantity, Slot.Quantity);
	Slot.Quantity -= Take;
	if (Slot.Quantity <= 0)
	{
		Slot.Clear();
	}
	OnInventoryChanged.Broadcast(Tab, SlotIndex);
	return Take;
}

bool UInventoryComponent::MoveSlot(const EInventoryTab FromTab, const int32 FromIndex, const EInventoryTab ToTab, const int32 ToIndex)
{
	// Items are bound to their tab; only reordering within a tab is allowed.
	if (FromTab != ToTab || FromIndex == ToIndex ||
		!IsValidSlotIndex(FromTab, FromIndex) || !IsValidSlotIndex(ToTab, ToIndex))
	{
		return false;
	}

	FInventoryTabData& TabData = GetTabData(FromTab);
	FInventorySlot& From = TabData.Slots[FromIndex];
	FInventorySlot& To = TabData.Slots[ToIndex];

	if (From.IsEmpty())
	{
		return false;
	}

	if (!To.IsEmpty() && To.Item == From.Item && To.Item->MaxStack > 1)
	{
		// Merge stacks.
		const int32 Moved = FMath::Min(From.Quantity, To.Item->MaxStack - To.Quantity);
		if (Moved <= 0)
		{
			return false;
		}
		To.Quantity += Moved;
		From.Quantity -= Moved;
		if (From.Quantity <= 0)
		{
			From.Clear();
		}
	}
	else
	{
		Swap(From, To);
	}

	OnInventoryChanged.Broadcast(FromTab, FromIndex);
	OnInventoryChanged.Broadcast(ToTab, ToIndex);
	return true;
}

bool UInventoryComponent::SplitStack(const EInventoryTab Tab, const int32 FromIndex, const int32 ToIndex, const int32 Quantity)
{
	if (!IsValidSlotIndex(Tab, FromIndex) || !IsValidSlotIndex(Tab, ToIndex) || FromIndex == ToIndex || Quantity <= 0)
	{
		return false;
	}

	FInventoryTabData& TabData = GetTabData(Tab);
	FInventorySlot& From = TabData.Slots[FromIndex];
	FInventorySlot& To = TabData.Slots[ToIndex];

	if (From.IsEmpty() || !To.IsEmpty() || Quantity >= From.Quantity)
	{
		return false;
	}

	To.Item = From.Item;
	To.Quantity = Quantity;
	To.InstanceId = FGuid::NewGuid();
	From.Quantity -= Quantity;

	OnInventoryChanged.Broadcast(Tab, FromIndex);
	OnInventoryChanged.Broadcast(Tab, ToIndex);
	return true;
}

bool UInventoryComponent::SetTabCapacity(const EInventoryTab Tab, const int32 NewCapacity)
{
	if (NewCapacity < 0)
	{
		return false;
	}

	FInventoryTabData& TabData = GetTabData(Tab);
	if (NewCapacity == TabData.MaxSlots)
	{
		return true;
	}

	if (NewCapacity < TabData.MaxSlots)
	{
		// Refuse to cut off occupied slots.
		for (int32 i = NewCapacity; i < TabData.Slots.Num(); ++i)
		{
			if (!TabData.Slots[i].IsEmpty())
			{
				return false;
			}
		}
	}

	TabData.MaxSlots = NewCapacity;
	TabData.Slots.SetNum(NewCapacity);
	OnInventoryChanged.Broadcast(Tab, INDEX_NONE);
	return true;
}

bool UInventoryComponent::UseConsumable(const int32 SlotIndex)
{
	if (!IsValidSlotIndex(EInventoryTab::Consumables, SlotIndex))
	{
		return false;
	}

	const FInventorySlot& Slot = GetTabData(EInventoryTab::Consumables).Slots[SlotIndex];
	const UConsumableItemDefinition* Consumable = Cast<UConsumableItemDefinition>(Slot.Item);
	if (!Consumable)
	{
		return false;
	}

	ApplyConsumableEffect(Consumable);
	RemoveFromSlot(EInventoryTab::Consumables, SlotIndex, 1);
	OnConsumableUsed(Consumable);
	return true;
}

void UInventoryComponent::ApplyConsumableEffect(const UConsumableItemDefinition* Consumable)
{
	AActor* Owner = GetOwner();

	switch (Consumable->Effect)
	{
	case EConsumableEffect::RestoreHealth:
		if (UActorComponent* Health = FVitalsReflection::FindComponentByClassNamePrefix(Owner, TEXT("BPC_health")))
		{
			FVitalsReflection::CallFunctionWithDouble(Health, TEXT("IncreaseHealth"), Consumable->Magnitude);
		}
		break;

	case EConsumableEffect::RestoreStamina:
		if (UActorComponent* Stamina = FVitalsReflection::FindComponentByClassNamePrefix(Owner, TEXT("BPC_stamina")))
		{
			// BPC_stamina exposes a StaminaChange custom event driven by ModifyAmount.
			if (!FVitalsReflection::CallFunctionWithDouble(Stamina, TEXT("StaminaChange"), Consumable->Magnitude))
			{
				FVitalsReflection::CallFunctionWithDouble(Stamina, TEXT("IncreaseMaxStamina"), Consumable->Magnitude);
			}
		}
		break;

	case EConsumableEffect::GrantXP:
		if (UActorComponent* Xp = FVitalsReflection::FindComponentByClassNamePrefix(Owner, TEXT("BPC_LevelAndXP")))
		{
			FVitalsReflection::CallFunctionWithDouble(Xp, TEXT("IncreaseXP"), Consumable->Magnitude);
		}
		break;

	case EConsumableEffect::ExpandTab:
		SetTabCapacity(Consumable->TargetTab, GetTabCapacity(Consumable->TargetTab) + FMath::RoundToInt32(Consumable->Magnitude));
		break;

	default:
		break;
	}
}

void UInventoryComponent::SaveProgress()
{
	UPlayerProgressSaveGame::SaveFor(GetOwner());
}

bool UInventoryComponent::LoadProgress()
{
	return UPlayerProgressSaveGame::LoadInventory(this);
}
