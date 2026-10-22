# Locker native trace — instrumentation (not deployed)

**Branch:** `diag/locker-native-trace-20261022`  
**Status:** Build-only; **no Steam deploy** until trace scope is reviewed. Arm A baseline unchanged (`LockerStationGrantOrdinary=0`, withdraw DLL).

## Enable (after approved deploy only)

```ini
[Diagnostic]
LockerNativeTrace=1
MenuAssetWitness=1
```

Default **`LockerNativeTrace=0`** — zero hooks, zero overhead.

## What is instrumented (read-only)

| Source | Identity check | Records |
|--------|----------------|---------|
| `Start` | Vtable slots @ `0x019A0948` → `0xF11430` | ENTER/EXIT, tid, FNames in `mEquippedWeapons` / `mOriginalWeapons`, owned mask/count |
| `equip` | `0xF00F50` | same |
| `LoadEquippedWeapons` | `0xF23840` | same |
| `PopMenu` | `0xF17F60` | same |
| Native pickup | Existing `+0xD70` adapter | tid, `armed` session flag, archetype **name/class pointer**, keep in/out, rc, mask |

**Budget:** 64 total events, 20 per exec method, 24 pickup lines per run. **No** TArray writes, **no** `RunAuditedPickupRestore`, **no** GObjects scan, **no** per-frame logging.

## Log prefix

`locker native trace:` — grep `%LOCALAPPDATA%\SingularityVR\view_matrix.log`.

## Session semantics

- **SESSION OPEN** on `RvUIGFxMovie_WeaponLocker` focus (same path as VR UI focus log).
- **SESSION CLOSE** when focus leaves WeaponLocker class.
- Monotonic **`#N`** sequence across exec + pickup for ordering analysis.

## Questions this trace answers (evidence plan)

1. **Event order** — Does `LoadEquippedWeapons` run before/after `Start`, between `equip` calls, or only on close before `PopMenu`?
2. **Four vs two** — Do `equip` / TArray FName lists ever exceed **two** entries while UI is open? (availability vs two-slot equipped)
3. **Ownership delta** — Does `owned mask/count` change only on `LoadEquippedWeapons`, `PopMenu`, or pickup during session?
4. **Threads** — Compare `hookTid` vs `pollTid` on PopMenu/pickup (Phase F thread safety).
5. **Archetype source** — Log **`archName` / `archClass` / pointer** on pickup when station grants (no scan).

## Rollback

Deploy backup as usual; set `LockerNativeTrace=0` or restore prior `d3d9.dll`.

## Post-trace deliverable (MAIN)

After one controlled station visit log review: update `docs/weapon-locker-native-contract-20261022.md` with **smallest valid Phase F integration point** (evidence-based, not hypothesis).
