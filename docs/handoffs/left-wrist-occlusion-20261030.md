# MAIN — wrist orientation and two-depth occlusion candidate

## Physical baseline and preserved scope

The user tested DLL `71F7254B871EE8C079196A6D02AAC3DB26A9D8517BDE6F255D780F4F4B42AEB2`.
Wrist panel visibility/tracking received a qualified positive response, but sideways text,
glove penetration and lack of magazine overlap FAILED. The response does not establish
Kasimov/Centurion, optional live values or performance acceptance.

Archived full session log: private `build/wrist-main/session-20261030/view_matrix.log`,
SHA256 `DE20959F96AC8ADB2A7B43E52BAA307EBA73F19020A9AF42023DC729939F8FA5`.
The installed baseline INI hash was `FB58219E802795F081965CABF193C3074E08471E9A3353207AAB020D1A916CE9`.
No right/left controller transform, grip, reload, weapon model, haptic or audio implementation is changed here.

## Proven mechanism and falsifier

Actual session frame 2527, 20 trace events, zero overflow:

- World eye submissions at draw 5 used native DS `389F4AC0`, D24S8, 5640x2876,
  no MSAA, full viewport with left/right half scissors. Native projection was c0 ROW.
- The late clear at draw 970 used that exact DS, Z=1, flags=6 (Z + stencil).
- Foreground eye draws 971 and wrist publication 972 still used the same DS.
- A later foreground projection at draw 989 had a different near-depth mapping.
- At Present, DS was null. The original HUD explicitly disabled Z and used a
  translucent backing. Turning on Z at Present cannot recover world occlusion.

Falsifier: if either native camera/attachment is unavailable, the scene surface
is discardable/unknown, the clear is partial/Z-only, or an extra scene-depth clear
invalidates the retained information, the new route refuses HUD rendering. It
must not fall back to drawing through the world or suppress the legacy health HUD.

## Minimal route

Opt-in `[Render] EquipmentWristOcclusion=1` (normal default 0):

1. The SDK depth-surface creation hook records actual discard metadata on the COM
   resource. Only full-backbuffer, non-MSAA D24S8 allocations use Discard=FALSE in
   this mode; all other arguments/resources are preserved. Unknown auto-depth
   surfaces are not accepted as preserved world depth.
2. The first successful c0 per-eye world submissions establish exact scene DS
   identity and native, un-ridden camera matrices. Collection is capped at eight
   attempts per phase/eye/frame. OFF issues no capture queries.
3. At the measured exact full-scene Z+stencil clear, bind a cached compatible
   private D24S8 attachment before forwarding the original Clear once. Only later
   binds of that exact world DS are redirected for the foreground pass. Shadow
   targets remain native. No depth copy, readback or GPU synchronization occurs.
4. Keep the wrist pose, tracking-to-native map and both projection families from
   the same frame. Native world XYZ/RHW renders panel/text into a color mask
   against retained world depth, without writing Z. A second quad submission uses
   native foreground XYZ/RHW and foreground Z to reject hand/weapon/magazine
   overlaps. Incompatible projective-W families fail closed.
5. Explicit RT0/MRT/depth/viewport restoration accompanies the existing full state
   block. Reset and ResetEx release cached resources. Present ends redirection.

Depth split receipts are capped at four. The existing one-frame collector now
also records native Z enable/write/compare state. GPU pixel fixtures validate the
two-depth mechanism; actual native carried-magazine depth writing remains part
of the headset/session acceptance, not an offline claim.

## Text and source-measured surface clearance

`EquipmentWristTextQuarterTurn=1` rotates the complete logical display by a quarter
turn in the existing physical footprint. Set 0 for the exact prior orientation.
Opaque backing replaces translucent backing. No wrist/controller offsets change.

The candidate INI uses `EquipmentWristOffsetXmm=-62`, rather than -25. This is a
panel-only normal separation measured from the actual private SVNC/SVTG indices
and original PSK wrist bind (row-vector, native Y reflection once), not an AIM-space
guess. The old plane intersects the source sleeve. At the retained scale of
65.625 UU/m, front bounds within the panel footprint are -3.45429243 UU for both
cuff profiles and -3.99431520 UU for SVTG. Adding one millimetre panel thickness
and rounding outwards yields -62 mm. Y/Z, dimensions and controller poses stay unchanged.

Reproduce with `tools/hud/measure_wrist_panel_clearance.py --psk-dir <private PSK directory>
--assets <installed/private glove-animation directory> --output <report.json>`.
Only derived bounds and hashes are committed in `wrist-panel-source-clearance-20261030.json`.
Bind-surface clearance does not prove final animated headset placement. Other
world scales, panel scales/angles or replacement assets require remeasurement.

## Validation and remaining gates

- HUD policy, stereo projection, final-palette/ride, source/freshness, placement
  and depth-policy regressions: PASS.
- Actual D3D9 and D3D9Ex pixels: both eyes reject wall, glove and carried-magazine
  regions independently while preserving the visible panel region. Unknown and
  discardable surfaces, exact identity, partial/Z-only clears, missing cameras,
  OFF path, failed bind, row/column projection, W mismatch, viewport restoration,
  Reset and ResetEx: PASS.
- Accepted weapon-mounted ammo HUD: 156 actual-atlas cases per backend PASS.
- Dual-TMD/hands/reload/accuracy, independent-hand shot haptics, OLED D3D11 lifetime
  and pixel fixtures, WeaponLocker policy/preview/layout and compiled x86 literal
  `[eax+20Ch]` factory ABI: PASS. Source/event paths for these systems are preserved.
- Optimized x86 PE32 build identity follows in the candidate JSON receipt.
- Actual native two-depth route, animated panel clearance, wall/glove/magazine
  occlusion, readability and frame timing: PENDING physical/session validation.
- Optional E99, supplies and energy are NOT accepted from this session. The log
  only validated health 150/150; HUD/TMD source pointers were unavailable. Missing
  fields remain hidden. Production replacement of the legacy health HUD remains NO-GO.

Extra cached surfaces at 5640x2876 cost about 124 MiB (one D24S8 + one A8R8G8B8).
No FPS or GPU-time gain is claimed. The normal OFF path does not allocate them.
This is a controlled occlusion candidate, not physical PASS or release promotion.

## One physical acceptance

Keep `EquipmentWristPreviewNoDepth=1` and `EquipmentWristSuppressHealth=0` for safety.
Raise/rotate the wrist: check the new text direction and that the panel rests outside
the cuff in both eyes. Move it behind a wall and the right weapon, then carry a
fresh magazine over the panel: each nearer surface must hide the panel. Check
Grip/reload/TMD transitions once and compare frame timing. Close normally;
inspect split/submission/Z-state receipts. Never enable health suppression before
the final visual overlap/live-data gates pass. MAIN alone deploys; no automatic timer.
