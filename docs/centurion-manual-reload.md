# Centurion reload — first controlled prototype

SIDE, 2026-10-16. Initial base: MAIN `5763e6c`; synchronized into this own
branch with current committed MAIN `db4a261`, preserving its menu/holster,
knife-joint and PSVR2 bridge changes. No Steam operations.
Scope: Centurion only, right-hand weapon, left-hand interaction; existing left
knife and right gun holsters retain their positions, radii and mechanics.
The new ammo zone is front-centre, yaw-relative and head-height-relative, so
sitting on the floor does not anchor it to an inaccessible standing height.

## Mechanism and falsifier

Observed source: `manual_reload.h` was a CPU-only gesture recognizer, not wired
to runtime, inventory, assets or the engine. Native XInput `XI_X` is **shared
Use/reload**, not an independently proven Reload function. The existing hook
called `Hook_ProcessEvent` actually intercepts **ProcessInternal(FFrame&,Result)**;
its trampoline MUST NOT be invoked with UFunction/Parms arguments.

Hypothesis for this first slice: a contextual remove → fresh-ammo → insert
gesture can exclusively own the off-hand and request **one ordinary native
Use/reload tap**, while UE3 alone changes ammo and plays its existing animation.
Falsifiers: an owned squeeze also picks the knife/TMD/support latch; one insert
requests twice; or native clip gain is not matched by reserve loss. A callback
or successful XInput publication is NOT evidence of an accepted reload.

This is an **assisted gesture prototype**, not finished physical/manual reload:
native automatic reload and the full original animation remain. There is no
intermediate engine cylinder opening, cartridge transfer or firing lockout.
Calibration boxes represent the pouch/socket/carried token, not final art.
Original extracted clip geometry is prepared OFFLINE but is NOT yet rendered
or removed from the original gun. Do not promote this as completed manual reload.

## Implemented

- Allocation-free pure CPU transaction: Idle → Removing → Discard → NeedAmmo
  → Carrying → WaitingNative → Cooldown, with timeout and lifecycle cancellation.
- Grip edges, not proximity alone. Release away from the socket drops the token
  without spending/creating ammunition. The original HUD fields are read-only.
- Full/unknown/empty-reserve/unsupported/left-handed weapon, dead pawn, focus or
  tracking loss, firing, swap, holster, menu/cinema and busy hand fail closed.
- Knife wins overlapping acquisition zones. Owned off-hand squeeze suppresses
  TMD/support/knife acquisition until release, including after cancellation.
  Primary shared Use is suppressed only during the active transaction.
- Existing grip action/bindings reused: no extra XR polling, keyboard injection,
  DirectInput enumeration, native detour or UObject writes.
- Cached HUD observer at at most 10 Hz, with guarded reads and named
  `mClipAmmo/mBinAmmo/mClipSize` reflection per class. Reuses the existing HUD
  census; never introduces a GObjects walk into input. Unknown data disables it.
- Two-eye bounded calibration overlay, per-eye scissor, state block + explicit
  RT/MRT/DS/viewport restoration, Reset release, nonblocking snapshot lock.
  Overlaid markers intentionally have no world occlusion or final material claim.
- Independent default-OFF native reload observer, existing script hook only,
  at most 16 records per exported name: ServerStartReload, ReloadFinished,
  AttemptReload, IsReloading. Absence may mean native/script coverage mismatch.
- Probe-only mode now independently logs ammo changes (maximum 128), plus the
  current weapon name/class and class-chain UFunction bindings. Uses only the
  existing measured Children/Next/Super and voted Func offsets: maximum eight
  weapon identities, 2048 field reads/identity and 16 binding records/identity.
  Logs shared-interpreter versus game-text binding, never calls it. Function
  identity is not an ABI or execution proof. No new hook is installed.
- Off-hand trigger latch requires a physical trigger release after handling
  ammo, so a held L2 cannot leak into a TMD/aim action on insertion/cancellation.

## Original asset measurement

Existing private UEViewer export from `wp_SP_ARPistol_SF`:
`ch_inview_alt_pistol_mesh.psk`, SHA-256
`4E18A25D5241DE1B3F18100E3BD9373D464EE16BAF3E29D7C60289AE18FE73D4`.
5799 source wedges, 6767 triangles, 79 bones. Bone `clip` is 57 (measured,
not a runtime hardcoded index); 1009 rigid source points, 1067 UV vertices,
1252 extracted triangles, material slot 0, zero crossing triangles.

At the explicit authoring conversion 52.5 source units/metre the clip-local
bounding dimensions are approximately 10.064 × 10.010 × 12.582 cm. This is NOT
a headset-measured scale. All extracted UVs are preserved and all vertices
round-trip back to raw PSK bind space. The bind transform is research output,
NOT a proven controller socket/animation palette. Native material and animated
socket ownership remain to be captured; no guessed texture is assigned.

`tools/reload/prepare_centurion.py` writes an ignored private SVRK v2 cache
and report. Cache SHA-256:
`F21961145D6645EE93A8A2C73AE89380535D918352BABB1FD4F811C67422D89B`.
No game assets are committed/distributed or copied to Steam.

The private three-axis wireframe preview was inspected: this really is a
six-chamber cylinder, not a box magazine. The validated RvGame package export
table also contains `RvComponent_Weapon_PistolCylinder` with named cylinder
controller/current/desired rotation properties, plus the shared weapon Reload
state and its reload functions. Their presence does not prove a safe write or
call path. Do not apply an unrelated generic slide-pistol reload to this mesh.

## Flags and independent gates

All are `[Input]`, restart required; omitted values are OFF:

```ini
CenturionManualReload=0
CenturionNativeReloadTap=0
CenturionReloadProbe=0
ReloadPouchDownCm=42
ReloadPouchForwardCm=34
ReloadSocketRightMm=-35
ReloadSocketUpMm=-35
ReloadSocketBackMm=-80
```

The socket numbers are bounded **authored calibration controls**, not measured
engine offsets. Pouch X stays physical centre; yaw only, never pitch/roll.
Its visible box is smaller than its 10 cm acquisition radius. White socket
box has a 7.5 cm acquisition radius; extraction travel is 15 cm.

First MAIN L3: `CenturionReloadProbe=1`, other two OFF. Load Centurion save,
fire two shots and perform one ordinary existing native reload away from
interactables. Collect object/function identities, before/after HUD counts,
and check input/performance baseline. No new headset debugging requirement.

Then MAIN can test `CenturionManualReload=1`, native tap still OFF: the complete
gesture should emit exactly one `inserted/request`; the later native-timeout
is EXPECTED in this dry-run. Only after confirming identity, no collisions,
HUD ownership and the shared-X semantics may MAIN enable
`CenturionNativeReloadTap=1` for an assisted reload away from interactables.
Require `native-ammo-ack` with conserved counts; an interaction instead is a
failed backend, not success. The 120 ms X tap is not a dedicated Reload call.

### Three-sentence physical script for MAIN

1. Without switching to chat, load the Centurion save with reserve ammo and a
   partly empty gun; leave the knife docked, release both grips and confirm
   the yellow pouch marker is reachable in front of the centre of the belt.
2. At the white marker on the weapon squeeze the left grip, move that hand
   15 cm away and release; squeeze at the yellow marker, bring the cyan token
   to white and release, once only.
3. Try left knife/right gun holsters, pause/Options/resume, then close normally
   and report reach, accidental actions and ammo change; MAIN records counters
   and restores/verifies its accepted installation.

## Evidence / next native work

L1_PASS: six optimized CPU executables (transaction + old gesture + existing
knife, holster, hand anchor, D3D fake-state regressions) and four Python
extraction tests; actual original-asset extraction/UV/bind round trip.
Continuation also passes the six-executable menu/holster regression suite from
the synchronized MAIN base (including shared knife/holster tests).

Latest L2_PASS: clean source `45dd1faa5dcb9d75792a5eaaa4deeab2304f4c36`,
`/O2`, x86 PE32 (`014C/010B`), empty working-tree status in build manifest.
Latest DLL SHA-256:
`5607AA19203539725559C15258D5B9950BF363308AE5C6E006333218B1AECA39`.
Local `build/centurion-reload/candidate` DLL/manifest match this exact build.
Both the extraction cache hash and all original UVs remain unchanged after the
stricter encoded-FLOAT32 round trip and optional wireframe inspection.

Initial L2_PASS (older base; retained for provenance only): clean source
`01d7e2817580b610a983361ff0eab82dc3fbdde8`,
`/O2`, x86 PE32 (`014C/010B`), empty working-tree status in build manifest.
DLL SHA-256:
`71C733C6DA7DC1BA4648BC13C3DBB50DDCE19E4DA058D883DECF96D322CA03BA`.
Latest local DLL/manifest are in `build/centurion-reload/candidate`; latest
receipt is recorded below after rebuilding the synchronized source.
Compiler analysis retains inherited warnings; no fatal format-string gate
violations or compile errors. L3/L4 UNKNOWN; not physical acceptance of reload.

Next: use native observation to identify the authoritative weapon/ammo owner,
dedicated begin/commit/end reload path and animation notify boundaries. Prove
native `clip` transform/material in the actual first-person draw before mounting
the extracted item or suppressing native clip triangles. Do not mutate HUD
caches, synthesize capacity, invoke a guessed ABI, or disable auto-reload by name.
Other three weapons intentionally unchanged.

Integration: stack/cherry-pick only this task's commits onto CURRENT accepted
MAIN (including subsequent knife/menu changes) and rebuild. Never deploy this
older standalone base in place of accepted work. No merge authorized by SIDE.
