#include "ItemPickup.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "../Inventory/InventoryComponent.h"
#include "../Inventory/ItemDefinition.h"
#include "../Weapons/WeaponLoadoutComponent.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Query-only so the pickup never blocks pawn movement, but is still hit by the
	// interaction line trace (Visibility by default). Designers can enable physics
	// on top of this for dropped loot.
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

FText AItemPickup::GetInteractionText_Implementation() const
{
	if (!Item)
	{
		return FText::GetEmpty();
	}
	if (Quantity > 1)
	{
		return FText::Format(NSLOCTEXT("Pickup", "PickupNameQty", "{0} x{1}"),
			Item->DisplayName, FText::AsNumber(Quantity));
	}
	return Item->DisplayName;
}

FText AItemPickup::GetInteractionVerb_Implementation() const
{
	return NSLOCTEXT("Pickup", "PickupVerb", "Pick Up");
}

bool AItemPickup::CanInteract_Implementation(APawn* Interactor) const
{
	return Item != nullptr && Interactor != nullptr && Interactor->IsPlayerControlled() &&
		Interactor->FindComponentByClass<UInventoryComponent>() != nullptr;
}

void AItemPickup::OnBeginFocus_Implementation()
{
	SetHighlighted(true);
}

void AItemPickup::OnEndFocus_Implementation()
{
	SetHighlighted(false);
}

void AItemPickup::Interact_Implementation(APawn* Interactor)
{
	if (!Interactor || !Interactor->IsPlayerControlled() || !Item)
	{
		return;
	}

	UInventoryComponent* Inventory = Interactor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return;
	}

	const int32 NotAdded = Inventory->AddItem(Item, Quantity);
	if (NotAdded >= Quantity)
	{
		return; // Inventory full, leave the pickup in the world.
	}

	if (NotAdded > 0)
	{
		Quantity = NotAdded; // Partial pickup: leave the remainder behind.
		return;
	}

	OnCollected(Interactor);
	Destroy();
}

void AWeaponPickup::Interact_Implementation(APawn* Interactor)
{
	UWeaponItemDefinition* Weapon = Cast<UWeaponItemDefinition>(Item);
	UInventoryComponent* Inventory = Interactor ? Interactor->FindComponentByClass<UInventoryComponent>() : nullptr;
	const bool bHadItem = Weapon && Inventory && Inventory->CountItem(Weapon) > 0;

	// Base handles moving the weapon into the Weapons tab (and may Destroy() this actor).
	Super::Interact_Implementation(Interactor);

	// Auto-equip into the first free matching slot the first time the weapon is obtained.
	// Uses only cached locals - never touch this pickup's members after Super may have destroyed it.
	if (!Weapon || !Interactor || bHadItem || !Inventory)
	{
		return;
	}

	if (UWeaponLoadoutComponent* Loadout = Interactor->FindComponentByClass<UWeaponLoadoutComponent>())
	{
		if (Inventory->CountItem(Weapon) > 0)
		{
			const EWeaponSlot Slot = Loadout->FindSlotForWeapon(Weapon);
			if (!Loadout->GetSlotWeapon(Slot))
			{
				Loadout->AssignWeapon(Slot, Weapon);
			}
		}
	}
}
