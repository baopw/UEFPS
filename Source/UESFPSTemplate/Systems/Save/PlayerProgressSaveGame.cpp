#include "PlayerProgressSaveGame.h"

#include "Engine/AssetManager.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "../Inventory/InventoryComponent.h"
#include "../Inventory/ItemDefinition.h"
#include "../VitalsReflection.h"
#include "../Weapons/WeaponLoadoutComponent.h"

const FString UPlayerProgressSaveGame::SaveSlotName(TEXT("PlayerProgress"));

UPlayerProgressSaveGame* UPlayerProgressSaveGame::LoadFromSlot()
{
	return Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
}

UItemDefinition* UPlayerProgressSaveGame::ResolveItem(const FPrimaryAssetId& ItemId)
{
	if (!ItemId.IsValid())
	{
		return nullptr;
	}

	const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(ItemId);
	if (!Path.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerProgressSaveGame: unknown item id '%s' (asset removed?)"), *ItemId.ToString());
		return nullptr;
	}
	// Synchronous load is acceptable here: restoration happens at BeginPlay, not during gameplay.
	return Cast<UItemDefinition>(Path.TryLoad());
}

bool UPlayerProgressSaveGame::SaveFor(AActor* PlayerActor)
{
	if (!PlayerActor)
	{
		return false;
	}

	UInventoryComponent* Inventory = PlayerActor->FindComponentByClass<UInventoryComponent>();
	UWeaponLoadoutComponent* Loadout = PlayerActor->FindComponentByClass<UWeaponLoadoutComponent>();
	if (!Inventory && !Loadout)
	{
		return false;
	}

	UPlayerProgressSaveGame* Save = Cast<UPlayerProgressSaveGame>(
		UGameplayStatics::CreateSaveGameObject(StaticClass()));

	if (Inventory)
	{
		for (int32 i = 0; i < InventoryTabs::Num; ++i)
		{
			const EInventoryTab Tab = static_cast<EInventoryTab>(i);
			FSavedInventoryTab& SavedTab = Save->InventoryTabs.AddDefaulted_GetRef();
			SavedTab.Tab = Tab;
			SavedTab.MaxSlots = Inventory->GetTabCapacity(Tab);

			const TArray<FInventorySlot>& Slots = Inventory->GetItemsInTab(Tab);
			for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
			{
				const FInventorySlot& Slot = Slots[SlotIndex];
				if (Slot.IsEmpty())
				{
					continue;
				}
				FSavedInventorySlot& SavedSlot = SavedTab.Slots.AddDefaulted_GetRef();
				SavedSlot.ItemId = Slot.Item->GetPrimaryAssetId();
				SavedSlot.SlotIndex = SlotIndex;
				SavedSlot.Quantity = Slot.Quantity;
				SavedSlot.InstanceId = Slot.InstanceId;
			}
		}
	}

	if (Loadout)
	{
		for (int32 i = 1; i < static_cast<int32>(EWeaponSlot::Count); ++i)
		{
			const EWeaponSlot Slot = static_cast<EWeaponSlot>(i);
			if (const UWeaponItemDefinition* Weapon = Loadout->GetSlotWeapon(Slot))
			{
				FSavedWeaponSlot& SavedSlot = Save->Loadout.AddDefaulted_GetRef();
				SavedSlot.Slot = Slot;
				SavedSlot.WeaponItemId = Weapon->GetPrimaryAssetId();
			}
		}
	}

	// Vitals live in Blueprint components (BPC_*), read via reflection.
	if (const UActorComponent* Health = FVitalsReflection::FindComponentByClassNamePrefix(PlayerActor, TEXT("BPC_health")))
	{
		Save->bHasVitals |= FVitalsReflection::GetNumericProperty(Health, TEXT("CurrentHealth"), Save->CurrentHealth);
	}
	if (const UActorComponent* Stamina = FVitalsReflection::FindComponentByClassNamePrefix(PlayerActor, TEXT("BPC_stamina")))
	{
		Save->bHasVitals |= FVitalsReflection::GetNumericProperty(Stamina, TEXT("CurrentStamina"), Save->CurrentStamina);
	}
	if (const UActorComponent* Xp = FVitalsReflection::FindComponentByClassNamePrefix(PlayerActor, TEXT("BPC_LevelAndXP")))
	{
		Save->bHasVitals |= FVitalsReflection::GetNumericProperty(Xp, TEXT("CurrentXP"), Save->CurrentXP);
		if (!FVitalsReflection::GetNumericProperty(Xp, TEXT("LevelNumber"), Save->Level))
		{
			FVitalsReflection::GetNumericProperty(Xp, TEXT("Level"), Save->Level);
		}
	}

	return UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
}

bool UPlayerProgressSaveGame::LoadInventory(UInventoryComponent* Inventory)
{
	UPlayerProgressSaveGame* Save = LoadFromSlot();
	if (!Save || !Inventory || Save->InventoryTabs.Num() == 0)
	{
		return false;
	}

	for (const FSavedInventoryTab& SavedTab : Save->InventoryTabs)
	{
		Inventory->SetTabCapacity(SavedTab.Tab, SavedTab.MaxSlots);

		FInventoryTabData& TabData = Inventory->GetTabData(SavedTab.Tab);
		for (const FSavedInventorySlot& SavedSlot : SavedTab.Slots)
		{
			if (!TabData.Slots.IsValidIndex(SavedSlot.SlotIndex))
			{
				continue;
			}
			UItemDefinition* Item = ResolveItem(SavedSlot.ItemId);
			if (!Item)
			{
				continue;
			}
			FInventorySlot& Slot = TabData.Slots[SavedSlot.SlotIndex];
			Slot.Item = Item;
			Slot.Quantity = FMath::Clamp(SavedSlot.Quantity, 1, Item->MaxStack);
			Slot.InstanceId = SavedSlot.InstanceId;
		}
		Inventory->OnInventoryChanged.Broadcast(SavedTab.Tab, INDEX_NONE);
	}
	return true;
}

bool UPlayerProgressSaveGame::LoadLoadout(UWeaponLoadoutComponent* Loadout)
{
	UPlayerProgressSaveGame* Save = LoadFromSlot();
	if (!Save || !Loadout)
	{
		return false;
	}

	bool bAnyApplied = false;
	for (const FSavedWeaponSlot& SavedSlot : Save->Loadout)
	{
		if (UWeaponItemDefinition* Weapon = Cast<UWeaponItemDefinition>(ResolveItem(SavedSlot.WeaponItemId)))
		{
			bAnyApplied |= Loadout->AssignWeapon(SavedSlot.Slot, Weapon);
		}
	}
	return bAnyApplied;
}

bool UPlayerProgressSaveGame::LoadVitals(AActor* PlayerActor)
{
	UPlayerProgressSaveGame* Save = LoadFromSlot();
	if (!Save || !PlayerActor || !Save->bHasVitals)
	{
		return false;
	}

	if (UActorComponent* Health = FVitalsReflection::FindComponentByClassNamePrefix(PlayerActor, TEXT("BPC_health")))
	{
		FVitalsReflection::SetNumericProperty(Health, TEXT("CurrentHealth"), Save->CurrentHealth);
		FVitalsReflection::CallFunctionWithDouble(Health, TEXT("CalculateHealthPercentage"), 0.0);
	}
	if (UActorComponent* Stamina = FVitalsReflection::FindComponentByClassNamePrefix(PlayerActor, TEXT("BPC_stamina")))
	{
		FVitalsReflection::SetNumericProperty(Stamina, TEXT("CurrentStamina"), Save->CurrentStamina);
	}
	if (UActorComponent* Xp = FVitalsReflection::FindComponentByClassNamePrefix(PlayerActor, TEXT("BPC_LevelAndXP")))
	{
		FVitalsReflection::SetNumericProperty(Xp, TEXT("CurrentXP"), Save->CurrentXP);
		if (!FVitalsReflection::SetNumericProperty(Xp, TEXT("LevelNumber"), Save->Level))
		{
			FVitalsReflection::SetNumericProperty(Xp, TEXT("Level"), Save->Level);
		}
	}
	return true;
}
