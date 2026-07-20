#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class UItemDefinition;
class USphereComponent;
class UStaticMeshComponent;

/**
 * World pickup that adds its item to the overlapping player's inventory.
 * Place in the level and set Item + Quantity. If only part of the quantity
 * fits, the remainder stays in the world.
 */
UCLASS(BlueprintType, Blueprintable)
class UESFPSTEMPLATE_API AItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AItemPickup();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = 1))
	int32 Quantity = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USphereComponent> CollectionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Blueprint hook for pickup feedback (sound, VFX) before the actor is destroyed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void OnCollected(APawn* Collector);

protected:
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

/**
 * Weapon pickup: adds the weapon to the Weapons tab and auto-equips it into
 * the first free loadout slot of the matching kind.
 */
UCLASS(BlueprintType, Blueprintable)
class UESFPSTEMPLATE_API AWeaponPickup : public AItemPickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
};
