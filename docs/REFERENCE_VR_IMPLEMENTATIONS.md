# Reference VR Implementations: Dishonored-VR + KHARVOX → SingularityVR

Research branch: `research/dishonored-kharvox-integration` (never merge to
main from here; the main agent cherry-picks). Reference checkouts live
OUTSIDE this tree at `C:\Users\Trilex\vr-mods\references\` and are not
vendored here.

## Reference revisions (reproducible)

| Project | URL | Commit SHA | Branch/Tag | License |
|---|---|---|---|---|
| Dishonored-VR | https://github.com/VR-Stereo-Hub/Dishonored-VR.git | `7c1cb8a323e889220d33e62dbb7ed2b64ec765a5` | `VR-Main` / `v1.0.3` | zlib/libpng (DXVK-derived; `LICENSE`: "VR modifications Copyright (c) 2026 GingasVR … This is not the original DXVK software") |
| KHARVOX | https://github.com/CactusVRStudios/KHARVOX.git | `ff348bbd62204067fe6e16abc99be2526cd08dc0` | `main` / `v1.1.1` | MIT (C) 2026 CactusVRStudios (`LICENSE.txt`) |
| PSVR2Toolkit (KHARVOX submodule `tools/PSVR2Toolkit`) | https://github.com/BnuuySolutions/PSVR2Toolkit.git | `339f2eb4dff01f05bae1a3ae5b8629a345507e2b` (expected; submodule NOT populated in our checkout — `git submodule status` shows `-339f2eb…`) | `main` | MIT (C) 2026 Bnuuy Solutions (`third-party/psvr2-toolkit/LICENSE`) |
| PSVR2Toolkit loader binary | vendored in KHARVOX | `third-party/psvr2-toolkit/psvr2_toolkit_capi_loader.dll`, PE Machine `0x8664` (x64-only) | — | MIT, official binary; fetch/redistribute per upstream terms |

Submodule checkouts used: Dishonored-VR `third_party/OpenXR-SDK @ 5267613e`,
`third_party/imgui @ 8936b58f`.

Licensing posture: KHARVOX original code is MIT → logic may be reimplemented
with notice preserved (done: see headers + `THIRD-PARTY-NOTICES.txt`).
Dishonored-VR is zlib/libpng → equally permissive, but nothing was copied;
all Dishonored-derived work here is clean-room concept/reference.
PSVR2Toolkit CAPI headers/DLL internals and bHaptics SDK/player assets were
NOT copied (bHaptics is proprietary, reference-only).

---

## P0-1. PSVR2 adaptive triggers (KHARVOX)

Exact integration (`src/psvr2/`, `launcher/KharvoxLauncher/`):

- Game process NEVER touches Sense hardware. `Psvr2IpcClient.cpp:143
  KharvoxPsvr2IpcStart()` (called from `OpenXRBootstrap.cpp:689-691`,
  gated on `KHARVOX_USE_PSVR2_TOOLKIT=1`) spawns a worker thread
  (`CreateThread`, `:189`) that opens a write-only overlapped named pipe
  (`\\.\\pipe\\KharvoxPsvr2_<pid>_<guid>`, connect loop `:89-101`).
- The x64-only `KharvoxPsvr2Bridge.exe` (`Psvr2BridgeMain.cpp:370 wmain`)
  owns the pipe server (current-user SID ACL, `PIPE_REJECT_REMOTE_CLIENTS`),
  loads `psvr2_toolkit_capi_loader.dll` from its own directory
  (`Psvr2ToolkitBackend.cpp:63-66`), and translates messages via
  `psvr2_toolkit_set_trigger_effect(hand, ScePadTriggerEffectCommand)`
  (`Psvr2ToolkitBackend.h:53-54`, `.cpp:137-193`).
- Protocol: `Psvr2IpcProtocol.h` — magic `0x3250484B` ("KHP2" LE, `:15`),
  version 3 (`:16`), `Hello=1 / TriggerState=2 / Shutdown=3`, 21-byte
  `TriggerStatePayload`, packed-uint64 dedup codec
  (`packTriggerCommand:265-301`), constant-time token compare, monotonic
  sequence enforcement. Lifetime: launcher `Psvr2BridgeSession.cs:63-139`
  (`TryStart` → env handoff `KHARVOX_PSVR2_PIPE_NAME/_SESSION_TOKEN/_PARENT_PID`
  → `StopAsync`); game `KharvoxXRShutdownHaptics` (`OpenXRBootstrap.cpp:3305`)
  and session-loss paths (`:2916-2926, :2968-2969`) force Off.
- x86 answer: KHARVOX is x64-only (`README.md:199`, `CMake -A x64`,
  launcher `PlatformTarget=x64`). The 32-bit problem is solved by the
  OUT-OF-PROCESS bridge — the game links only the IPC client, never the CAPI
  headers (`Psvr2ToolkitBackend.h:9` is bridge-only). A 32-bit D3D9 mod reuses
  the same x64 bridge exe + protocol.
- Effect API supports `OFF / WEAPON / VIBRATION / FEEDBACK / SLOPE_FEEDBACK /
  MULTIPLE_POSITION_{FEEDBACK,VIBRATION}` with 10-point curves
  (`Psvr2ToolkitBackend.cpp:142-180`).
- Weapon table (`Psvr2TriggerPolicy.h:188-253`, `selectTriggerCommand`):
  Pistol `weapon(4,6,3)`; PlasmaRifle `feedback(4,3)` + fire
  `vibration(4,3,60)`; HAR/Assault/Arc `weapon(3,5,5)` + fire
  `vibration(3,7,40)`; Chaingun spin-up profile `{6,25}→{7,35}→{8,45}→{8,55}`
  (`:90-99`); Gauss `slope(2,8,2,7)`; Shotgun/SSG/Rocket `weapon(2,4,7)`;
  BFG `slope(2,8,3,8)`; Chainsaw fire-only `multiVib(45, …)`;
  Fists/Unknown → Off. Gating: Disabled → UnsupportedRuntime → Unfocused →
  !gameplay → ammo Empty/Unavailable blocks fire vibration only.
  Hand is logical (`leftHanded ? Left : Right`); support hand stays Off.
- Haptics are THREE disjoint paths: generic OpenXR `xrApplyHapticFeedback`
  (`OpenXRBootstrap.cpp:825-913`, policy `XInputHapticsPolicy.h`: 100 ms
  pulse, 75 ms refresh, 0.45/160 Hz fire fallback, unspecified-frequency
  retry); Sense trigger vibration (resistance motor, independent); bHaptics
  vest mirroring XInput energy. Do NOT replace our `xrApplyHapticFeedback`
  path — add Sense triggers as a parallel backend.

Singularity prototype: `spikes/view_matrix/psvr2_trigger.h` — `ProfileFor()`
(PISTOL/RIFLE/SHOTGUN mirror KHARVOX ranges; SNIPER provisional heavy wall,
NEEDS HEADSET VALIDATION; TMD/OTHER Off), `Select()` gating in KHARVOX order,
`Pack()` dedup codec, `Runner` event API
(`SetWeapon/SetFireDown/SetReloading/SetAmmoOk/ClearTrigger`), `IBackend`
seam + `NullBackend` graceful fallback. Probe:
`tools/reference-tests/psvr2_trigger_probe.cpp` (logic-only, passive
`WaitNamedPipeW` presence check, never actuates).

## P0-2. Dishonored menu architecture

Dishonored captures UI to textures and reprojects as world/head-relative
quads + dedicated per-element composition quads. No in-engine stereo fix.

- Classification: `src/core/gfx/hud_class.h:1-33` — Scaleform draws on the
  backbuffer at frame tail; 4-term rule (rt0==backbuffer, full viewport at
  origin, depth off, blending on) + vertex-shader `c6..c9` region probe.
- Redirect: `src/core/gfx/hud_capture.h:1-28,79-89` + `hud_capture.cpp:28-92`
  — recognised draws are rebound to private `A8R8G8B8` sink targets;
  copy→clear→deliver every present (previous slot, never just-written),
  `alpha=max(r,g,b)` premultiplied repair, per-slot swapchains sized to the
  crop (`openxr_runtime.cpp:5616-5635`), submitted as
  `XrCompositionLayerQuad`. Gate is game state, never draw shape.
- Layout: `src/core/gfx/hud_layout.h:39-53,151-160` (`Anchor{Off,Frame,
  Window,World,HandL,HandR}`, `kMaxSinks=12`) + `hud_layout.cpp:1044
  provide()` → `openxr_runtime.cpp:5486` — per-element isolated quads,
  `place()` `:989-1040`, `expand_reference_panel()` (pixel-stable growth),
  `stereo_menu_hold.h:3-10` 150 ms mono hold across menu gaps.
- Placement math (pure, host-tested): `src/core/vr/hud_anchor.h:30-90`
  (`camera_panel_position`, `wrist_position`, `follow_grip_orientation`,
  `too_near/behind_face/billboard_degenerate` guards, `crop_rect` never
  stretches); `mono_anchor.h:10-26` yaw-only upright LOCAL parking.

Recommended Singularity approach: KEEP our `HudStereoPair` + ADD the
sink/provider split — port the 4-term classification rule, the
private-target + copy-after-clear + previous-slot delivery discipline, and
the `hud_anchor` pure-math placement (our `ui_pointer.h HeadPlane` already
implements the head-locked panel half of this). This yields ONE authoritative
menu at a comfortable distance, identical in both eyes, without touching
engine stereo. Our `test-vr-interaction.cpp` menu round-trip already proves
visual-hit == click-pixel for our remap; the missing piece is Dishonored's
capture-to-quad delivery, which is REFERENCE ONLY until mapped to our GFx
draw seam.

## P0-3. Dishonored controller→UI mapping

No ray→UI-plane→game-coords injection for in-world Scaleform. Two paths:

- F10 overlay (ImGui eye-texture panel): `src/core/ui/overlay.cpp:54-160
  OvlInjectControllerPointer` — aim-quat forward → head-conjugate rotate →
  symmetric-tangent NDC → `io.AddMousePosEvent` between Win32 and ImGui
  `NewFrame`; trigger → mouse button, stick-Y → scroll.
- Gameplay aim: `src/game/dishonored/aim_ray.cpp:104,118,133-160` publishes
  ONE `FireFrame` ray; `fire_aim_math.h:19-56 solve()` converts the XR ray
  into the camera frame and hands engine the origin/direction — the game
  still traces/validates/highlights. Reticle rides the aim dot
  (`hud_layout.cpp:995-1009` LocalBillboard).

Recommended: our `ui_pointer.h Intersect` + `TriggerGate` already implements
the Dishonored-equivalent chain (ray → plane → logical canvas → pixel, edge
click, context/focus suppression). Adopt Dishonored's "game traces, VR hands
it the ray" principle: never synthesize cursor motion the game can disagree
with — the SINGLE remap in `test-vr-interaction.cpp` must stay the one
authority for visual-hit == click-pixel.

## P0-4. Dishonored camera stabilization

- Head vs game: `head_track.cpp:881+ ApplyHeadToViewRotation` — yaw
  RELATIVE (`dy` clamped ±0.5 rad, `:1014-1021`), pitch absolute, 2 ms
  chain-stamp grouping, menu/cine suspend.
- Translation: `camera.cpp:712-744` — under projection uses a YAW-ONLY frame
  (else true basis); eye offset `eye*ipd/2*scale` along the measured right
  row (`:640,666-667,776-807`).
- Shake/bob REMOVED, not cancelled: `cam_shake.cpp:1-61` holds
  Shake/Recoil/HitReact/PhysicalReact/Rumble + `m_BobAmount/m_RollAmount` +
  master `m_fReactionWeight` at zero (instrumented with a `fought` counter);
  `game_opts.cpp:667,986` forces head-bob 0 at launch. Residual 1.5 uu walk /
  0.4 uu idle stays in `PlayerControl` (untouched, VR-175).

Recommended: copy the removal-not-cancellation strategy — our
`camera_stabilization.h ComputeVrCameraStabilization` (pawn-minus-POV
residual replacement) is the same idea one level up; extend it with
Dishonored's per-influence hold table once the Singularity equivalents of
`m_BobAmount`/reaction weights are identified in the frame (REFERENCE ONLY
until those addresses/fields are found). Adopt the yaw-only-basis-under-VR
rule wherever we build view frames.

## P0-5. Dishonored test runtime (IMPLEMENTED for Singularity)

- `src/tools/xrsim/xrsim_*.cpp`: 32-bit mock OpenXR runtime
  (`XR_RUNTIME_JSON`), scriptable head/hand motion, `TESTING.md:36-42`
  scripts (`mono.xrs / smoke.xrs / headlook.xrs / stereo.xrs /
  world-6dof.xrs`), `eye-check.ps1`, `dump frame/capture/eye_*_mono`.
- `pose_record.h/.cpp:1-80`: CAMERA vs RENDER vs SUBMISSION ring (128),
  `note_render_pos/note_render_vp`, `CTRL_YAW/OLD_REC/WRONG_EYE` checks.
- Host tests: `frame_test/main.cpp` + `hand_frame_test.h /
  weapon_frame_test.h`, `tools/hud-anchor-tests.cpp`, `postest/pitchtest/
  eyetest` instruments.

Implemented: `spikes/view_matrix/fake_xr.h` (`Rig`: seeded closed-form
motion, 90 Hz stepping, `DropTracking`, `NudgeController`, exact replay) —
covers FakeHmdPose/FakeLeftControllerPose/FakeRightControllerPose +
deterministic frame stepping for hand transforms, menu projection, laser
intersection, recenter and arm IK, no headset required.

## P1. Hands / IK

- Dishonored: NO two-bone IK. Bone-matrix rewrite + procedural fallback
  (`hand_mesh.cpp:73-83` `HmBuildYPR`, trim `R=Ry*Rx*Rz`); arm/hand mesh
  SPLIT from one skinned tri-list (`ARM_HAND_SPLIT.md:119-150`,
  `mesh_split.cpp`); open-hand mirror `P'[finger_R]=P[wrist_R]*X*
  inv(P[wrist_L])*P[finger_L]*X` (`HAND_POSE.md:37-51`); engine
  `SkelControlLimb`/CCD surveyed, not used.
- KHARVOX: NO smoothing/prediction/velocity (`AerWeaponSource.h:174`,
  `BodyPoseRebase.h:5`); controller→game grip map
  `grip={-pos.z*s, -pos.x*s, pos.y*s}`, `s=39.3701` units/m
  (`OpenXRBootstrap.cpp:1983-1985`); hand calibration
  `hand_models_calibration_default.cfg` + `HandCalibrationPolicy.h:34-43`;
  two-hand profiles `two_hand_weapon_profiles_default.cfg`
  (all `captureRadius=0.20 m`, pistol ABSENT → one-hand fallback),
  barrel-mode aim solve vs side-grip roll-only
  (`solveTwoHandOrientation:1674-1704`), virtual stock only for Barrel +
  head-tracked (`:1959-1967`, `aimOrigin=headPos-0.18m`), grip thresholds
  press >0.55 / release ≤0.55 (`:1713-1750`).

Singularity: our `arm_ik.h Solve()` two-bone solver with parallel-transport
bend persistence already exceeds both references (10 cm → ~10 cm proven in
`test-reference-integration.cpp`). Keep it; adopt KHARVOX's conventions:
pistol = one-hand/authored pair (matches our `support_grip.h AuthoredPair`
for 5799 verts), long guns = weapon-local support point + 0.20 m capture
radius + release hysteresis (our `SupportTarget` acquire/release radii
already model this — validate 0.15/0.30 defaults against 0.20 m in-headset).

## P1. Recenter (IMPLEMENTED)

Dishonored has NO single HMD+controller transform — three lanes:
runtime LOCAL origin (`openxr_runtime.cpp:2624-2633`, generation watch),
game yaw/pos reference (`head_track.cpp:58-62,1455-1477,1571-1577`,
`yaw_book.h`, both-stick chord tap<350 ms = panel / hold 600 ms = recenter
in `openxr_input.cpp:557-582`), panel re-parking
(`recenter_mono_anchor`, `mono_anchor.h:14-24`).

Implemented: `spikes/view_matrix/recenter.h` — ONE yaw-only stage correction
(`RecenterForHead` + `Apply`) carrying HMD + both controllers rigidly
(relative geometry exactly preserved, height untouched, floor stays floor),
`YawOf`/`Forward` inverses. Tests prove head-to-origin, rigid invariance and
idempotence. OPTIONS short-press vs hold disambiguation and focus/session
gating belong at the input layer (Dishonored chord timings are the reference:
tap <350 ms, hold ≥600 ms).

## P1. Shadows

- Dishonored: NO stereo shadow-map handling. "Shadow" = managed-texture
  shadow copies (`d3d9ex.h:26-40`), the `vs_const` HUD-probe shadow
  (`frame_hooks.cpp:592-596`), and exclusion of <512 targets / shadow uploads
  from jitter/capture (`depth_probe.cpp:558`, `dlss_jitter.h:9`,
  `vs_const_hook.cpp:520`). First-person shadows ride the second eye draw
  under reentry natively.
- KHARVOX: scene-depth-integrated beam only; nothing on shadow maps.

Recommended: our `shadow_policy.h` (exact 512×512 R32F classification +
`SuppressVrShadow`) is already the correct narrow approach. Do NOT build
per-eye shadow-matrix reconstruction — neither reference needed it; if our
asymmetric shadows persist, the fix is suppressing the first-person shadow
draw (already classified), not re-rendering shadow maps.

## P2. KHARVOX laser / haptics / stock / render scale

- Laser: muzzle-anchored 8-sided cylinder (r=2 mm, 20 m,
  `HandRenderer.cpp:670-687`), world conversion
  `trackingOrigin=tracking.pos+R*(-local.y/s, local.z/s, -local.x/s)`
  (`WeaponHook.cpp:1559-1574`), shared scene depth with reverse-Z awareness,
  BEAM WITHHELD when depth unavailable (`OpenXRBootstrap.cpp:4726-4730`) —
  adopt the never-render-a-lying-laser rule for our menu laser.
- Haptics: keep OpenXR generic; Sense = triggers only (see P0-1).
- Virtual stock: barrel-mode + head-tracked only, `headPos-0.18 m` — adopt
  as the stock experiment starting point (NEEDS HEADSET VALIDATION).
- Render scale: `Fsr1Policy.h:10-47` (`max(scale,0.5)`, no silent ceiling,
  FSR as optional post-process on cached per-eye images) — architecture
  lesson for the D3D11/NIS branch only; NOT implemented here.

## Coordinate comparison

- Dishonored XR: right +X, up +Y, forward −Z, metres, xyzw quats
  (`xr_math.h:1-64`, `quat_mul(a,b)` = a×b, b-first); UE game: X forward, Y
  right, Z up, cm→uu, rotators 65536/360°, `c5 = −worldPos`, row-major
  `p'=p*A`, VP row/col-vector validated at runtime (`pose_record.cpp:62-66`).
- KHARVOX: OpenXR right-handed Y-up metres/quats; DOOM Z-up world units,
  row-basis `origin+local*axis`, `s=39.3701` units/m, grip
  `{-z*s, -x*s, y*s}`, yaw-only baselines.
- Singularity: UE3 X-forward/Y-right/Z-up uu (same family as Dishonored's
  game side); `armik`/`uipointer`/`vrrecenter` headers share one space per
  header — never move matrix math between spaces without adapting the basis
  (the recenter test failure fixed during development was exactly such a
  sign-convention bug: `Apply` now satisfies `R(a)·Forward(0)=Forward(a)`).

## Code prepared (this branch)

| File | Purpose | Status | Commit |
|---|---|---|---|
| `spikes/view_matrix/psvr2_trigger.h` | Adaptive-trigger profiles/gating/dedup/event API + backend seam | READY TO CHERRY-PICK (pure logic; SNIPER values provisional) | (this branch) |
| `spikes/view_matrix/fake_xr.h` | Deterministic fake HMD/left/right poses + frame stepping | READY TO CHERRY-PICK | (this branch) |
| `spikes/view_matrix/recenter.h` | One-transform yaw recenter for HMD + both controllers | READY TO CHERRY-PICK | (this branch) |
| `spikes/view_matrix/tests/test-reference-integration.cpp` | Tests: trigger, runner, fake_xr, recenter, arm_ik properties | READY TO CHERRY-PICK (PASS, `/W4` clean) | (this branch) |
| `tools/reference-tests/psvr2_trigger_probe.cpp` | Standalone x86-safe probe (logic + passive pipe check, never actuates) | NEEDS HEADSET VALIDATION (build verified by test compile path; run on dev PC) | (this branch) |
| `docs/REFERENCE_VR_IMPLEMENTATIONS.md` | This map | READY TO CHERRY-PICK | (this branch) |

## Tests

`spikes/view_matrix/tests/test-reference-integration.cpp` — build with the
VS 18 x64 toolchain, `/W4` zero warnings:

```
trigger: profiles, fire override, ammo gate, gating order
trigger: runner events/dedup/teardown, 6 backend sends
fake_xr: 300-frame determinism, tracking loss, nudge
recenter: head-to-origin, rigid controller invariance, idempotence
arm_ik: 10cm wrist tracking, clamp at full extension, elbow straightens
PASS
```

Existing `test-vr-interaction.cpp` (menu round-trip, support grip, shadow)
unaffected — new headers are additive; no production path touched.

## Main-agent handoff

READY TO CHERRY-PICK (no production dependency, isolated headers/tests):
- `psvr2_trigger.h` — wire `Runner` to weapon/fire/reload events + a future
  x64 bridge implementing `IBackend`; right trigger only. Integration point:
  wherever `xrApplyHapticFeedback` fire events originate.
- `fake_xr.h` — drive `ui_pointer.h`/`arm_ik.h`/`recenter.h` inputs in host
  tests; next step is a `d3d9.cpp`-adjacent pose-source switch (not done
  here, deliberately).
- `recenter.h` — use `RecenterForHead` output as the single stage correction
  for HMD + both controllers on OPTIONS-hold (≥600 ms, cf. Dishonored chord).
- `test-reference-integration.cpp` — extend with menu/laser cases over
  `fake_xr.h` frames.

REFERENCE ONLY (do not copy code; concepts mapped above):
- Dishonored sink/provider HUD capture (`hud_class`/`hud_capture`/
  `hud_layout`/`hud_anchor`/`mono_anchor`/`stereo_menu_hold`), `cam_shake`
  hold-table, `YawBook`, `xrsim` scripts, `pose_record` triple-split.
- KHARVOX `Psvr2IpcProtocol.h` wire format (adopt if building the x64
  bridge; note launcher C# writes v1 vs C++ v3 — standardise on v3),
  two-hand `captureRadius=0.20 m`, virtual-stock `headPos-0.18 m`,
  chaingun spin-up / BFG slope curves for future profiles.

NEEDS HEADSET VALIDATION:
- SNIPER `weapon(2,5,6)` starting point; TMD profile (currently Off).
- Support-grip radii (0.15 acquire / 0.30 release vs KHARVOX 0.20) and
  long-gun fore-end offsets in `support_grip.h`.
- Virtual stock adoption; menu-at-distance comfort values.
- `psvr2_trigger_probe.cpp` run against a real PSVR2 + x64 bridge
  (mild resistance only; never continuous actuation).

## Status

REFERENCE INTEGRATION READY — reusable code/tests (`psvr2_trigger.h`,
`fake_xr.h`, `recenter.h`, `test-reference-integration.cpp`, probe) plus
this handoff exist on `research/dishonored-kharvox-integration`. No gameplay
branch touched, no `d3d9.dll`/`.ini` deployed, no main merge.
