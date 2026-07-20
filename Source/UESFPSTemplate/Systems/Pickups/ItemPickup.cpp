#include "ItemPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "../Inventory/InventoryComponent.h"
#include "../Inventory/ItemDefinition.h"
#include "../Weapons/WeaponLoadoutComponent.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	CollectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollectionSphere"));
	CollectionSphere->SetSphereRadius(80.f);
	CollectionSphere->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	SetRootComponent(CollectionSphere);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetupAttachment(CollectionSphere);

	CollectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AItemPickup::OnSphereOverlap);
}

void AItemPickup::OnSphereOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled() || !Item)
	{
		return;
	}

	UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>();
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
		Quantity = NotAdded;
		return;
	}

	OnCollected(Pawn);
	Destroy();
}

void AWeaponPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UWeaponItemDefinition* Weapon = Cast<UWeaponItemDefinition>(Item);
	APawn* Pawn = Cast<APawn>(OtherActor);
	const bool bHadItem = Weapon && Pawn && Pawn->IsPlayerControlled() &&
		Pawn->FindComponentByClass<UInventoryComponent>() &&
		Pawn->FindComponentByClass<UInventoryComponent>()->CountItem(Weapon) > 0;

	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	// Auto-equip into the first free matching slot the first time the weapon is obtained.
	if (Weapon && Pawn && !bHadItem)
	{
		if (UWeaponLoadoutComponent* Loadout = Pawn->FindComponentByClass<UWeaponLoadoutComponent>())
		{
			if (Pawn->FindComponentByClass<UInventoryComponent>()->CountItem(Weapon) > 0)
			{
				const EWeaponSlot Slot = Loadout->FindSlotForWeapon(Weapon);
				if (!Loadout->GetSlotWeapon(Slot))
				{
					Loadout->AssignWeapon(Slot, Weapon);
				}
			}
		}
	}
}
