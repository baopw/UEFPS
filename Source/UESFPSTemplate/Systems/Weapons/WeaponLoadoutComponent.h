#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Systems/Inventory/InventoryTypes.h"
#include "WeaponLoadoutComponent.generated.h"

class AFPSWeaponBase;
class UInputAction;
class UInputMappingContext;
class UWeaponItemDefinition;
struct FInputActionValue;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveWeaponChanged, EWeaponSlot, Slot, AFPSWeaponBase*, WeaponActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadoutSlotChanged, EWeaponSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponSwitchDenied, EWeaponSlot, Slot);

/**
 * Manages the four logical weapon slots (Unarmed / Melee / RangedPrimary /
 * RangedSecondary), weapon actor spawning and Enhanced Input for switching,
 * holstering, firing and reloading.
 *
 * Weapon actor classes referenced by definitions are asynchronously preloaded
 * via the Asset Manager the moment they are assigned to a slot, so equipping
 * never synchronously loads assets during gameplay.
 */
UCLASS(ClassGroup = (CoreSystems), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UESFPSTEMPLATE_API UWeaponLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponLoadoutComponent();

	UPROPERTY(BlueprintAssignable, Category = "Weapons")
	FOnActiveWeaponChanged OnActiveWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapons")
	FOnLoadoutSlotChanged OnLoadoutSlotChanged;

	/** Fired when switching to an empty slot is attempted ("No weapon in slot" UI feedback). */
	UPROPERTY(BlueprintAssignable, Category = "Weapons")
	FOnWeaponSwitchDenied OnWeaponSwitchDenied;

	/** Weapons assigned at startup when no save game exists. Unarmed needs no entry. */
	UPROPERTY(EditAnywhere, Category = "Weapons")
	TMap<EWeaponSlot, TSoftObjectPtr<UWeaponItemDefinition>> DefaultLoadout;

	// Input actions. When the soft references resolve to assets (created under
	// /Game/FirstPerson/Input/Actions) their key mappings come from IMC_Default;
	// otherwise runtime actions with the documented default keys are created.
	UPROPERTY(EditAnywhere, Category = "Weapons|Input")
	TSoftObjectPtr<UInputAction> WeaponSlotActions[4];

	UPROPERTY(EditAnywhere, Category = "Weapons|Input")
	TSoftObjectPtr<UInputAction> HolsterAction;

	UPROPERTY(EditAnywhere, Category = "Weapons|Input")
	TSoftObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, Category = "Weapons|Input")
	TSoftObjectPtr<UInputAction> ReloadAction;

	// ---- Queries ----

	UFUNCTION(BlueprintPure, Category = "Weapons")
	EWeaponSlot GetActiveSlot() const { return ActiveSlot; }

	UFUNCTION(BlueprintPure, Category = "Weapons")
	AFPSWeaponBase* GetActiveWeaponActor() const { return ActiveWeaponActor; }

	UFUNCTION(BlueprintPure, Category = "Weapons")
	UWeaponItemDefinition* GetSlotWeapon(EWeaponSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	bool IsSlotOccupied(EWeaponSlot Slot) const;

	// ---- Mutations ----

	/** Holsters the current weapon and draws the weapon in Slot. Unarmed always succeeds. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool SwitchToSlot(EWeaponSlot Slot);

	/** Same as SwitchToSlot(Unarmed). */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void HolsterWeapon();

	/** Assigns a weapon definition to its matching slot and starts preloading its actor class. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool AssignWeapon(EWeaponSlot Slot, UWeaponItemDefinition* Weapon);

	/** Equips a weapon item from the inventory Weapons tab into the loadout. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool EquipWeaponFromInventory(const FInventorySlot& InventorySlot, EWeaponSlot PreferredSlot);

	/** Picks the first free (or first, if all taken) slot matching the weapon kind. */
	UFUNCTION(BlueprintPure, Category = "Weapons")
	EWeaponSlot FindSlotForWeapon(const UWeaponItemDefinition* Weapon) const;

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void ClearSlot(EWeaponSlot Slot);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	static constexpr int32 NumSlots = static_cast<int32>(EWeaponSlot::Count);

	UPROPERTY()
	TObjectPtr<UWeaponItemDefinition> SlotWeapons[4];

	UPROPERTY()
	TObjectPtr<AFPSWeaponBase> ActiveWeaponActor;

	EWeaponSlot ActiveSlot = EWeaponSlot::Unarmed;

	/** Slot requested while its weapon class was still streaming in. */
	TOptional<EWeaponSlot> PendingSwitchSlot;

	TSharedPtr<FStreamableHandle> PreloadHandles[4];

	UPROPERTY()
	TObjectPtr<UInputMappingContext> RuntimeMappingContext;

	void SetupInput();
	void BindResolvedAction(class UEnhancedInputComponent* Input, TSoftObjectPtr<UInputAction>& SoftAction,
		const struct FKey& FallbackKey, void (UWeaponLoadoutComponent::*PressedHandler)(const FInputActionValue&),
		void (UWeaponLoadoutComponent::*ReleasedHandler)(const FInputActionValue&) = nullptr);

	void OnSlot1(const FInputActionValue&);
	void OnSlot2(const FInputActionValue&);
	void OnSlot3(const FInputActionValue&);
	void OnSlot4(const FInputActionValue&);
	void OnHolster(const FInputActionValue&);
	void OnFirePressed(const FInputActionValue&);
	void OnFireReleased(const FInputActionValue&);
	void OnReload(const FInputActionValue&);

	UFUNCTION()
	void OnPawnRestarted(APawn* Pawn);

	void PreloadSlot(EWeaponSlot Slot);
	void SpawnAndEquip(EWeaponSlot Slot);
	void DestroyActiveWeapon();
	void RemoveLegacyRifle();
	class USkeletalMeshComponent* FindAttachMesh(FName SocketName) const;
	class ACharacter* GetOwnerCharacter() const;
};
