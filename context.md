# UESFPSTemplate — Working Context

_Last updated: 2026-07-22_

## Interaction / pickup system (look-at + press E)

- Replaced the old sphere-overlap auto-collect on `AItemPickup` with a Cyberpunk-style
  look-at interaction:
  - **`Systems/Interaction/InteractableInterface.h`** — `IInteractable` (BlueprintNativeEvent
    interface): `GetInteractionText`, `GetInteractionVerb`, `CanInteract`, `OnBeginFocus`,
    `OnEndFocus`, `Interact`. Any actor (item, enemy body, door) can implement it.
  - **`Systems/Interaction/InteractionComponent.{h,cpp}`** — add `UInteractionComponent` to the
    player pawn. Line-traces from the camera each tick (`TraceDistance`, `TraceChannel`,
    `TraceInterval`), highlights the focused interactable, and binds `IA_Interact` (fallback key
    **E**). Broadcasts `OnFocusedInteractableChanged(Actor, Text, Verb)` for the HUD prompt.
  - **`Systems/Pickups/ItemPickup.{h,cpp}`** — `AItemPickup`/`AWeaponPickup` now implement
    `IInteractable`. Mesh is the root (QueryOnly, blocks Visibility so the trace hits it).
    `Interact` moves the item into the inventory (partial-fit remainder stays in world);
    `AWeaponPickup::Interact` also auto-equips on first pickup. BP hooks: `OnCollected`,
    `SetHighlighted(bool)`.
- **Still TODO in-editor:** add `UInteractionComponent` to the player character BP; build the
  HUD prompt widget bound to `OnFocusedInteractableChanged`; ensure `IA_Interact` is mapped to E
  in `IMC_Default`. Equip/unequip/trash from the inventory already exist on the components
  (`WeaponLoadoutComponent::EquipWeaponFromInventory` / `ClearSlot`,
  `InventoryComponent::RemoveFromSlot`).

## Progress today

- Created the missing **`AFPSWeaponBase`** C++ class in `Source/UESFPSTemplate/Systems/Weapons/`
  (`FPSWeaponBase.h` / `FPSWeaponBase.cpp`), inheriting from `AActor`.
  - Provides the full surface the existing `UWeaponLoadoutComponent` already calls into:
    `InitFromDefinition()`, `OnEquipped(ACharacter*)`, `OnUnequipped()`, `StartFire()`,
    `StopFire()`, `Reload()`, plus `GetDefinition()` / `GetOwningCharacter()` getters.
  - `OnEquipped` / `OnUnequipped` / `StartFire` / `StopFire` / `Reload` are
    `BlueprintNativeEvent`s so Blueprint children (BP_Rifle, BP_MeleeWeapon, ...) implement
    actual fire/reload behavior. Class is marked `Abstract`.
  - Owns a `WeaponMesh` skeletal mesh root (owner-only-see, no collision/shadow — standard
    first-person setup) and tracks `Definition` + `OwningCharacter`.
- Committed the two new weapon files with message
  `"Add AFPSWeaponBase C++ weapon actor base class"` (commit succeeded; push still pending).

## Current architecture

Module root maps `Source/UESFPSTemplate/` → include prefix `Systems/...`.

```
Source/UESFPSTemplate/Systems/
  Inventory/
    InventoryTypes.h        // EInventoryTab, EWeaponSlot, EItemRarity, FInventorySlot, FInventoryTabData
    ItemDefinition.h/.cpp    // UItemDefinition, UWeaponItemDefinition, UConsumableItemDefinition
    InventoryComponent.h/.cpp
  Weapons/
    WeaponLoadoutComponent.h/.cpp  // 4 logical slots, async preload, Enhanced Input
    FPSWeaponBase.h/.cpp           // NEW C++ weapon actor base (AActor)
  Pickups/
    ItemPickup.h            // AItemPickup, AWeaponPickup
  Save/
    PlayerProgressSaveGame.h
```

- **Loadout:** `UWeaponLoadoutComponent` manages 4 slots (Unarmed / Melee / RangedPrimary /
  RangedSecondary). Weapon actor classes are soft-referenced on `UWeaponItemDefinition` and
  asynchronously preloaded via the Asset Manager when assigned to a slot, so equipping never
  synchronously loads assets during gameplay.
- **Weapon lifecycle:** loadout spawns `AFPSWeaponBase`, calls `InitFromDefinition` →
  attaches to the character mesh socket → `OnEquipped`; on switch/holster it calls
  `StopFire` → `OnUnequipped` → `Destroy`.
- Concrete weapons are intended to be Blueprint children of `AFPSWeaponBase`
  (BP_Rifle refactored to child, plus BP_MeleeWeapon and a second ranged using ScifiWeapons).

## Immediate next steps

1. Push the committed weapon-base changes (push was still pending).
2. Regenerate project files and compile from the editor/IDE to verify `AFPSWeaponBase` builds
   against all call sites in `WeaponLoadoutComponent.cpp`.
3. Reparent `BP_Rifle` onto `AFPSWeaponBase` and stub `BP_MeleeWeapon` + a second ranged weapon.

## Environment note

- The integrated shell in recent sessions returned no output/exit status for any command
  (even `echo`), which blocked automated git verification. Reloading the Cursor window
  typically resets the execution environment.
