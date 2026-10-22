# LockerAvailabilityTrace — minimal read-only witness

**Flag:** `[Diagnostic] LockerAvailabilityTrace=0` (default OFF). Independent of `LockerNativeTrace`.

## Hooks installed when enabled

| Target | VA | ABI |
|--------|-----|-----|
| `equip` only | `0x00F00F50` | `void __thiscall Equip(void* self, void* frame, void* slotNum, void* stackArg10)` — primary epilogue **`ret 0x0C`** (inspect_locker_abi.py) |

Session arm/disarm uses existing WeaponLocker **focus/defocus** on the input thread (`LockerAvailabilityTrace::OnFocus` / `OnDefocus`). No Start/Load/PopMenu/LoadUpgrades hooks.

## Read-only probes

- **Open:** `pawn+0x3D0` → `*(uint32*)(manager+0x3E4)`; live catalog `GGameSystem @ 0x01CF394C`, `+0x160/+0x164`, stride `0x50`, max 32 rows; log four `wp_SP_AR*` paths with runtime index, native mask, `row24`, `row4C`, `bitSet`.
- **Each equip (after native):** slot 0/1, FName, path, live index, native bit, current manager mask, `bitSet`.
- **Close:** `locker avail summary:` with `offeredOrdinary` (VR 1/2/4/8 from cycled FNames) vs `nativeProgressionOrdinary` (VR bits from live catalog + mask).

## Sample log lines

```
locker avail: SESSION OPEN movie=0x... mgr=0x... nativeProgressionMask=0x7 obj=... tid=...
locker avail: path=wp_SP_ARShotgun runtimeIndex=2 nativeMask=0x4 bitSet=1 row24=0 row4C=0
locker avail equip: slot=1 Weapon=ARMachineGun path=wp_SP_ARMachineGun idx=1 nativeBit=0x2 managerMask=0x7 bitSet=1
locker avail summary: offeredOrdinary=0x6 nativeProgressionOrdinary=0x7 observed={wp_SP_ARMachineGun,wp_SP_ARShotgun} (VR bits 1/2/4/8; native catalog bits separate; manager+0x3E4=0x7)
```

## Safety

No writes to `+0x3E4`, no `FD5BA0`/`FD5AC0`, no `EnableAllWeaponsForLocker`, no TArray mutation, no inventory `Poll()` from the equip detour.

## PSVR2 protocol

1. Open WeaponLocker, stay on **WYPOSAŻ**.
2. Cycle **one** native slot through the full weapon list until the first weapon repeats.
3. Do not enter **ULEPSZENIA**; close locker. No purchases.

## Decision (from logs)

- **A / PROVEN:** live indices match offline inference; every offered ordinary weapon has native bit set; every clear bit absent from full cycle (ideal: sniper clear + not offered).
- **B:** all four bits set and all four offered — report strong correlation, negative case unavailable.
- **C:** offered weapon with clear bit, or cycle/mask mismatch — retire `+0x3E4` as direct locker availability producer.
