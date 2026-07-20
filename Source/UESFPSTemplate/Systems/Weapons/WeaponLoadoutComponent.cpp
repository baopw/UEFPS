#include "WeaponLoadoutComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "../Inventory/ItemDefinition.h"
#include "../Save/PlayerProgressSaveGame.h"
#include "FPSWeaponBase.h"
#include "TimerManager.h"

UWeaponLoadoutComponent::UWeaponLoadoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	WeaponSlotActions[0] = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/FirstPerson/Input/Actions/IA_WeaponSlot1.IA_WeaponSlot1")));
	WeaponSlotActions[1] = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/FirstPerson/Input/Actions/IA_WeaponSlot2.IA_WeaponSlot2")));
	WeaponSlotActions[2] = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/FirstPerson/Input/Actions/IA_WeaponSlot3.IA_WeaponSlot3")));
	WeaponSlotActions[3] = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/FirstPerson/Input/Actions/IA_WeaponSlot4.IA_WeaponSlot4")));
	HolsterAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/FirstPerson/Input/Actions/IA_HolsterWeapon.IA_HolsterWeapon")));
	FireAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/FirstPerson/Input/Actions/IA_Fire.IA_Fire")));
	ReloadAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/FirstPerson/Input/Actions/IA_Reload.IA_Reload")));
}

ACharacter* UWeaponLoadoutComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

void UWeaponLoadoutComponent::BeginPlay()
{
	Super::BeginPlay();

	// Restore saved loadout, otherwise use the defaults configured on the component.
	if (!UPlayerProgressSaveGame::LoadLoadout(this))
	{
		for (const TPair<EWeaponSlot, TSoftObjectPtr<UWeaponItemDefinition>>& Pair : DefaultLoadout)
		{
			// Startup-time sync resolve is fine; gameplay-time loads stay async.
			if (const UWeaponItemDefinition* Weapon = Pair.Value.LoadSynchronous())
			{
				AssignWeapon(Pair.Key, const_cast<UWeaponItemDefinition*>(Weapon));
			}
		}
	}

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveRestartedDelegate.AddDynamic(this, &UWeaponLoadoutComponent::OnPawnRestarted);
		if (Pawn->InputComponent)
		{
			SetupInput();
		}
	}

	// The character Blueprint spawns and attaches its legacy rifle in BeginPlay,
	// which runs after component BeginPlay - clean it up on the next tick.
	GetWorld()->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &UWeaponLoadoutComponent::RemoveLegacyRifle));
}

void UWeaponLoadoutComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (TSharedPtr<FStreamableHandle>& Handle : PreloadHandles)
	{
		if (Handle.IsValid())
		{
			Handle->CancelHandle();
			Handle.Reset();
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UWeaponLoadoutComponent::OnPawnRestarted(APawn* Pawn)
{
	SetupInput();
}

void UWeaponLoadoutComponent::RemoveLegacyRifle()
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<AActor*> Attached;
	Owner->GetAttachedActors(Attached);
	for (AActor* Actor : Attached)
	{
		if (Actor && Actor != ActiveWeaponActor && Actor->GetClass()->GetName().StartsWith(TEXT("BP_Rifle")))
		{
			Actor->Destroy();
		}
	}
}

void UWeaponLoadoutComponent::SetupInput()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	UEnhancedInputComponent* Input = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
	if (!Input)
	{
		return;
	}

	BindResolvedAction(Input, WeaponSlotActions[0], EKeys::One, &UWeaponLoadoutComponent::OnSlot1);
	BindResolvedAction(Input, WeaponSlotActions[1], EKeys::Two, &UWeaponLoadoutComponent::OnSlot2);
	BindResolvedAction(Input, WeaponSlotActions[2], EKeys::Three, &UWeaponLoadoutComponent::OnSlot3);
	BindResolvedAction(Input, WeaponSlotActions[3], EKeys::Four, &UWeaponLoadoutComponent::OnSlot4);
	BindResolvedAction(Input, HolsterAction, EKeys::H, &UWeaponLoadoutComponent::OnHolster);
	BindResolvedAction(Input, FireAction, EKeys::LeftMouseButton,
		&UWeaponLoadoutComponent::OnFirePressed, &UWeaponLoadoutComponent::OnFireReleased);
	BindResolvedAction(Input, ReloadAction, EKeys::R, &UWeaponLoadoutComponent::OnReload);

	if (RuntimeMappingContext)
	{
		if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				if (!Subsystem->HasMappingContext(RuntimeMappingContext))
				{
					Subsystem->AddMappingContext(RuntimeMappingContext, 2);
				}
			}
		}
	}
}

void UWeaponLoadoutComponent::BindResolvedAction(UEnhancedInputComponent* Input, TSoftObjectPtr<UInputAction>& SoftAction,
	const FKey& FallbackKey, void (UWeaponLoadoutComponent::*PressedHandler)(const FInputActionValue&),
	void (UWeaponLoadoutComponent::*ReleasedHandler)(const FInputActionValue&))
{
	UInputAction* Action = SoftAction.LoadSynchronous();
	if (!Action)
	{
		// No asset available: build a runtime action with the documented default key.
		UInputAction* RuntimeAction = NewObject<UInputAction>(this);
		RuntimeAction->ValueType = EInputActionValueType::Boolean;
		if (!RuntimeMappingContext)
		{
			RuntimeMappingContext = NewObject<UInputMappingContext>(this);
		}
		RuntimeMappingContext->MapKey(RuntimeAction, FallbackKey);
		Action = RuntimeAction;
	}

	Input->BindAction(Action, ETriggerEvent::Started, this, PressedHandler);
	if (ReleasedHandler)
	{
		Input->BindAction(Action, ETriggerEvent::Completed, this, ReleasedHandler);
	}
}

void UWeaponLoadoutComponent::OnSlot1(const FInputActionValue&) { SwitchToSlot(EWeaponSlot::Unarmed); }
void UWeaponLoadoutComponent::OnSlot2(const FInputActionValue&) { SwitchToSlot(EWeaponSlot::Melee); }
void UWeaponLoadoutComponent::OnSlot3(const FInputActionValue&) { SwitchToSlot(EWeaponSlot::RangedPrimary); }
void UWeaponLoadoutComponent::OnSlot4(const FInputActionValue&) { SwitchToSlot(EWeaponSlot::RangedSecondary); }
void UWeaponLoadoutComponent::OnHolster(const FInputActionValue&) { HolsterWeapon(); }

void UWeaponLoadoutComponent::OnFirePressed(const FInputActionValue&)
{
	if (ActiveWeaponActor)
	{
		ActiveWeaponActor->StartFire();
	}
}

void UWeaponLoadoutComponent::OnFireReleased(const FInputActionValue&)
{
	if (ActiveWeaponActor)
	{
		ActiveWeaponActor->StopFire();
	}
}

void UWeaponLoadoutComponent::OnReload(const FInputActionValue&)
{
	if (ActiveWeaponActor)
	{
		ActiveWeaponActor->Reload();
	}
}

UWeaponItemDefinition* UWeaponLoadoutComponent::GetSlotWeapon(const EWeaponSlot Slot) const
{
	const int32 Index = static_cast<int32>(Slot);
	return (Index > 0 && Index < NumSlots) ? SlotWeapons[Index].Get() : nullptr;
}

bool UWeaponLoadoutComponent::IsSlotOccupied(const EWeaponSlot Slot) const
{
	return Slot == EWeaponSlot::Unarmed || GetSlotWeapon(Slot) != nullptr;
}

bool UWeaponLoadoutComponent::SwitchToSlot(const EWeaponSlot Slot)
{
	if (Slot >= EWeaponSlot::Count)
	{
		return false;
	}

	// Pressing the active slot's key again holsters (Cyberpunk-style toggle).
	if (Slot == ActiveSlot)
	{
		if (Slot != EWeaponSlot::Unarmed)
		{
			return SwitchToSlot(EWeaponSlot::Unarmed);
		}
		return true;
	}

	if (Slot == EWeaponSlot::Unarmed)
	{
		PendingSwitchSlot.Reset();
		DestroyActiveWeapon();
		ActiveSlot = EWeaponSlot::Unarmed;
		OnActiveWeaponChanged.Broadcast(ActiveSlot, nullptr);
		return true;
	}

	const UWeaponItemDefinition* Weapon = GetSlotWeapon(Slot);
	if (!Weapon)
	{
		OnWeaponSwitchDenied.Broadcast(Slot);
		return false;
	}

	if (!Weapon->WeaponActorClass.Get())
	{
		// Class still streaming: remember the request, it completes in the preload callback.
		PendingSwitchSlot = Slot;
		PreloadSlot(Slot);
		return true;
	}

	SpawnAndEquip(Slot);
	return true;
}

void UWeaponLoadoutComponent::HolsterWeapon()
{
	SwitchToSlot(EWeaponSlot::Unarmed);
}

EWeaponSlot UWeaponLoadoutComponent::FindSlotForWeapon(const UWeaponItemDefinition* Weapon) const
{
	if (!Weapon)
	{
		return EWeaponSlot::Unarmed;
	}
	if (Weapon->WeaponKind == EWeaponKind::Melee)
	{
		return EWeaponSlot::Melee;
	}
	if (!GetSlotWeapon(EWeaponSlot::RangedPrimary))
	{
		return EWeaponSlot::RangedPrimary;
	}
	if (!GetSlotWeapon(EWeaponSlot::RangedSecondary))
	{
		return EWeaponSlot::RangedSecondary;
	}
	return EWeaponSlot::RangedPrimary;
}

bool UWeaponLoadoutComponent::AssignWeapon(const EWeaponSlot Slot, UWeaponItemDefinition* Weapon)
{
	const int32 Index = static_cast<int32>(Slot);
	if (Index <= 0 || Index >= NumSlots || !Weapon)
	{
		return false;
	}

	const bool bSlotWantsMelee = Slot == EWeaponSlot::Melee;
	if (bSlotWantsMelee != (Weapon->WeaponKind == EWeaponKind::Melee))
	{
		return false;
	}

	SlotWeapons[Index] = Weapon;
	PreloadSlot(Slot);
	OnLoadoutSlotChanged.Broadcast(Slot);

	if (ActiveSlot == Slot)
	{
		DestroyActiveWeapon();
		SwitchToSlot(Slot);
	}
	return true;
}

bool UWeaponLoadoutComponent::EquipWeaponFromInventory(const FInventorySlot& InventorySlot, const EWeaponSlot PreferredSlot)
{
	UWeaponItemDefinition* Weapon = Cast<UWeaponItemDefinition>(InventorySlot.Item);
	if (!Weapon)
	{
		return false;
	}

	EWeaponSlot TargetSlot = PreferredSlot;
	const bool bPreferredValid =
		(PreferredSlot == EWeaponSlot::Melee && Weapon->WeaponKind == EWeaponKind::Melee) ||
		((PreferredSlot == EWeaponSlot::RangedPrimary || PreferredSlot == EWeaponSlot::RangedSecondary) &&
			Weapon->WeaponKind == EWeaponKind::Ranged);
	if (!bPreferredValid)
	{
		TargetSlot = FindSlotForWeapon(Weapon);
	}

	return AssignWeapon(TargetSlot, Weapon);
}

void UWeaponLoadoutComponent::ClearSlot(const EWeaponSlot Slot)
{
	const int32 Index = static_cast<int32>(Slot);
	if (Index <= 0 || Index >= NumSlots)
	{
		return;
	}

	SlotWeapons[Index] = nullptr;
	PreloadHandles[Index].Reset();
	OnLoadoutSlotChanged.Broadcast(Slot);

	if (ActiveSlot == Slot)
	{
		SwitchToSlot(EWeaponSlot::Unarmed);
	}
}

void UWeaponLoadoutComponent::PreloadSlot(const EWeaponSlot Slot)
{
	const int32 Index = static_cast<int32>(Slot);
	const UWeaponItemDefinition* Weapon = GetSlotWeapon(Slot);
	if (!Weapon || Weapon->WeaponActorClass.IsNull())
	{
		return;
	}

	if (Weapon->WeaponActorClass.Get())
	{
		if (PendingSwitchSlot.IsSet() && PendingSwitchSlot.GetValue() == Slot)
		{
			PendingSwitchSlot.Reset();
			SpawnAndEquip(Slot);
		}
		return;
	}

	TWeakObjectPtr<UWeaponLoadoutComponent> WeakThis(this);
	PreloadHandles[Index] = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Weapon->WeaponActorClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda([WeakThis, Slot]()
		{
			if (UWeaponLoadoutComponent* Loadout = WeakThis.Get())
			{
				if (Loadout->PendingSwitchSlot.IsSet() && Loadout->PendingSwitchSlot.GetValue() == Slot)
				{
					Loadout->PendingSwitchSlot.Reset();
					Loadout->SpawnAndEquip(Slot);
				}
			}
		}));
}

USkeletalMeshComponent* UWeaponLoadoutComponent::FindAttachMesh(const FName SocketName) const
{
	const ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return nullptr;
	}

	// Prefer the mesh that actually has the requested socket (first-person arms).
	TInlineComponentArray<USkeletalMeshComponent*> Meshes(Character);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh->DoesSocketExist(SocketName))
		{
			return Mesh;
		}
	}
	return Character->GetMesh();
}

void UWeaponLoadoutComponent::SpawnAndEquip(const EWeaponSlot Slot)
{
	UWeaponItemDefinition* Weapon = GetSlotWeapon(Slot);
	UClass* WeaponClass = Weapon ? Weapon->WeaponActorClass.Get() : nullptr;
	ACharacter* Character = GetOwnerCharacter();
	if (!Weapon || !WeaponClass || !Character)
	{
		return;
	}

	DestroyActiveWeapon();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFPSWeaponBase* NewWeapon = GetWorld()->SpawnActor<AFPSWeaponBase>(
		WeaponClass, Character->GetActorTransform(), SpawnParams);
	if (!NewWeapon)
	{
		return;
	}

	NewWeapon->InitFromDefinition(Weapon);

	if (USkeletalMeshComponent* AttachMesh = FindAttachMesh(Weapon->AttachSocket))
	{
		NewWeapon->AttachToComponent(AttachMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, Weapon->AttachSocket);
	}

	ActiveWeaponActor = NewWeapon;
	ActiveSlot = Slot;
	NewWeapon->OnEquipped(Character);
	OnActiveWeaponChanged.Broadcast(ActiveSlot, NewWeapon);
}

void UWeaponLoadoutComponent::DestroyActiveWeapon()
{
	if (ActiveWeaponActor)
	{
		ActiveWeaponActor->StopFire();
		ActiveWeaponActor->OnUnequipped();
		ActiveWeaponActor->Destroy();
		ActiveWeaponActor = nullptr;
	}
}
