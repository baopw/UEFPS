#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "../Inventory/InventoryTypes.h"
#include "PlayerProgressSaveGame.generated.h"

class AActor;
class UInventoryComponent;
class UWeaponLoadoutComponent;

/**
 * A saved inventory slot. Only primary keys are serialized (item id, quantity,
 * instance id) - never full item data - so tweaking item stats in their data
 * assets later can not corrupt existing saves.
 */
USTRUCT()
struct FSavedInventorySlot
{
	GENERATED_BODY()

	UPROPERTY()
	FPrimaryAssetId ItemId;

	UPROPERTY()
	int32 SlotIndex = 0;

	UPROPERTY()
	int32 Quantity = 0;

	UPROPERTY()
	FGuid InstanceId;
};

USTRUCT()
struct FSavedInventoryTab
{
	GENERATED_BODY()

	UPROPERTY()
	EInventoryTab Tab = EInventoryTab::Miscellaneous;

	UPROPERTY()
	int32 MaxSlots = 0;

	UPROPERTY()
	TArray<FSavedInventorySlot> Slots;
};

USTRUCT()
struct FSavedWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY()
	EWeaponSlot Slot = EWeaponSlot::Unarmed;

	UPROPERTY()
	FPrimaryAssetId WeaponItemId;
};

/**
 * Persists player progress (vitals, inventory, loadout) between sessions.
 * Weapon actor references are never saved; the loadout is reconstructed from
 * item ids through the Asset Manager on load.
 */
UCLASS()
class UESFPSTEMPLATE_API UPlayerProgressSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static const FString SaveSlotName;

	UPROPERTY()
	bool bHasVitals = false;

	UPROPERTY()
	double CurrentHealth = 0.0;

	UPROPERTY()
	double CurrentStamina = 0.0;

	UPROPERTY()
	double CurrentXP = 0.0;

	UPROPERTY()
	double Level = 1.0;

	UPROPERTY()
	TArray<FSavedInventoryTab> InventoryTabs;

	UPROPERTY()
	TArray<FSavedWeaponSlot> Loadout;

	/** Collects inventory, loadout and vitals from the player actor and writes the slot. */
	static bool SaveFor(AActor* PlayerActor);

	/** Applies saved tabs/items to the inventory. Returns false when no save exists. */
	static bool LoadInventory(UInventoryComponent* Inventory);

	/** Applies saved slot assignments to the loadout. Returns false when no save exists. */
	static bool LoadLoadout(UWeaponLoadoutComponent* Loadout);

	/** Applies saved vitals to the Blueprint vital components on the actor. */
	static bool LoadVitals(AActor* PlayerActor);

private:
	static UPlayerProgressSaveGame* LoadFromSlot();
	static class UItemDefinition* ResolveItem(const FPrimaryAssetId& ItemId);
};
