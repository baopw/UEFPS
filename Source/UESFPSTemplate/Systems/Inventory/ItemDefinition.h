#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/Inventory/InventoryTypes.h"
#include "ItemDefinition.generated.h"

class AFPSWeaponBase;
class UTexture2D;

/**
 * Base definition of an inventory item. Create instances as data assets under
 * Content/CoreSystems/ItemSystem/Data (DA_*). Registered with the Asset Manager
 * under primary asset type "Item" so items can be resolved from ids in save games.
 */
UCLASS(BlueprintType)
class UESFPSTEMPLATE_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType PrimaryAssetTypeName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
	FText Description;

	/** Icon shown in inventory grids. Soft reference; only loaded by the UI on demand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Which inventory tab this item is allowed to live in. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EInventoryTab Tab = EInventoryTab::Miscellaneous;

	/** Maximum quantity per slot. 1 = unique/non stackable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 MaxStack = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 0))
	float Weight = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemRarity Rarity = EItemRarity::Common;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bCanDrop = true;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(PrimaryAssetTypeName, GetFName());
	}
};

/** Kind of weapon slot a weapon item occupies in the loadout. */
UENUM(BlueprintType)
enum class EWeaponKind : uint8
{
	Melee   UMETA(DisplayName = "Melee"),
	Ranged  UMETA(DisplayName = "Ranged")
};

/**
 * A weapon that can be equipped into the loadout. The spawned actor class is a
 * soft reference: it is asynchronously preloaded by the loadout component when
 * assigned to a slot and never synchronously loaded during gameplay.
 */
UCLASS(BlueprintType)
class UESFPSTEMPLATE_API UWeaponItemDefinition : public UItemDefinition
{
	GENERATED_BODY()

public:
	UWeaponItemDefinition()
	{
		Tab = EInventoryTab::Weapons;
		MaxStack = 1;
	}

	/** Actor spawned when the weapon is drawn. Preloaded asynchronously via the Asset Manager. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSoftClassPtr<AFPSWeaponBase> WeaponActorClass;

	/** Melee weapons go to the Melee slot, ranged weapons to RangedPrimary/RangedSecondary. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EWeaponKind WeaponKind = EWeaponKind::Ranged;

	/** Socket on the character first-person mesh the weapon actor attaches to. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttachSocket = TEXT("GripPoint");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0))
	float BaseDamage = 20.f;

	/** Shots (or swings) per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0.1))
	float FireRate = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0))
	float Range = 10000.f;

	/** Rounds per magazine. Ignored for melee weapons. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0))
	int32 MagazineSize = 30;
};

/** What a consumable does when used. */
UENUM(BlueprintType)
enum class EConsumableEffect : uint8
{
	RestoreHealth  UMETA(DisplayName = "Restore Health"),
	RestoreStamina UMETA(DisplayName = "Restore Stamina"),
	GrantXP        UMETA(DisplayName = "Grant XP"),
	/** Permanently expands an inventory tab by Magnitude slots (dynamic inventory size demo). */
	ExpandTab      UMETA(DisplayName = "Expand Inventory Tab")
};

/**
 * A usable item. Effects are applied in C++ by the inventory component which
 * routes them to the Blueprint vital components (BPC_health etc.) via reflection.
 */
UCLASS(BlueprintType)
class UESFPSTEMPLATE_API UConsumableItemDefinition : public UItemDefinition
{
	GENERATED_BODY()

public:
	UConsumableItemDefinition()
	{
		Tab = EInventoryTab::Consumables;
		MaxStack = 10;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	EConsumableEffect Effect = EConsumableEffect::RestoreHealth;

	/** Health / stamina / XP amount, or number of slots for ExpandTab. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	float Magnitude = 25.f;

	/** Only used by ExpandTab. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	EInventoryTab TargetTab = EInventoryTab::Miscellaneous;
};
