#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Fired when the focused interactable changes. Interactable is the actor now under
 * the crosshair (nullptr when nothing is focused). InteractionText / InteractionVerb
 * carry the prompt data so the HUD widget can bind directly without re-querying.
 * UI must only refresh from this event - never poll on Tick.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFocusedInteractableChanged, AActor*, Interactable,
	const FText&, InteractionText, const FText&, InteractionVerb);

/**
 * Cyberpunk-style look-at interaction. Add this to the player pawn. Each tick it
 * line-traces from the camera; when the aim rests on an actor implementing
 * IInteractable it highlights it and shows a prompt, and pressing the Interact
 * key (E, via IA_Interact) runs the interaction. Looking away or walking out of
 * range clears the focus and hides the prompt automatically.
 */
UCLASS(ClassGroup = (CoreSystems), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UESFPSTEMPLATE_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	/** Bind the HUD interaction prompt widget to this. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnFocusedInteractableChanged OnFocusedInteractableChanged;

	/** How far the player can reach interactables (world units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float TraceDistance = 350.f;

	/** Trace channel used to detect interactables. Visibility works with the default pickup setup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Seconds between focus traces. 0 = every frame. Keep low (~0.05-0.1) for responsiveness. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = 0))
	float TraceInterval = 0.05f;

	/** Interact input action. Resolved from IA_Interact; falls back to a runtime E mapping. */
	UPROPERTY(EditAnywhere, Category = "Interaction|Input")
	TSoftObjectPtr<UInputAction> InteractAction;

	/** The actor currently focused (under the crosshair and in range), if any. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** Runs the interaction on the currently focused actor, if any. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Actor under the crosshair, kept weak so a destroyed pickup can't dangle. */
	TWeakObjectPtr<AActor> FocusedActor;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> RuntimeMappingContext;

	void SetupInput();
	void UpdateFocus();
	void SetFocusedActor(AActor* NewFocus);
	AActor* TraceForInteractable() const;

	void OnInteractPressed(const FInputActionValue& Value);

	UFUNCTION()
	void OnPawnRestarted(APawn* Pawn);
};
