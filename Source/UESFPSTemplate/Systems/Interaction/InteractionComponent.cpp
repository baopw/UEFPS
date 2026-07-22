#include "InteractionComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InteractableInterface.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = TraceInterval;

	InteractAction = TSoftObjectPtr<UInputAction>(
		FSoftObjectPath(TEXT("/Game/FirstPerson/Input/Actions/IA_Interact.IA_Interact")));
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// TickInterval is copied from the default at registration, so re-apply the edited value.
	PrimaryComponentTick.TickInterval = TraceInterval;

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveRestartedDelegate.AddDynamic(this, &UInteractionComponent::OnPawnRestarted);
		if (Pawn->InputComponent)
		{
			SetupInput();
		}
	}
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Make sure a focused actor's highlight is cleared if we get torn down while aiming at it.
	SetFocusedActor(nullptr);
	Super::EndPlay(EndPlayReason);
}

void UInteractionComponent::OnPawnRestarted(APawn* Pawn)
{
	SetupInput();
}

void UInteractionComponent::SetupInput()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	UEnhancedInputComponent* Input = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
	if (!Input)
	{
		return;
	}

	UInputAction* Action = InteractAction.LoadSynchronous();
	if (!Action)
	{
		// No asset available: build a runtime action mapped to the documented default key (E).
		Action = NewObject<UInputAction>(this);
		Action->ValueType = EInputActionValueType::Boolean;
		if (!RuntimeMappingContext)
		{
			RuntimeMappingContext = NewObject<UInputMappingContext>(this);
		}
		RuntimeMappingContext->MapKey(Action, EKeys::E);
	}

	Input->BindAction(Action, ETriggerEvent::Started, this, &UInteractionComponent::OnInteractPressed);

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

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateFocus();
}

void UInteractionComponent::UpdateFocus()
{
	SetFocusedActor(TraceForInteractable());
}

AActor* UInteractionComponent::TraceForInteractable() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC)
	{
		return nullptr; // Only the locally controlled player traces for interactables.
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), false, GetOwner());
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, TraceChannel, Params))
	{
		return nullptr;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor || !HitActor->Implements<UInteractable>())
	{
		return nullptr;
	}

	// Respect the interactable's own gating (e.g. inventory full, locked door).
	if (!IInteractable::Execute_CanInteract(HitActor, const_cast<APawn*>(Pawn)))
	{
		return nullptr;
	}

	return HitActor;
}

void UInteractionComponent::SetFocusedActor(AActor* NewFocus)
{
	AActor* OldFocus = FocusedActor.Get();
	if (OldFocus == NewFocus)
	{
		return;
	}

	if (OldFocus && OldFocus->Implements<UInteractable>())
	{
		IInteractable::Execute_OnEndFocus(OldFocus);
	}

	FocusedActor = NewFocus;

	FText Text = FText::GetEmpty();
	FText Verb = FText::GetEmpty();
	if (NewFocus && NewFocus->Implements<UInteractable>())
	{
		IInteractable::Execute_OnBeginFocus(NewFocus);
		Text = IInteractable::Execute_GetInteractionText(NewFocus);
		Verb = IInteractable::Execute_GetInteractionVerb(NewFocus);
	}

	OnFocusedInteractableChanged.Broadcast(NewFocus, Text, Verb);
}

void UInteractionComponent::OnInteractPressed(const FInputActionValue& Value)
{
	TryInteract();
}

void UInteractionComponent::TryInteract()
{
	AActor* Target = FocusedActor.Get();
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Target || !Pawn || !Target->Implements<UInteractable>())
	{
		return;
	}

	if (!IInteractable::Execute_CanInteract(Target, Pawn))
	{
		return;
	}

	IInteractable::Execute_Interact(Target, Pawn);

	// The interaction may have destroyed the actor (item picked up) or changed its
	// state; re-run the trace so the prompt refreshes on the same frame.
	UpdateFocus();
}
