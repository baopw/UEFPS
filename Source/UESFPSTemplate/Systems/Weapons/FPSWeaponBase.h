#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeaponBase.generated.h"

class ACharacter;
class USkeletalMeshComponent;
class UWeaponItemDefinition;

/**
 * C++ base for all weapon actors spawned by UWeaponLoadoutComponent.
 * Blueprint children (BP_Rifle, BP_MeleeWeapon, ...) implement the actual
 * fire / reload behavior; this class owns the definition reference, the
 * owning character and the equip lifecycle hooks.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class UESFPSTEMPLATE_API AFPSWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AFPSWeaponBase();

	/** First-person weapon mesh, attached to the character's GripPoint socket by the loadout component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	/** Called by the loadout component right after spawning, before OnEquipped. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void InitFromDefinition(UWeaponItemDefinition* InDefinition);

	/** Called when the weapon is drawn and attached to the character. */
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void OnEquipped(ACharacter* NewOwningCharacter);

	/** Called right before the weapon actor is holstered and destroyed. */
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void OnUnequipped();

	/** Fire input pressed. Blueprint children implement tracing / projectile spawning. */
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void StartFire();

	/** Fire input released. */
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void StopFire();

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void Reload();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UWeaponItemDefinition* GetDefinition() const { return Definition; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	ACharacter* GetOwningCharacter() const { return OwningCharacter; }

protected:
	virtual void OnEquipped_Implementation(ACharacter* NewOwningCharacter);
	virtual void OnUnequipped_Implementation();
	virtual void StartFire_Implementation() {}
	virtual void StopFire_Implementation() {}
	virtual void Reload_Implementation() {}

	/** Definition this actor was spawned from. Set via InitFromDefinition. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponItemDefinition> Definition;

	/** Character currently holding the weapon. Valid between OnEquipped and OnUnequipped. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ACharacter> OwningCharacter;
};
