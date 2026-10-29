# Diegetic left-wrist HUD SIDE handoff

## Scope and state

Branch `feature/diegetic-left-wrist-hud` is an **offline candidate**, default OFF. No Steam installation, game run, OpenXR session, or PSVR2 acceptance occurred on SIDE. It must not be called a complete accepted replacement yet.

The renderer is implemented in `spikes/view_matrix/d3d9.cpp`. It draws one compact panel in both SBS eyes using `VrUiProjectEye`, the actual physical-left `l_arm_wrist` skin palette, and the existing controller ride/UE-to-tracking map. The no-TMD bind comes from `bindassets::noTmd`; the TMD bind comes from `armnatural::TmdBind().bind[9]`. A pose is published only after a successful stereo arm draw and must match the current Pawn and be younger than 150 ms. Controller tracking must also be younger than 150 ms. It does not modify finger, grip, weapon, or ammo logic. The new panel is hidden during menus, pause, cinematics, support grip, tracking loss, failed projection, or an invalid data sample.

The 5x7 technical glyph face in `wrist_equipment_art.h` is newly authored code art. The panel uses dark steel, amber trim, red health, cyan E99, and small fasteners. The fixture generator `tools/hud/render_wrist_equipment_fixture.py` creates a **synthetic** stereo layout image; its values are examples, not game evidence. PL/EN labels are selected with `EquipmentWristPolish`.

## Live producer mapping

| Display | Source | Gate |
| --- | --- | --- |
| HP/current and maximum | Existing `g_playerPawn` + reflected `Health`/`HealthMax`, decoded by `healthbar::DecodeLiveHealth` | Valid floats, current Pawn |
| E99 Tech | `RvGameSharedHUD.mE99TechCount` (IntProperty) | Reflected offset, readable value, range 0..1,000,000 |
| Health Packs | `RvGameSharedHUD.mHealthPackCount` (IntProperty) | Same independent validation |
| E99 Vials | `RvGameSharedHUD.mE99VialCount` (IntProperty) | Same independent validation |
| TMD energy | `RvGameSharedHUD.mTMDEnergy` (FloatProperty), plus exported `RvTMD.GetTMDEnergyMax` function | Current must be readable; maximum must be observed from a natural getter return on the local Pawn's TMD within 1.5 s. Otherwise hidden. Runtime invocation remains unverified. |

The property and function identities were checked against the private decompressed `RvGame` export inventory at `C:\Users\Trilex\vr-mods\research\native-hands-20261007\rvgame-export-inventory-hud.txt`. That inventory is intentionally not committed. Offline export metadata establishes names and types; it does not prove collection/spend update timing in a running game.

The existing weapon-mounted clip/reserve ammo plate is unchanged. Native GFx is not globally hidden. The legacy VR wrist/health plate is suppressed only when the new panel actually drew **both eyes with valid health** on this frame and `EquipmentWristSuppressHealth=1`. All switches default OFF.

## Configuration (`[Render]`)

```ini
EquipmentWristHud=0
EquipmentWristEnergy=0
EquipmentWristTech=0
EquipmentWristSupplies=0
EquipmentWristSuppressHealth=0
EquipmentWristPolish=0
EquipmentWristNormalSign=1
EquipmentWristScalePct=100
EquipmentWristOffsetXmm=0
EquipmentWristOffsetYmm=0
EquipmentWristOffsetZmm=0
EquipmentWristYawDeg=0
EquipmentWristPitchDeg=0
EquipmentWristRollDeg=0
```

`NormalSign=-1` flips the inner-face visibility test. Offsets are wrist-local millimetres; angles are wrist-local degrees. Scale is clamped to 60..150%, offsets to ±80 mm, and angles to ±90°. These values are calibration controls, not verified comfort settings.

## Offline validation

- Optimized x86 PE32 `spikes/view_matrix/build.ps1`: PASS.
- `tools/tests/test-hud-live.ps1`: PASS, including HP full/half/zero/invalid, independent count policy, stale pose, visibility hysteresis, and synthetic stereo projection/rotation.
- `tools/hud/render_wrist_equipment_fixture.py`: PASS as a synthetic SBS layout receipt.
- Runtime HUD field updates, natural TMD maximum getter frequency and return ABI, actual native wrist inward normal, glove/TMD clipping, scene-depth occlusion, GPU/CPU frame cost, Steam/OpenXR/PSVR2 comfort: **NOT RUN / BLOCKED on SIDE**.

## MAIN integration gates

1. Verify the `RvTMD.GetTMDEnergyMax` natural return witness and fresh current/max during depletion and recharge before enabling the energy row. If the game does not invoke the getter at least every 1.5 s, the row correctly stays hidden and needs a separately validated read path.
2. Verify `mE99TechCount`, `mHealthPackCount`, and `mE99VialCount` while collecting/spending/consuming; confirm the sampled HUD instance belongs to the local player.
3. Check actual inner-wrist facing and calibrate orientation/offset with the headset; inspect both eyes, tracking loss, TMD, glove, support grip, reload and weapon switching.
4. Confirm panel scene-depth behavior and frame-time cost. The current Present overlay has no validated scene-depth occlusion; keep the renderer OFF if it appears through geometry.
5. Only after the panel reliably renders both eyes, opt into health suppression. Native GFx element-level suppression still needs a separate verified producer and replacement contract.

No native deployment or Steam test was performed by SIDE.
