#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/InteractableInterface.h"
#include "ItemPickup.generated.h"

class UItemDefinition;
class UStaticMeshComponent;

/**
 * World pickup the player picks up by looking at it and pressing Interact (E),
 * Cyberpunk-style. It implements IInteractable: the player's UInteractionComponent
 * highlights it and shows its name while focused, and calls Interact to move the
 * item into the inventory. If only part of the quantity fits, the remainder stays
 * in the world.
 *
 * Set Item + Quantity. The mesh is the root and must be hit by the interaction
 * trace channel (Visibility by default) - keep its collision query-enabled.
 */
UCLASS(BlueprintType, Blueprintable)
class UESFPSTEMPLATE_API AItemPickup : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AItemPickup();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = 1))
	int32 Quantity = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Blueprint hook for pickup feedback (sound, VFX) before the actor is destroyed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void OnCollected(APawn* Collector);

	/**
	 * Blueprint hook to toggle the highlight/outline while the player is looking at
	 * this pickup. Drive a post-process outline or material param from here.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void SetHighlighted(bool bHighlighted);

	// ---- IInteractable ----
	virtual FText GetInteractionText_Implementation() const override;
	virtual FText GetInteractionVerb_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void OnBeginFocus_Implementation() override;
	virtual void OnEndFocus_Implementation() override;
	virtual void Interact_Implementation(APawn* Interactor) override;
};

/**
 * Weapon pickup: adds the weapon to the Weapons tab and auto-equips it into
 * the first free loadout slot of the matching kind the first time it is picked up.
 */
UCLASS(BlueprintType, Blueprintable)
class UESFPSTEMPLATE_API AWeaponPickup : public AItemPickup
{
	GENERATED_BODY()

public:
	virtual void Interact_Implementation(APawn* Interactor) override;
};
