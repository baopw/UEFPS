#pragma once

#include "CoreMinimal.h"
#include "InventoryTypes.generated.h"

class UItemDefinition;

/** Tabs of the player inventory. Every item definition belongs to exactly one tab. */
UENUM(BlueprintType)
enum class EInventoryTab : uint8
{
	Weapons        UMETA(DisplayName = "Weapons"),
	Clothes        UMETA(DisplayName = "Clothes"),
	Miscellaneous  UMETA(DisplayName = "Miscellaneous"),
	MissionItems   UMETA(DisplayName = "Mission Items"),
	Consumables    UMETA(DisplayName = "Consumables"),

	Count          UMETA(Hidden)
};

/** Logical weapon slots. Maps 1:1 to number keys 1-4. Unarmed is always available. */
UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Unarmed          UMETA(DisplayName = "Unarmed"),
	Melee            UMETA(DisplayName = "Melee"),
	RangedPrimary    UMETA(DisplayName = "Ranged Primary"),
	RangedSecondary  UMETA(DisplayName = "Ranged Secondary"),

	Count            UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common     UMETA(DisplayName = "Common"),
	Uncommon   UMETA(DisplayName = "Uncommon"),
	Rare       UMETA(DisplayName = "Rare"),
	Epic       UMETA(DisplayName = "Epic"),
	Legendary  UMETA(DisplayName = "Legendary")
};

/** One slot of an inventory tab. Item == nullptr means the slot is empty. */
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemDefinition> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = 0;

	/** Identity for unique (non-stackable) items, preserved through save/load. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid InstanceId;

	bool IsEmpty() const { return Item == nullptr || Quantity <= 0; }

	void Clear()
	{
		Item = nullptr;
		Quantity = 0;
		InstanceId.Invalidate();
	}
};

/** All slots of a single inventory tab. Slots.Num() always equals MaxSlots. */
USTRUCT(BlueprintType)
struct FInventoryTabData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	EInventoryTab Tab = EInventoryTab::Miscellaneous;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 MaxSlots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FInventorySlot> Slots;
};

namespace InventoryTabs
{
	constexpr int32 Num = static_cast<int32>(EInventoryTab::Count);

	inline FText GetDisplayName(const EInventoryTab Tab)
	{
		switch (Tab)
		{
		case EInventoryTab::Weapons:       return NSLOCTEXT("Inventory", "TabWeapons", "WEAPONS");
		case EInventoryTab::Clothes:       return NSLOCTEXT("Inventory", "TabClothes", "CLOTHES");
		case EInventoryTab::Miscellaneous: return NSLOCTEXT("Inventory", "TabMisc", "MISC");
		case EInventoryTab::MissionItems:  return NSLOCTEXT("Inventory", "TabMission", "MISSION");
		case EInventoryTab::Consumables:   return NSLOCTEXT("Inventory", "TabConsumables", "CONSUMABLES");
		default:                           return FText::GetEmpty();
		}
	}
}

namespace ItemRarity
{
	inline FLinearColor GetColor(const EItemRarity Rarity)
	{
		switch (Rarity)
		{
		case EItemRarity::Common:    return FLinearColor(0.55f, 0.57f, 0.60f);
		case EItemRarity::Uncommon:  return FLinearColor(0.10f, 0.85f, 0.35f);
		case EItemRarity::Rare:      return FLinearColor(0.00f, 0.75f, 1.00f);
		case EItemRarity::Epic:      return FLinearColor(0.75f, 0.20f, 1.00f);
		case EItemRarity::Legendary: return FLinearColor(1.00f, 0.65f, 0.05f);
		default:                     return FLinearColor::White;
		}
	}

	inline FText GetDisplayName(const EItemRarity Rarity)
	{
		switch (Rarity)
		{
		case EItemRarity::Common:    return NSLOCTEXT("Inventory", "RarityCommon", "Common");
		case EItemRarity::Uncommon:  return NSLOCTEXT("Inventory", "RarityUncommon", "Uncommon");
		case EItemRarity::Rare:      return NSLOCTEXT("Inventory", "RarityRare", "Rare");
		case EItemRarity::Epic:      return NSLOCTEXT("Inventory", "RarityEpic", "Epic");
		case EItemRarity::Legendary: return NSLOCTEXT("Inventory", "RarityLegendary", "Legendary");
		default:                     return FText::GetEmpty();
		}
	}
}
