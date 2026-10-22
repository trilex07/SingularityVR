# WeaponLocker diagnostic crash — 2026-10-22 (PSVR2)

**Build:** `DA8CD473AEC62BE33CFCF072706B1922E1D067FE4FB2BEFF57182555CCE47A79`  
**Rollback installed:** `9E7CEFF998741F229810B5FCB00B3220ED12893789EB27676D1CCDA87F178CCD`  
**Crash dump:** `%LOCALAPPDATA%\CrashDumps\Singularity.exe.45292.dmp` (2026-10-22 ~14:05, thread **30308**)

## Hook firing order (log ends at LoadUpgrades EXIT)

| Question | Answer |
|----------|--------|
| **LoadUpgrades ENTER before crash?** | **Yes** — `#5 LoadUpgrades ENTER` |
| **LoadUpgrades EXIT before crash?** | **Yes** — full EXIT + ctx logged |
| **Upgrade ENTER before crash?** | **No** — no `Upgrade ENTER` line in session |
| **Upgrade hook installed?** | Yes at startup (`hooked Upgrade at 00F87E60`) |

Log file ends immediately after LoadUpgrades EXIT (no further `view_matrix.log` lines). User report: crash when entering **ULEPSZENIA** / Upgrade UI.

## Last locker-native trace lines (session tail)

```
locker native trace: #5 LoadUpgrades ENTER tid=30308 movie=3DC4BFA0 ...
locker native trace: ctx #5 LoadUpgrades ... lastPageLoad=LoadUpgrades
locker avail witness: LoadUpgrades mWeaponsAvailableToLocker offset unresolved
locker avail witness: LoadUpgrades SessionManager unresolved
locker native trace: LoadUpgrades EXIT tid=30308 movie=3DC4BFA0
```

Prior tab/equip sequence: Start → LoadEquippedWeapons → equip (Centurion→Valkyrie) → LoadEquippedWeapons → **LoadUpgrades**.

## Windows crash metadata

Minidump exception stream did not record a usable fault address (`code=0`, `addr=0`). No Application log hit in automated query. **Primary evidence remains hook ABI + log ordering.**

## Root-cause hypothesis (to prove on next build)

Both `LoadUpgrades` and `Upgrade` bodies end with **`ret 8`** (two stack arguments after `this` in ECX). Detours were compiled as **`void(__thiscall*)(void* self, void* frame)`** (implicit **`ret 4`** epilogue), while callers use **`ret 8`**. That mismatch corrupts the stack on return from the detour.

Offline disassembly (capture `20261006-205508`, body `0x00F87C80`):

- `mov ebx, ecx` — `this`
- `mov esi, [ebp+8]` — FFrame / script stack
- `ret 8` @ `0x00F87D50`
- `[ebp+0xC]` not read in body (RESULT_DECL slot still on stack)

## Fix for next candidate (not deployed)

- Typedef: `void(__thiscall*)(void* self, void* frame, void* result)`
- Hook **LoadUpgrades only**; leave **`Upgrade @ 0xF87E60` unhooked**
- No `NativeOwnedInventory::Poll()` from page hook / availability witness on that path
- Steam rolled back; `LockerNativeTrace=0` until review
