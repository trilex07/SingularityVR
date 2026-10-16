# Centurion reload — first controlled prototype

SIDE, 2026-10-16. Initial base: MAIN `5763e6c`; synchronized into this own
branch first with committed MAIN `db4a261`, then `f0b96ad`, preserving its
menu/holster, knife-joint, cinematic transition and PSVR2 changes. No Steam operations.
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
Original extracted clip geometry now has an independent default-OFF,
albedo-only carried preview (implemented, not GPU/headset verified). It is NOT
removed from the original gun. Do not promote this as completed manual reload.

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
  AttemptReload, IsReloading, AnimNotify_RefillClip. Absence may mean native/script
  coverage mismatch. First four target the current player weapon, the refill
  notify targets the current player pawn: NPCs cannot exhaust the budget.
- Probe-only mode now independently logs ammo changes (maximum 128), plus the
  current weapon name/class and class-chain UFunction bindings. Uses only the
  existing measured Children/Next/Super and voted Func offsets: maximum sixteen
  combined pawn/weapon identities, 2048 field reads/identity and 16 binding records/identity.
  Logs shared-interpreter versus game-text binding, never calls it. Function
  identity is not an ABI or execution proof. No new hook is installed.
- Off-hand trigger latch requires a physical trigger release after handling
  ammo, so a held L2 cannot leak into a TMD/aim action on insertion/cancellation.
- HUD observations expire after 250 ms and are bound to pawn/weapon/HUD
  identity; context gaps clear them. Missing reflected fields retry at most
  once/second instead of remaining permanently unresolved. A capacity change
  cancels the transaction even if clip/reserve deltas happen to balance.
  Grip ownership stays tied to physical LEFT hand across a handedness change,
  not to whichever hand becomes the new support hand. No extra XR polls.

## Native reload animation continuation (offline)

UEViewer loaded the existing PRIVATE `wp_SP_ARPistol_SF.xxx` and resolved
the measured first-person mesh material slot to
`wp_alt_pistol.Materials.AR_Pistol`. That instance references original
`AR_Pistol_d`, `AR_Pistol_n`, `AR_Pistol_s` and `AR_Pistol_rim`.
The parent shader/reflection imports were missing in this offline package set;
this is NOT proof of the complete live shader or a reason to substitute
the unrelated `handgun_d` texture. Runtime cylinder rendering remains gated.
The original texture cache also exported successfully to the ignored private
`build/centurion-reload/native-material` directory: diffuse 1024x1024 DXT5,
normal 1024x1024 DXT1, specular 256x256 DXT1 and rim 128x128 DXT1.
Specular/rim are the exporter's available mip level 2, not claimed full-res.
Diffuse SHA-256:
`0C958ED11C3311DDBB517A8893EC7E372D0811A58582220F04A6EFD30A0D9A39`;
normal `AE206388BB839175C3D3D0DB453A7B7A0F0505B6DB76FC3DC46D950EC23AE2D8`;
specular `9189EEC3AC33B84A32FA4EEB4FA1C251B984FA2C9C1AD41DC5A3120D5ADCDF4E`;
rim `FFD873DD23120D67AA36EAFBB7ADB65A5283C428A5E1032A948B44374F73DA70`.
These assets are staged privately. Current DLL renders calibration boxes by
default; opt-in carried preview is albedo only, NOT a complete native material.

Original `Alt_pistol` exports 10 sequences, 78 tracks / 228 frames / 17784
keys. Private PSA SHA-256:
`9A30ECE4C009BDBEA8672AEEA67B832F1AD840E946CB3887CEFBF584DAA412D3`.
`Reload`: 53 frames at 24 Hz, 2.20833 s. Its `clip` track has maximum local
translation 42.3398 UU (frame 18) and rotation 65.6277 degrees from the first
sample; both return to the original pose at the end. Other nine sequences
have no local clip motion in this export. These are parent-relative authoring
coordinates, not a controller socket, palette index or physical scale proof.
Crucially, PSA `clip` is track 54, PSK `clip` is bone 57, parent
`cog_alt_pistol`: match by NAME, never assume indices coincide. UEViewer's
PSA parent fields are placeholders; use the original PSK hierarchy instead.

The narrow offline PSA reader validates bounded chunks, contiguous sequence
windows, finite/unit rotations, exact key counts and a byte-exact re-encoding
of all original records/FLOAT32 keys. Its four synthetic tests include a known
translation/rotation, mismatched track indices and malformed input.

Separately, decompression of the private package validated all three chunks;
the existing package inspector accepted all 1105 exports with exact serial
tiling. Reload export 77 (outer `Alt_pistol`) contains 14 animation notifies.
Its script notify at **1.47052908 s** is **`AnimNotify_RefillClip`**. UEViewer's
independent notify dump agrees. The validated RvGame package locates this
function under **`RvSharedPawn`**, not `RvWeaponShared`. No function body,
ABI or inventory semantics are inferred just from this name.

`inspect_reload_notifies.py` fully consumes the tagged array, validates field
bounds/reference targets and has four synthetic parser tests. Both readers
produce ignored private reports, never redistribute animation keys/assets.
The probe now watches this fifth event ONLY on the current PLAYER pawn;
the other four remain restricted to its current weapon. Binding observations
cover both objects with a combined maximum of 16 identities, and timestamps
allow MAIN to compare callback entry and actual conserved ammo change.
Absence of the callback remains inconclusive (native/interpreter coverage).
Falsifier for the next mechanism: notify entry without the expected clip/
reserve transfer is not a commit proof and must not trigger a direct call.

L1 continuation: seven CPU executables, including deterministic 360000-frame
reload lifecycle replay (7584 insert requests / 6753 conserved acknowledgements /
5957 cancellations), plus 12 Python tests. Positive controls and interruptions
include focus/tracking loss, stale HUD, weapon/pawn change, nonfinite positions,
clock regression, capacity change and ammo pickup. This is CPU evidence only.

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

### Optional carried-cylinder preview (implemented, NOT physically accepted)

Safety continuation hypothesis: the outer calibration pass currently logs a
failed state restore but retries next frame. A repeated capture/restore failure
could keep disturbing native gun/menu rendering. Latch ONLY this optional
presentation pass off after capture/restore failure, until D3D Reset/XR init;
retain gesture/input/native ammo behavior. A draw failure with successful outer
restore must not be confused with restore failure. Falsifier: synthetic fault
injection still executes another draw while latched, fails to restore after an
early draw failure, or Reset cannot re-enable the pass. Existing fake-D3D tests
separately cover actual RT/MRT/DS/viewport/reference restoration; no GPU or
physical acceptance is claimed by this policy test.

Implemented: a lock-free presentation-only latch guards both calibration markers
and carried cylinder. Capture/outer-restore failure stops subsequent presentation
before any D3D calls; only Reset/XR input init re-arms it. An ordinary draw failure
still attempts the full outer restore; if that succeeds, marker retry remains
allowed and cylinder failure retains its existing cyan fallback. CPU policy
tests cover eight partial-draw failures, 100000 suppressed frames, capture and
combined draw/restore failure, plus recovery. Eight reload CPU executables and
27 Python tests PASS. This does not repair a driver's failed restore in the
faulting frame; it prevents repeated experimental mutations afterward.

Hypothesis: the measured rigid cylinder and its original albedo can replace the
cyan carrying marker without altering native gun/clip triangles or inventory.
Falsifiers: marker/asset disagrees between eyes, old-pawn snapshot stays visible,
the same grip triggers other actions, or any D3D state/COM reference leaks.

`CenturionClipPreview=1` requires `CenturionManualReload=1`, with the native tap
still OFF for this presentation-only round. Original private assets are
`vr-assets/reload/centurion/centurion_clip.svrk` and `centurion_clip.svrt` next
to the DLL. The preparation tool's explicit `--diffuse AR_Pistol_d.dds` option
creates the second cache with original BGRA channels/alpha data preserved.
Texture cache SHA-256:
`17A3355AD6CC7E2F35A03883FDF4B314577FEE908773262C5BC199C006C6CD60`.
The textured three-axis CPU preview was inspected: six chamber bases and native
metal/surface detail match the extracted cylinder UVs. No generated repaint.

Reuses the existing tested knife renderer with an explicit mesh/albedo loader;
its original `Load(file,sceneLit)` path remains `knife.svrt` with identical
rendering behavior. Eager CPU loading at XR input initialization took 2.166 ms
in one local run (not a GPU or hitch guarantee). GPU resources warm at the first
eligible gameplay frame, before the gesture; fixed scratch arrays allocate at
load, not while carrying. Two indexed calls per carrying frame, one per eye,
same symmetric eye/half-FOV projection contract as existing knife/menu overlays.
No new shader/hook, D3DPOOL_MANAGED or direct DEFAULT LockRect. SYSTEMMEM upload,
DEFAULT texture, private depth, nested full-state/RT/MRT/depth/viewport restore;
Reset releases GPU resources. Missing asset/GPU fault logs and keeps cyan marker.

Fresh input/render snapshot is bound to pawn+weapon, expires at 100 ms and
requires focus, tracking, gameplay and Carrying. Pausing, holstering, swapping,
returning to menus or releasing the cylinder hides it. The authored mount is
8 cm forward in left aim-space by default; configurable XYZ/Y-X-Z rotation,
not inferred from the engine bind matrix. Original mesh scale remains unchanged.
There is NO glove/scene-depth occlusion, fitted finger grip, dynamic lighting,
normal/specular shading, native cylinder removal or firing lockout here.
This remains presentation calibration, not completed physical reload.

Eight CPU executables /14 Python tests cover lifecycle, 50653 mount rotations,
projection equivalence, stale identity gates, original UV/FLOAT32 geometry,
BGRA/alpha preservation, bounded texture input, and existing D3D fake-state /
texture upload failures. Actual 1067-vertex /1252-triangle +1024x1024 cache
pair loaded through the explicit renderer API. No GPU/Steam/headset proof.
`tools/reload/stage-preview.ps1` stages ONLY this checkout's ignored private
candidate directory, verifies DLL/manifest/source and both fixed asset hashes,
and writes a separate receipt; no INI copy, launch or deployment. MAIN must
integrate on its current accepted base and own the actual installation lease.

All are `[Input]`, restart required; omitted feature flags are OFF:

```ini
CenturionManualReload=0
CenturionNativeReloadTap=0
CenturionReloadProbe=0
CenturionClipPreview=0
ReloadPouchDownCm=42
ReloadPouchForwardCm=34
ReloadSocketRightMm=-35
ReloadSocketUpMm=-35
ReloadSocketBackMm=-80
ReloadClipRightMm=0
ReloadClipUpMm=0
ReloadClipBackMm=-80
ReloadClipPitchDeg=0
ReloadClipYawDeg=0
ReloadClipRollDeg=0
```

The socket numbers are bounded **authored calibration controls**, not measured
engine offsets. Pouch X stays physical centre; yaw only, never pitch/roll.
Its visible box is smaller than its 10 cm acquisition radius. White socket
box has a 7.5 cm acquisition radius; extraction travel is 15 cm.

First MAIN L3: `CenturionReloadProbe=1`, other three OFF. Load Centurion save,
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

Optional separate MAIN presentation check after the gesture dry-run: enable
only the clip preview in addition to manual mode; native tap remains OFF.
Carry the fresh cylinder, roll/pitch the left hand, insert/release, pause and
holster. Expected: only the carried object uses the original albedo; native
cylinder remains on the gun for now. Assess mount/scale/eye agreement separately
from inventory semantics, and never credit a draw log as physical acceptance.

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

### Offline log correlation helper (no runtime changes)

The ammo probe logs changed HUD samples only, without pawn ID; the interpreter
probe logs entry, not completion. Hypothesis for the next MAIN measurement:
a normal reload has a conserved positive clip/reserve transfer bracketed by
the same player's reload callbacks. Unequal deltas, changed identities, absent
callback coverage or a competing action falsify or leave that interpretation
unknown. Even a matching bracket is correlation, not authoritative ammo/ABI proof.

`python tools/reload/analyze_reload_log.py --log <archived-view_matrix.log>`
reads an explicit log and emits JSON to stdout; it never launches/attaches to
the game or writes output files. It separates attached-header sessions, checks
identities/count bounds, uses producer ticks rather than async file ordering,
and reports malformed rows, possible log caps and missing headers. Invalid
samples and identity/capacity changes break comparison chains. Only adjacent
changed samples at most 5000 ms apart are compared (configurable `--window-ms`,
an analysis heuristic, NOT an engine constant). `IsReloading` polling alone
does not count as a reload callback. Multiple callback pawn IDs cannot establish
single-player correlation. Input/record limits bound parsing and report size.

Output never marks native reload proven or upgrades L3/L4. The missing pawn
and unchanged-sample history prevent freshness/ownership proof; even exact
counts can conceal intervening actions. Thirteen synthetic regression tests
cover conservation, identity/session changes, malformed/capped input, timing,
and read-only input SHA/line preservation. The test runner includes them.
No runtime source, DLL, INI or staged asset changed for this helper.

Current presentation-safety L2_PASS: clean source
`E7A0C8986B10859A6832847C0D0A77F74D76D7F3`, `/O2`, x86 PE32 (`014C/010B`),
empty build-manifest working-tree status. DLL SHA-256:
`23AB074B813499DE2338EAC625079C3F5B0A5A1AB97196F6299F0B95BDCAB77A`.
Eight reload CPU executables /27 Python tests PASS, including the 100000-frame
presentation fault policy; six knife CPU executables /14 Python tests PASS.
Private original-rig replay/GPU execution are NOT claimed. Private staging
verifies DLL/source provenance and both unchanged original cylinder caches.
All four flags remain default OFF, no INI staging or Steam operations. L3/L4
UNKNOWN; no Centurion physical feedback yet. Native begin/commit/end remains
unproven, not implemented by this safety change.

Previous preview L2_PASS before presentation latch: clean source
`E0FB5E1E71434988ED20D24BA3AF1D97DACFB7CA`, `/O2`, x86 PE32 (`014C/010B`),
empty build-manifest working-tree status. DLL SHA-256:
`00C49400DC5FCF6A9F9DA2F67ED35E47F7239B86FB8B1F15915AC5D3B90E502B`.
Eight reload CPU executables /14 Python tests pass. Six knife CPU executables /
14 Python tests pass with the shared loader extension; original private rig
replay and GPU execution are NOT claimed. The actual cylinder/albedo pair also
loaded through that C++ API. Fixed-output private staging verified DLL, source
provenance and BOTH assets; its receipt is
`build/centurion-reload/candidate/centurion-preview.manifest.json`.
No INI was staged or installed. L3/L4 remain UNKNOWN; user has not yet tested
Centurion. All four feature flags remain default OFF, no Steam operations.

Previous synchronized L2_PASS before carried preview: clean merge source
`9C164EBA647C48998612C283EB4331FAC913A09F`, `/O2`, x86 PE32 (`014C/010B`),
empty build-manifest working-tree status. DLL SHA-256:
`99A00CB7C636856EAAA88A829ACD09E4548045D3B4A71FB30064812CFADDD051`.
That older candidate is superseded above. Seven reload CPU executables /
12 Python tests pass again; the synchronized six-executable knife suite /14
Python tests also pass. Its private original-rig replay was NOT requested and
is not claimed. MAIN base advanced by seven commits while this continuation
was running: its f0b96ad was merged ONLY INTO THIS SIDE branch. One d3d9 input
conflict was resolved by preserving MAIN's knife-impulse attack path AFTER the
reload ownership gate. No MAIN edits/reverts. Canonical INI, knife headers,
cinematic headers and muzzle policies remain byte-identical to that new base.

Previous continuation L2_PASS before latest MAIN synchronization: clean source
`861FC12C5C73AEAEA1D641CEBCABA902F93C4CD8`, `/O2`, x86 PE32 (`014C/010B`),
empty build-manifest working-tree status. DLL SHA-256:
`8DBB1B4FFCCDFDFE22B2E45039DC8C37DAC93F68421D99C05E51DDFB2F0B2AEA`.
Private candidate DLL and manifest hashes match. All seven CPU executables,
12 Python tests and the six-executable menu/holster regression suite pass.
L3/L4 remain UNKNOWN; SIDE did not launch, deploy or restore Steam.

L1_PASS: six optimized CPU executables (transaction + old gesture + existing
knife, holster, hand anchor, D3D fake-state regressions) and four Python
extraction tests; actual original-asset extraction/UV/bind round trip.
Continuation also passes the six-executable menu/holster regression suite from
the synchronized MAIN base (including shared knife/holster tests).

Previous L2_PASS (before this continuation): clean source `db1495986a9eef704b3aa6972277aa03ef3fca2c`,
`/O2`, x86 PE32 (`014C/010B`), empty working-tree status in build manifest.
Previous DLL SHA-256:
`BACD5FDBBB4C23E55149F4CFF9ECBCD3D6E8DE67FACF99EE1A423B8212AD9343`.
This older DLL is superseded by the continuation receipt above.
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
