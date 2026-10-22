# WeaponLocker read-only diagnostic pass — 2026-10-22

**Branch:** `feat/locker-vr-loadout-audit-20261022`  
**Enable:** `[Diagnostic] LockerNativeTrace=1`  
**Disable overlay:** `[Input] LockerOwnedDisplay=0`

## Resolved page exec addresses (offline + prefix verify at install)

| Method | VA (body) | Vtable slot | ABI (capture) | Registration |
|--------|-----------|-------------|---------------|--------------|
| LoadUpgrades | `0x00F87C80` | **none** | `ret 8`; `[ebp+8]→esi` FFrame | `ScriptExecFn(self,frame,result)` — **hook** |
| Upgrade | `0x00F87E60` | **none** | `ret 8` (same shape) | witness only — **do not hook** |

Core vtable (`0x019A0948`) unchanged: Start `0x11C`, PopMenu `0x190`, LoadEquippedWeapons `0x1F4`, equip `0x1F8`.

Witness tool: `python tools/reload/inspect_locker_page_methods.py --exe ...\\Singularity.exe --text ...\\Singularity.text-memory.bin`

## Reflected movie fields (cached once per session)

- `mEquippedWeapons` / `mOriginalWeapons` (`ArrayProperty`)
- `mActiveWeaponSlot` / `mRequestedPackages` (`IntProperty`)

## Bounded log format

**Enter/exit (unchanged budget):**

`locker native trace: #N METHOD ENTER|EXIT tid=... movie=...`

**Context line (every budgeted ENTER on session movie):**

`locker native trace: ctx #N METHOD tid=... movie=... activeSlot=... reqPkg=... eq0=FName(mesh) eq1=FName(mesh) owned=0x... lastPageLoad=LoadEquippedWeapons|LoadUpgrades|Unknown`

**Availability (cap 24 lines/session, on equip / LoadEquippedWeapons / LoadUpgrades):**

`locker avail witness: ... mWeaponsAvailableToLocker=...`  
`locker avail witness: ... SessionManager=... mWeapons count=...`

**Page signal:** `g_lastPageLoadSignal` set on ENTER of `LoadEquippedWeapons` (1) or `LoadUpgrades` (2) only — **analysis aid**, not a proven CURRENT page gate until PSVR2 ordering confirms symmetric Equip↔Upgrade transitions.

## PSVR2 protocol

See audit doc § test protocol (Equip → cycle → Upgrade → back → cycle → close).

## Build (x86 `/O2`)

- **Crash build (NO-GO):** `DA8CD473…` — see `docs/locker-trace-crash-20261022.md`
- **Corrected candidate (not deployed):** `33A8417393B49A1ADE7F7AA8772786FC35D0098F701AE465636A8EC1980DA7CA`
- **Provenance:** `spikes/view_matrix/d3d9.build.json`

## Steam deploy — corrected trace `7d440d5` (2026-10-22 ~14:15)

- **Pre backup:** `deployment/install-backups/20261022-locker-trace-7d440d5-pre/` (`d3d9.dll` `9E7CEFF9…`, INI `ACCABCA1…`)
- **Installed DLL:** `33A8417393B49A1ADE7F7AA8772786FC35D0098F701AE465636A8EC1980DA7CA`
- **INI:** `LockerNativeTrace=1`; overlay off; Phase F retention unchanged
- **Stop rollback:** restore pre backup DLL `9E7CEFF9…`, set `LockerNativeTrace=0`

**Post-PSVR2 stable restore (2026-10-22):** Steam `d3d9.dll` `9E7CEFF998741F229810B5FCB00B3220ED12893789EB27676D1CCDA87F178CCD`; `LockerNativeTrace=0`, `LockerOwnedDisplay=0`; retention flags unchanged. Availability research continues offline only.

## Steam deploy (2026-10-22) — superseded NO-GO build

Accepted-profile guard blocked (live `d3d9.dll` / extra INI keys drift from pinned profile). Manual controlled deploy:

- **Backup:** `deployment/install-backups/20261022-locker-trace-diagnostic/` (`d3d9.dll` `9E7CEFF9…`, INI `7F69D038…`)
- **Installed DLL:** `DA8CD473…`
- **INI:** `LockerOwnedDisplay=0`; kept `KeepAcquiredWeapons=1`, `LockerRetainOrdinary=1`, `WeaponWheelOwnedInventory=1`, `LockerNativeTrace=1`
- **Guard plan (reference):** `deployment/change-plan-locker-trace-diagnostic-20261022.json` (`before` must match profile pin when guard is usable)

## Post-run conclusions (fill after log review)

1. **currentPage rule:** PROVEN / NOT PROVEN  
2. **availableOrdinaryMask producer:** PROVEN / NOT PROVEN  
