# WeaponLocker virtual ordinary slots — implementation sketch (2026-10-22)

**Status:** Phase B **PSVR2 log PASS** (2026-10-22): `mask=0x1` → plan `uiOrdinary=1` mesh **5799**; native witness `mEquippedWeapons count=2` (Start 0/1 vs owned mismatch documented). Phase C wired (`LockerVirtualApply=0` default): FName TArray rewrite on focus when enabled. PopMenu/equip hooks still **phase D/E**.

## Problem (witness-backed)

Native `RvUIGFxMovie_WeaponLocker`:

| Method | Address | Risk |
|--------|---------|------|
| Start | `0xF11430` | Builds `mEquippedWeapons` from controller **slots 0 and 1 only**. |
| equip | `0xF00F50` | Writes `equipped[slotNum]` **without bounds check**. |
| PopMenu | `0xF17F60` | `SetWeaponSlotName(i)` for **every** `equipped[i]`; `i≥2` overwrites special slots. |
| LoadEquippedWeapons | `0xF23840` | Grants from equipped TArray at load (not a fixed 0/1 loop). |

The VR wheel (`WeaponWheelOwnedInventory`) is a **different producer**. Do not append four names into the native TArray and call PopMenu unchanged.

## Goal

Show up to **four acquired ordinary firearms** in the locker UI while:

1. **Preserving** native controller slots **≥ 2** (heavy / grenade / special).
2. Sourcing ordinary entries from **owned inventory** (same mesh set as the wheel: 5799, 11387, 8297, 11389).
3. **No** catalog unlocks, save edits, or resizing the fixed native controller array.

## Policy code (offline)

`spikes/view_matrix/locker_virtual_contract.h` — pure decisions:

- `PlanVirtualLoad(mask)` — UI list up to 4; controller participation capped at 2 for Start-shaped paths.
- `DecidePopMenuControllerWrite(i, count)` — **refuse** `i ≥ 2`.
- `DecideEquipIndex(slotNum, count)` — refuse out-of-range slotNum.

Tests: `spikes/view_matrix/tests/test-locker-virtual-contract.cpp`.

## Runtime phases

| Phase | Flag | Behaviour |
|-------|------|-----------|
| **A** | OFF | Native locker only (current Steam). |
| **B** | `LockerVirtualOrdinary=1` | On `WeaponLocker` focus: log `locker virtual: plan …` from owned mask. **No mutation.** |
| **C** | `LockerVirtualApply=1` | After fresh owned snapshot on focus: rewrite `mEquippedWeapons` + `mOriginalWeapons` FName TArrays (bounded by capacity, fail-closed). **Not** a Start vtable hook yet. |
| **D** | + hook | **PopMenu** detour: filter `SetWeaponSlotName` loop with `DecidePopMenuControllerWrite`; optional scoped restore of slots 2+. |
| **E** | + hook | **equip** detour: enforce `DecideEquipIndex`; never pass script index ≥ `equipped.Count`. |
| **F** | + grant | For UI selections beyond controller slot pair, use **LoadEquippedWeapons** / inventory grant path (audit parity with `SwitchWeaponByArchetype`), not controller slot 2/3 writes. |

## Hook anchors (Singularity 2010, archived `.text` base `0x402000`)

Install only after vtable identity check (`inspect_locker_contract.py`):

- Vtable `0x019A0948`, slot `0x11C` → Start `0xF11430`.
- Slot `0x190` → PopMenu `0xF17F60`.
- Slot `0x1F8` → equip `0xF00F50`.
- Slot `0x1F4` → LoadEquippedWeapons `0xF23840` (grant observation / phase F).

Prefer **wrap-after-native-Start** (call original, then rewrite TArrays) over replacing Start entirely, so GFx init stays native.

PopMenu re-equip script path: `push 0xF0B470` → dispatcher `0x41F380` (not a direct call) — any script bridge hook must match that shape.

## Falsifiers (stop and re-audit)

- PopMenu still invokes `SetWeaponSlotName` for **slot ≥ 2** with ordinary weapons.
- Special slot weapons missing or wrong after close.
- `equip` receives `slotNum ≥ equipped.Count` without refusal.
- Locker UI shows weapons not in owned mask.
- Menu attract / class-2 frontend regresses (keep locker work off `ScriptedCameraFlat` menu experiments).

## INI

```ini
[Input]
; Default OFF. Phase B: logging only on WeaponLocker focus.
LockerVirtualOrdinary=0
LockerVirtualApply=0
```

Independent of `TmdNativeAim`, NIS, and `effects_remaster`.

## Verification ladder

1. `python tools/reload/test_locker_runtime_contract.py`
2. `test-locker-virtual-contract.exe` (x86 /O2)
3. Steam log: `locker virtual: plan …` with mask matching wheel
4. PSVR2: locker UI + commit without special-slot loss (only after phase C–F)
