# WeaponLocker one-visit VR LOADOUT (2026-10-22)

**Branch:** `feat/locker-vr-loadout-one-visit`  
**Basis:** availability audit **`dccc10d`** (verdict **A1**, closed).

## Availability (`availableOrdinaryMask`)

```text
nativeMask = *(uint32_t*)(RvInventoryManager+0x3E4)  // vtable 0x019CB480
for index in {0,1,2,5}:
  if nativeMask & (1<<index):
    row = GGameSystem+0x160 + index*0x50
    require row+0x24==0 and (row+0x4C&1)==0
    vr |= map(index)  // 0→1, 1→2, 2→4, 5→8
```

Pure policy: `locker_vr_loadout_policy.h` / `locker_vr_loadout_runtime.h`.  
**No** FString parsing for eligibility.

## UI + queue

- INI: `[Input] LockerVrLoadout=0` (default OFF), requires `KeepAcquiredWeapons=1`.
- Draw on **WYPOSAŻ** only: last exec **`LoadEquippedWeapons`** (`g_pageSignal==1`), not **`LoadUpgrades`**.
- Compact 2×2 panel ~`(u=0.78,v=0.47)`; **`EQUIP ALL`** queues `pendingAcquireMask` (no inventory writes while open).
- Post **`PopMenu`** (`0xF17F60` hook, after original):  
  `pendingNow = queue & AvailableOrdinaryMask() & ~NativeOwnedInventory::Mask()` then acquire batch.

## Acquisition source (proof)

| Step | Witness |
|------|---------|
| Archetype factory | Mission unlock **`0xF03450`**: catalog **`row+0x0C`** FString → **`0x4AFFD0`** → **`GGameSystem` vtable `+0x20C`** |
| Duplicate +D70 chain | **`0x11B4B6E`–`0x11B4BB3`**: same factory, then **`0xF69590`**, then pawn vtable **`+0xD70`** `keep=1` |
| Pickup adapter | `InventoryVmHook::RunAuditedPickupRestore` / `inventory_pickup_policy.h` |

Runtime requires offline-pinned **`GGameSystem` (`RvGameSystem`) vtable `0x1BCF9C4` +0x20C → `0xF85E30`** (prologue fingerprint in `locker_vr_loadout_identity.h`). Live session verifies; it does not define the expected target. Witness: `tools/reload/verify_locker_vr_loadout_factory_offline.py`.  
**No** GObjects scan, **no** `LoadEquippedWeapons` grant, **no** `AddWeapon +0x434` on the mod path.

## Threading (hardening)

- Engine/input `PollEngine()` publishes `g_published*` masks; render/XR read only `LockerVrLoadoutPublish`.
- PopMenu hook schedules `g_acquireRequested`; `+D70` runs on next safe input tick after locker defocus.
- Mod overlay clicks consume GFx mouse-down when hit targets match.

## Tests

- `tools/reload/test_locker_vr_loadout_policy.py`
- `tools/reload/test_locker_vr_loadout_hardening.py`
- `spikes/view_matrix/tests/test-locker-vr-loadout-policy.cpp`
