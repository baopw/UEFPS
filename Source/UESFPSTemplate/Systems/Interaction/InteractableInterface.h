#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

class APawn;

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by anything the player can look at and interact with (world item
 * pickups, enemy bodies to loot, doors, terminals, ...). The player's
 * UInteractionComponent line-traces from the camera every tick, calls
 * OnBeginFocus/OnEndFocus as the aim moves on and off the object, reads
 * GetInteractionText for the on-screen prompt, and calls Interact when the
 * player presses the Interact key (E).
 *
 * All methods are BlueprintNativeEvents so Blueprint actors can implement or
 * override the behavior. Always invoke them through the static thunks, e.g.
 * IInteractable::Execute_Interact(Object, Pawn).
 */
class UESFPSTEMPLATE_API IInteractable
{
	GENERATED_BODY()

public:
	/** Text shown in the interaction prompt while focused (e.g. item name + quantity). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionText() const;

	/** Verb shown next to the key prompt (e.g. "Pick Up", "Loot", "Open"). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionVerb() const;

	/** Whether Interactor can currently interact. Returning false hides the prompt. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(APawn* Interactor) const;

	/** Called when the player's aim starts resting on this object (highlight/outline on). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnBeginFocus();

	/** Called when the player looks away or moves out of range (highlight/outline off). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnEndFocus();

	/** Perform the interaction (pick up, loot, open, ...). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(APawn* Interactor);
};
