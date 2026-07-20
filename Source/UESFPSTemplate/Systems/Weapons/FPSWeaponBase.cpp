#include "Systems/Weapons/FPSWeaponBase.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Systems/Inventory/ItemDefinition.h"

AFPSWeaponBase::AFPSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// First-person weapon: only render for the owning player.
	WeaponMesh->SetOnlyOwnerSee(true);
	WeaponMesh->bCastDynamicShadow = false;
	WeaponMesh->CastShadow = false;
}

void AFPSWeaponBase::InitFromDefinition(UWeaponItemDefinition* InDefinition)
{
	Definition = InDefinition;
}

void AFPSWeaponBase::OnEquipped_Implementation(ACharacter* NewOwningCharacter)
{
	OwningCharacter = NewOwningCharacter;
}

void AFPSWeaponBase::OnUnequipped_Implementation()
{
	OwningCharacter = nullptr;
}
