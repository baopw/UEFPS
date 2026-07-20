#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Systems/Inventory/InventoryTypes.h"
#include "InventoryComponent.generated.h"

class UConsumableItemDefinition;
class UItemDefinition;

/**
 * Fired whenever the contents of a tab change. SlotIndex is the slot that
 * changed, or INDEX_NONE when the whole tab changed (e.g. capacity resize).
 * UI must only refresh from this event - never poll on Tick.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, EInventoryTab, Tab, int32, SlotIndex);

/**
 * Tabbed, dynamically resizable player inventory.
 * Pure state + rules; all presentation lives in the UI widgets.
 */
UCLASS(ClassGroup = (CoreSystems), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UESFPSTEMPLATE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	/** Starting slot count per tab. Capacity can be changed at runtime with SetTabCapacity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TMap<EInventoryTab, int32> DefaultTabCapacities;

	/** Items granted on BeginPlay when no save game exists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TMap<TObjectPtr<UItemDefinition>, int32> StartingItems;

	// ---- Queries ----

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetTabCapacity(EInventoryTab Tab) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetUsedSlotCount(EInventoryTab Tab) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventorySlot>& GetItemsInTab(EInventoryTab Tab) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventorySlot GetSlot(EInventoryTab Tab, int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 CountItem(const UItemDefinition* Item) const;

	/** True if Quantity of Item would fully fit into its tab. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(const UItemDefinition* Item, int32 Quantity = 1) const;

	// ---- Mutations ----

	/**
	 * Adds up to Quantity of Item into its tab (stacks first, then empty slots).
	 * Returns the amount that did NOT fit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UItemDefinition* Item, int32 Quantity = 1);

	/** Removes up to Quantity of Item from its tab. Returns the amount actually removed. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItem(UItemDefinition* Item, int32 Quantity = 1);

	/** Removes up to Quantity from a specific slot. Returns the amount actually removed. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveFromSlot(EInventoryTab Tab, int32 SlotIndex, int32 Quantity = 1);

	/**
	 * Moves/merges/swaps the content of one slot onto another. Cross-tab moves
	 * are rejected because every item belongs to exactly one tab.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveSlot(EInventoryTab FromTab, int32 FromIndex, EInventoryTab ToTab, int32 ToIndex);

	/** Splits Quantity off a stack into an empty slot of the same tab. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SplitStack(EInventoryTab Tab, int32 FromIndex, int32 ToIndex, int32 Quantity);

	/**
	 * Resizes a tab (cyberware / bag upgrades). Growing keeps everything;
	 * shrinking is rejected if occupied slots would be cut off.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetTabCapacity(EInventoryTab Tab, int32 NewCapacity);

	/**
	 * Consumes one item from the slot and applies its effect. Health/stamina/XP
	 * effects are routed to the Blueprint vital components on the owner.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseConsumable(int32 SlotIndex);

	/** Blueprint hook fired after a consumable effect was applied (VFX/SFX). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnConsumableUsed(const UConsumableItemDefinition* Consumable);

	// ---- Persistence (used by UPlayerProgressSaveGame flow) ----

	UFUNCTION(BlueprintCallable, Category = "Inventory|Save")
	void SaveProgress();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Save")
	bool LoadProgress();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class UPlayerProgressSaveGame;

	UPROPERTY()
	TArray<FInventoryTabData> Tabs;

	FInventoryTabData& GetTabData(EInventoryTab Tab);
	const FInventoryTabData& GetTabData(EInventoryTab Tab) const;
	bool IsValidSlotIndex(EInventoryTab Tab, int32 SlotIndex) const;
	void ApplyConsumableEffect(const UConsumableItemDefinition* Consumable);
	void GrantStartingItems();
};
