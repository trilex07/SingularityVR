# Diegetic left-wrist HUD SIDE handoff

## Scope and state

Branch `feature/diegetic-left-wrist-hud` is an **offline candidate**, default OFF. No Steam installation, game run, OpenXR session, or PSVR2 acceptance occurred on SIDE. Production drawing now fails closed until per-eye scene-depth occlusion is validated; `EquipmentWristPreviewNoDepth=1` is an explicit unoccluded inspection mode and cannot suppress the legacy health plate.

The renderer is implemented in `spikes/view_matrix/d3d9.cpp`. It draws one compact panel in both SBS eyes using `VrUiProjectEye`, the actual physical-left `l_arm_wrist` skin palette, and the existing controller ride/UE-to-tracking map. The no-TMD bind comes from `bindassets::noTmd`; the TMD bind comes from `armnatural::TmdBind().bind[9]`. A pose is published only after a successful stereo arm draw and must match the current Pawn and be younger than 150 ms. Controller tracking must also be younger than 150 ms. It does not modify finger, grip, weapon, or ammo logic. The new panel is hidden during menus, pause, cinematics, support grip, tracking loss, failed projection, or an invalid data sample.

The 5x7 technical glyph face in `wrist_equipment_art.h` is newly authored code art. The panel uses dark steel, amber trim, red health, cyan E99, and small fasteners. The fixture generator `tools/hud/render_wrist_equipment_fixture.py` creates a **synthetic** stereo layout image; its values are examples, not game evidence. PL/EN labels are selected with `EquipmentWristPolish`.

## Live producer mapping

| Display | Source | Gate |
| --- | --- | --- |
| HP/current and maximum | Existing `g_playerPawn` + reflected `Health`/`HealthMax`, decoded by `healthbar::DecodeLiveHealth` | Valid floats, current Pawn |
| E99 Tech | `RvGameSharedHUD.mE99TechCount` (IntProperty) | Reflected offset, readable value, range 0..1,000,000 |
| Health Packs | `RvGameSharedHUD.mHealthPackCount` (IntProperty) | Same independent validation |
| E99 Vials | `RvGameSharedHUD.mE99VialCount` (IntProperty) | Same independent validation |
| TMD energy | Local Pawn's `RvTMD.mTMDEnergy` (FloatProperty), plus exported `RvTMD.GetTMDEnergyMax` function | Current must be readable from that TMD; maximum must be observed from a natural getter return on the same TMD within 1.5 s. Otherwise hidden. Runtime invocation remains unverified. |

The property and function identities were checked against the private decompressed `RvGame` export inventory at `C:\Users\Trilex\vr-mods\research\native-hands-20261007\rvgame-export-inventory-hud.txt`. That inventory is intentionally not committed. Offline export metadata establishes names and types; it does not prove collection/spend update timing in a running game.

`RvPlayerController.sharedHUD` is an exported ObjectProperty. The XInput-side cache requires the same controller's `Pawn` to equal the current local Pawn, resolves all four HUD properties on that owned HUD class, and validates class/pointer/freshness on consumption. The local Pawn's `mTMD` owns the energy current source. The render path performs only bounded reads and never calls `PropOffsetInChain` or walks GObjects. The exported `GetTMDEnergyMax.ReturnValue` is a FloatProperty; the export alone does not establish native versus script implementation, calling convention, or natural call frequency. The hook only observes an existing return and never invokes the method. `RvPlayerPawn.SpendEnergy`, `RegenEnergy`, `UpdateEnergyDisplay`, `FinishE99Vial`, `UseHealthPack`, and `RvGameSharedHUD.PostHUDUpdates` are named in exports; their authoritative update ordering requires a bounded runtime trace. No offset or collection semantics is inferred from a plausible count.

## Depth and placement

The scene has a documented foreground depth clear at the start of the first-person pass. Consequently the bound Present-time depth surface cannot be assumed to retain wall depth. A deterministic two-eye fixture verifies that a wall at 0.3 depth hides a panel at 0.5, while clearing depth to 1 exposes it. The production gate therefore requires explicit per-eye world-depth evidence, which this SIDE branch cannot yet supply. Enabling `ZENABLE` at Present is insufficient. The smallest justified integration is a read-only witness of scene/foreground depth-surface identity, format, clear ordering and per-eye viewport/scissor, followed by a depth-aware draw at a proven scene stage or a compatible copied world-depth surface. A D3D9 depth copy must occur outside `BeginScene`/`EndScene` and satisfy format, size and non-discardable restrictions. This needs MAIN live evidence before enabling production drawing.

Source bind geometry gives wrist-to-elbow 21.4858837 UU / 52.5 = 409.3 mm. The panel's 155 mm axis now runs toward the elbow (wrist-local -Y), spans about 2.5..157.5 mm from the wrist with the -80 mm centre offset, and its 104 mm width runs across local Z. The front normal uses local -X. The signed inward face and glove/TMD clearance still require headset inspection. Scale, offsets and rotations remain configurable.

The existing weapon-mounted clip/reserve ammo plate is unchanged. Native GFx is not globally hidden. The legacy VR wrist/health plate is suppressed only when the new panel actually drew **both eyes with valid health** on this frame and `EquipmentWristSuppressHealth=1`. All switches default OFF.

## Configuration (`[Render]`)

```ini
EquipmentWristHud=0
EquipmentWristPreviewNoDepth=0
EquipmentWristEnergy=0
EquipmentWristTech=0
EquipmentWristSupplies=0
EquipmentWristSuppressHealth=0
EquipmentWristPolish=0
EquipmentWristNormalSign=1
EquipmentWristScalePct=100
EquipmentWristOffsetXmm=-25
EquipmentWristOffsetYmm=-80
EquipmentWristOffsetZmm=0
EquipmentWristYawDeg=0
EquipmentWristPitchDeg=0
EquipmentWristRollDeg=0
```

`NormalSign=-1` flips the inner-face visibility test. Offsets are wrist-local millimetres; angles are wrist-local degrees. Scale is clamped to 60..150%, offsets to ±80 mm, and angles to ±90°. These values are calibration controls, not verified comfort settings.

## Offline validation

- Optimized x86 PE32 `spikes/view_matrix/build.ps1`: PASS.
- `tools/tests/test-hud-live.ps1`: PASS, including HP full/half/zero/invalid, independent count policy, stale pose, source lifecycle, two-eye depth clear, wrist placement, visibility hysteresis, and synthetic stereo projection/rotation.
- `tools/hud/render_wrist_equipment_fixture.py`: PASS as a synthetic SBS layout receipt.
- Synthetic class-change microbenchmark (`test-wrist-equipment-source.cpp`, optimized build): approximately 370 ns per simulated reflection lookup versus 0.6 ns for a cached identity check. This isolates the changed code path; it is not an end-to-end frame-time measurement. The renderer also builds invariant normalized glyph/quad geometry once per frame and projects the same list for both eyes, with fixed-capacity thread-local buffers and no frame allocation.
- PR #70 coexistence: a disposable merge with PR #70 head `83e00755` required two manual joins in `d3d9.cpp` at `ProcessEvent` (function-name gate and post-return observers). The joined source passed the optimized x86 build, HUD test suite and `test-independent-hands-shot-haptics.ps1`. This proves offline coexistence after resolution; the two branches do not auto-merge. PR #73 does not incorporate PR #70's source.
- Runtime HUD field updates, natural TMD maximum getter frequency and return ABI, actual native wrist inward normal, glove/TMD clipping, scene-depth occlusion, GPU/CPU frame cost, Steam/OpenXR/PSVR2 comfort: **NOT RUN / BLOCKED on SIDE**.

## MAIN integration gates

1. Verify the `RvTMD.GetTMDEnergyMax` natural return witness and fresh current/max during depletion and recharge before enabling the energy row. If the game does not invoke the getter at least every 1.5 s, the row correctly stays hidden and needs a separately validated read path.
2. Verify `mE99TechCount`, `mHealthPackCount`, and `mE99VialCount` while collecting/spending/consuming; confirm the sampled HUD instance belongs to the local player.
3. Check actual inner-wrist facing and calibrate orientation/offset with the headset; inspect both eyes, tracking loss, TMD, glove, support grip, reload and weapon switching.
4. Identify a compatible world-depth source or earlier depth-aware draw point for both eyes before enabling production drawing. Measure panel frame time under live load. The optional unoccluded preview is for inspection only.
5. Only after the panel reliably renders both eyes, opt into health suppression. Native GFx element-level suppression still needs a separate verified producer and replacement contract.

No native deployment or Steam test was performed by SIDE.

## Consolidated MAIN PSVR2 acceptance

1. With a backed-up accepted installation, enable the preview only and check both eyes, inner forearm fit, glove/TMD clearance, support grip, menus, pause, tracking loss and weapon switching. Keep health suppression OFF. Measure frame time against the accepted build.
2. Capture a bounded depth trace at the world-to-foreground transition: per-eye surface identity/format, clear order, viewport/scissor, and a wall/panel overlap. Integrate a proven occluded draw path, run the two-eye wall/weapon occlusion scene, then enable production mode. Never accept a through-wall panel.
3. During actual pickup/use and TMD depletion/recharge, capture local controller, Pawn, sharedHUD and TMD identities and the four HUD values; record natural `GetTMDEnergyMax` calls/return timing. Enable optional rows only when their producers are confirmed. Finally enable health suppression after the two-eye draw succeeds, and restore the accepted installation on failure.
