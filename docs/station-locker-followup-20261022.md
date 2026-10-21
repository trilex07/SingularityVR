# Station presentation and native locker — 2026-10-22

User physically confirms NIS works. This is not acceptance of TMD or effects.
The installed combined candidate is unchanged by this offline follow-up.

## Smaller, centred station presentation (L1/L2 PASS; physical OPEN)

Existing INI `StationMenuScalePct=50`, `StationMenuDistanceMm=1400` did not
reach the private GFx RTT path. Runtime focus identified
`Menu_WeaponsLocker / RvUIGFxMovie_WeaponLocker`, with a registered 1280x720
private RTT. Direct-canvas-only scaling could not affect its provider plane.

Exact station class identity now selects the smaller, centred provider/input
plane for WeaponLocker, UseStation and BarisovStation. Notes, confirmations,
pause, frontend and the mod panel do not inherit this change. The same plane
drives both presentation and controller ray UVs. No texture or package patch.

`test-menu-hud-scale.cpp` passes x86 /O2 including exact class rejection and
captured RTT scale checks. Production x86 /O2 build passes. This does NOT prove
physical readability or pointer alignment. New DLL is local only, not installed.

## Four ordinary-firearm locker slots — NOT IMPLEMENTED

KeepAcquiredWeapons / the VR wheel are separate from the native locker.
Read-only disassembly of the owned executable and archived `.text` proves:

- Class vtable `0x019A0948`, constructor `0x00F9E750`.
- Start vtable +0x11c (`0x00F11430`) clears the original/equipped FName arrays,
  explicitly appends controller GetWeaponSlotName(0), then (1), and copies
  equipped into original. This is the actual two-slot producer.
- LoadEquippedWeapons +0x1f4 (`0x00F23840`) enumerates the equipped TArray;
  it is not a hardcoded two-iteration loop.
- equip +0x1f8 (`0x00F00F50`) writes the caller's array index without a proven
  spare-slot allocation. Invented GFx indices are unsafe.
- PopMenu +0x190 (`0x00F17F60`) compares original/equipped, removes deselected
  inventory, and writes EVERY equipped index via SetWeaponSlotName +0x744.
  Consequently merely growing the list to four is not a correct fix: it can
  overwrite special native slots and discard inventory at close.

Reproduce the bounded witness with `tools/reload/inspect_locker_contract.py`
and an owned EXE + archived text (`--text-base 0x402000`). No process access,
native mutation or proprietary extracted assets are added to Git.

Next implementation must virtualize the ordinary-firearm locker load/commit
contract against the already acquired inventory, preserve heavy/grenade native
slots, and reconcile the authored GFx slot layout. No catalog unlocks, save
edits or expanding the fixed native controller array. The existing
`MenuAssetWitness` optionally logs original/equipped array bounds on four
locker focus transitions; default remains OFF, no readback or captures.

Do not call this a four-slot candidate, bless it as accepted, or deploy an older
SIDE DLL. Preserve the current combined candidate and verified asset lease.
