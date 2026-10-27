# -*- coding: utf-8 -*-
"""Feature taxonomy for the SingularityVR gap analysis.

Single source of truth. Each entry:
  id, category, feature, cls (MUST/SHOULD/NICE/NA), prio (P0..P4),
  status, evidence, reference, difficulty, verification, deps, notes
"""

F = []

def add(cat, feat, cls, prio, status, ev, ref, diff, ver, deps, notes="", fid=None):
    F.append(dict(id=fid or (cat[:3].upper() + "-" + str(len(F) + 1).zfill(3)), category=cat,
                  feature=feat, classification=cls, priority=prio, current_status=status,
                  evidence=ev, reference=ref, difficulty=diff, verification=ver,
                  dependencies=deps, notes=notes))

# ---------------------------------------------------------------- HEAD / CAMERA
C = "HEAD/CAMERA"
add(C, "6DoF HMD tracking", "MUST", "P1", "L3_VERIFIED",
    ".vrrec 5573 samples: head_x/y/z/head_yaw/head_roll all move; head_y 1.114-1.271 m",
    "vrframework 09 (camera composition)", "MEDIUM",
    "L3 head pos+yaw+roll monotonic over a recording; L4 look-around stability")
add(C, "Roomscale translation", "MUST", "P1", "L3_VERIFIED",
    ".vrrec head_x spans -0.573..-0.364 m, head_z -0.570..-0.364 m (lean ~0.2 m)",
    "vrframework 09", "MEDIUM", "L3 translation range; L4 step sideways matches")
add(C, "Recenter (L3+R3 chord)", "MUST", "P1", "PHYSICALLY_VERIFIED",
    "Recentering implemented in UpdateFromHeadset; .vrrec has Recenter flag (1 sample)",
    "REFramework recenter_view", "LOW",
    "L3 Recenter flag fires; L4 view re-seats where you look")
add(C, "Recenter (hub row)", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
    "PR_RECENTERCHORD panel row exists (d3d9.cpp:6655, 6703)", "Dishonored-VR F5 + chord", "LOW",
    "L3 row toggles; L4 confirm same effect as chord")
add(C, "Standing mode", "MUST", "P1", "PARTIAL",
    "Keep-standing-position-on-transition landed (4c740cc). Standing reads 1.11-1.27 m in .vrrec",
    "vrframework 09 world_scale", "MEDIUM",
    "L3 head_y at deliberate recentre within 5 cm of true; L4 objects life-size")
add(C, "Seated mode", "SHOULD", "P2", "PARTIAL",
    "User tests from floor (0.55-0.58 m). Sub-1 m warning exists (02f5a60) but no auto elevation",
    "KHARVOX seated/standing presets", "MEDIUM",
    "L3 seated capture logs expected height; L4 reachable interaction")
add(C, "Height calibration", "SHOULD", "P2", "MISSING",
    "No runtime height offset key; only a log warning. Eye height is engine 70 UU (measured)",
    "KHARVOX launcher height; Dishonored height rows", "LOW",
    "L3 ini HeightOffsetMm moves view; L4 eye at true physical height")
add(C, "Floor calibration", "SHOULD", "P2", "MISSING",
    "No guardian/floor offset handling; runtime reported 0.55 m for a standing 176 cm user",
    "vrframework 09 standing origin", "MEDIUM",
    "L3 floor offset applied; L4 stand on flat, view level")
add(C, "IPD correctness", "MUST", "P1", "L3_VERIFIED",
    ".vrrec eyeDelta = 63-64 mm constant; user's real IPD is 64 mm. Runtime eye-pos delta used, no hardcode",
    "vrframework 09 (eye term from runtime)", "LOW",
    "L3 eyeDelta == device IPD; L4 depth comfortable")
add(C, "World scale", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
    "WorldScalePct shipped default changed 100 -> 80. Chain 52.5 UU/m at 100 %, engine eye 70 UU confirms UE3 units",
    "vrframework 09 world_scale knob", "LOW",
    "L3 1 m hand displacement == 1 m in world; L4 objects life-size")
add(C, "Body/head yaw relationship", "MUST", "P1", "L3_VERIFIED",
    "g_baseYaw from CAMERA with fold-in; yawSign/pitchSign plumbing; cutscene anchoring run 163",
    "vrframework 09 decoupled pitch", "MEDIUM",
    "L3 wantYaw tracks camera yaw; L4 turning body vs head feel right")
add(C, "Near plane", "MUST", "P1", "PARTIAL",
    "Engine near/far kept for depth range; VR FOV from runtime tangents. No fade for near geometry",
    "vrframework 08 (engine near/far, runtime FOV)", "MEDIUM",
    "L3 no z-fighting at 10 cm; L4 no clipping when leaning into walls")
add(C, "Far plane", "SHOULD", "P2", "PARTIAL",
    "Engine far plane retained; kFovHeadroom exists for culling (1.0 default)",
    "vrframework 08 far/near guard", "LOW",
    "L3 distant geometry stable; L4 horizon does not pop")
add(C, "FOV handling", "MUST", "P1", "L3_VERIFIED",
    "ApplyVrFov builds asymmetric-ish FOV from runtime g_eyeFov tangents * g_renderFovScale; logs 105x106 deg",
    "vrframework 08 (runtime frustum bounds)", "MEDIUM",
    "L3 forced FOV logged and readback stable; L4 world not stretched")
add(C, "Per-eye asymmetry (skew)", "SHOULD", "P2", "PARTIAL",
    "Same projection both eyes; separation lives in the view offset. Run 167 documented the choice",
    "vrframework 08 sec 3.2 skew terms", "HIGH",
    "L3 compare per-eye tangent sums; L4 depth cue comfortable")
add(C, "Camera shake", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
    "VrTranslationStabilization defaults ON (3059), removes pawn bob/shake residual",
    "vrframework 14 (comfort hazards)", "LOW",
    "L3 stabilization counter; L4 no nausea on landing")
add(C, "Camera bob / head bob", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
    "Same VrTranslationStabilization path (11407)", "vrframework 14", "LOW",
    "L3 offset residual small; L4 walk does not bounce view")
add(C, "Forced pitch", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
    "g_wantPitch clamp 16000 and pitchSign; pitch conjugated to view frame (run 125)",
    "vrframework 09", "MEDIUM", "L3 pitch range; L4 look up/down natural")
add(C, "Forced roll", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
    "HeadRoll row + rollSign, g_maxRollDeg report", "vrframework 09 (level horizon)", "LOW",
    "L3 roll tracking; L4 horizon level when head tilts")
add(C, "Scripted camera (cutscene)", "MUST", "P1", "PARTIAL",
    "CutsceneAuto default ON, base anchored to scripted camera (run 163), mode 3 ROT MATRIX",
    "Dishonored-VR CINEMATIC_HEAD_TRACKING", "HIGH",
    "L3 cutscene start/end logged; L4 cutscene watchable")
add(C, "Head collision", "NICE", "P4", "MISSING",
    "No collision clamp on head position", "KHARVOX", "HIGH", "L3 head inside wall rejected", "N/A")
add(C, "Fade near geometry", "NICE", "P4", "MISSING",
    "No near-plane fade", "Dishonored-VR rain depth", "MEDIUM", "L3 fade radius", "N/A")
add(C, "Zoom / ADS handling", "SHOULD", "P2", "PARTIAL",
    "AdsPullbackUU + AdsMaskAnim exist; F6 VR projection toggle",
    "KHARVOX ADS", "MEDIUM", "L3 ADS engage/disengage; L4 scope usable")
add(C, "Cutscene flat presentation", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
    "CutsceneFlatMode=1 / CutsceneCinemaMask=1 shipped; hub rows CUTSCENE VIEW + CINEMA MATTE (d93e2db, c94d832)",
    "Dishonored-VR cine window", "MEDIUM",
    "L3 SceneMode transitions logged; L4 flat menu readable in cutscene")

# ---------------------------------------------------------------- STEREO / RENDERING
C = "STEREO/RENDERING"
add(C, "Per-eye view matrix", "MUST", "P1", "L3_VERIFIED",
    "ApplyOffset(g_eyeDeltaUU) + ApplyEyeRemap(+/-0.5) in StereoPairBody; same order in laser/overlay",
    "vrframework 08 sec 4", "HIGH",
    "L3 left/right differentiated in log; L4 parallax visible")
add(C, "Stereo pass ownership (split vs mono)", "MUST", "P1", "L3_VERIFIED",
    "StereoPairBody duplicates with per-eye scissor; HudStereoPair remaps UI c5",
    "cyberpunk ViewReuse 'lend'", "VERY_HIGH",
    "L3 split/mono/offscreen counters sane; L4 no seam artifacts")
add(C, "Eye resource ownership", "MUST", "P1", "PARTIAL",
    "Single D3D11 bridge + one swapchain; per-eye dispatch exists for NIS. No pair_id/generation model",
    "dlss-nr-xr-layer eye identity = view index; Overlord pair_state",
    "VERY_HIGH",
    "L3 per-eye resources separate; L4 no cross-eye contamination")
add(C, "Render target state restoration", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
    "g_wristState->Capture()/Apply() around overlays; _origSetRenderTarget used",
    "Overlord restore_inherited_state()", "HIGH",
    "L3 state block save/restore asserted; L4 HUD does not corrupt world")
add(C, "Shadow-map generation exclusion", "MUST", "P1", "L3_VERIFIED",
    "g_boundShaderLayout.shadow gates all four draw hooks; InvalidateCamMats on shadow VS bind",
    "cyberpunk view census; vrframework 08 far/near guard", "HIGH",
    "L3 shadow draws counted separately; L4 no squashed shadow copies")
add(C, "Shadow sampling pass correctness", "MUST", "P0", "BROKEN",
    "Squares/different-per-eye persist across ShadowMode 0/1/2. shadow-sample counters 120-240/window. HighDecals=OFF removed all shadows -> decal/screen-space implicated",
    "cyberpunk CascadeSampling lend; research/shadow-vfx-reference-review-20261009", "VERY_HIGH",
    "L3 classify every sampling draw; L4 identical shadow both eyes")
add(C, "First-person shadow casters", "SHOULD", "P2", "PARTIAL",
    "TrackFpCaster + IsFpShadowCasterDraw; VrShadows default 1 now",
    "KHARVOX shadow policy", "HIGH", "L3 fp caster skip counter; L4 gun shadow correct")
add(C, "Decals", "MUST", "P1", "UNKNOWN",
    "No decal-specific classification; user evidence points here for squares",
    "cyberpunk view-dependent pass census", "MEDIUM",
    "L3 decal draws enumerated; L4 decals not doubled")
add(C, "SSAO", "SHOULD", "P3", "UNKNOWN",
    "No SSAO handling present", "Dishonored-VR", "HIGH", "L3 SSAO pass identified", "N/A")
add(C, "Fog", "SHOULD", "P3", "UNKNOWN",
    "No fog handling present", "cyberpunk fog history reprojection", "HIGH",
    "L3 fog pass identified; L4 fog consistent between eyes")
add(C, "Reflections", "SHOULD", "P3", "MISSING",
    "No reflection-camera exclusion (UE3 may have screen-space reflections)",
    "cyberpunk view census (reflection probes)", "HIGH",
    "L3 reflection camera excluded from remap", "N/A")
add(C, "Particles", "MUST", "P1", "PARTIAL",
    "VFX plugin draws overlay quads per-eye; sprite per-eye path exists (SpriteQuadPerEye)",
    "dishonored VFX plugin", "HIGH",
    "L3 item count per eye equal; L4 particles in both eyes")
add(C, "Muzzle flash", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
    "VfxPlugin socket table 300/350/950/750 mm; HideMuzzleFx suppresses engine emitter",
    "KHARVOX muzzle; VFX ABI round-trip PASS", "MEDIUM",
    "L3 fire event -> 1 item; L4 flash at barrel")
add(C, "Transparencies", "SHOULD", "P2", "PARTIAL",
    "User-pointer paths handle quads; generic transparency not specially treated",
    "vrframework 14 (2D screen-space suspect)", "MEDIUM",
    "L3 alpha draws per eye equal", "N/A")
add(C, "Bloom", "SHOULD", "P3", "UNKNOWN",
    "No bloom handling", "vrframework 14", "MEDIUM", "L3 bloom pass identified", "N/A")
add(C, "Postprocess / screen-space", "MUST", "P1", "PARTIAL",
    "UserPtrQuad passes fullscreen quads through unsplit (correct); no other post handling",
    "vrframework 14 (2D screen space suspect)", "HIGH",
    "L3 post passes enumerated; L4 no seam smear")
add(C, "Temporal effects (TAA/history)", "MUST", "P1", "UNKNOWN",
    "No TAA/history handling identified in D3D9 path (D3D9 game; history lives in engine)",
    "vrframework 08 sec 6.2 double-buffer by eye parity", "VERY_HIGH",
    "L3 temporal passes identified; L4 no smear on motion")
add(C, "Sky", "SHOULD", "P3", "UNKNOWN",
    "No sky-specific handling", "cyberpunk sky build", "MEDIUM", "L3 sky draws", "N/A")
add(C, "First-person layer", "MUST", "P1", "PARTIAL",
    "Foreground pass + gun/arms matchers + FgHideWholePass; palette path shared",
    "Overlord pair state", "VERY_HIGH",
    "L3 fg draw counts; L4 weapon/arms stable")
add(C, "Weapon layer", "MUST", "P1", "PARTIAL",
    "BuildGunC/BuildTmdC + ApplyMeshScale + holster; weapon=5799 in .vrrec (pistol only)",
    "KHARVOX weapon handling", "HIGH",
    "L3 pivot/scale; L4 weapon tracks controller")
add(C, "UI layer", "MUST", "P1", "PARTIAL",
    "HudStereoPair + GFx capture overlay + native viewport input",
    "vrframework guide 11", "VERY_HIGH",
    "L3 UI draws path-tagged; L4 UI readable in both eyes")
add(C, "Screen-space effects", "SHOULD", "P2", "UNKNOWN",
    "Not enumerated", "vrframework 14", "MEDIUM", "L3 census", "N/A")
add(C, "Depth target ownership", "MUST", "P1", "PARTIAL",
    "Hook_Clear sets g_inForeground; offscreen guard by size+format",
    "Overlord device generation", "HIGH", "L3 depth binds per draw; L4 depth correct")
add(C, "Upscaling (NIS)", "NICE", "P4", "IMPLEMENTED_NOT_VERIFIED",
    "NVIDIA Image Scaling v1.0.3 vendored, per-eye dispatch in D3D11 bridge, INI + hub rows",
    "dlss-nr-xr-layer layer order", "HIGH",
    "L3 NIS on/off A/B measured; L4 no ghosting")
add(C, "Resolution scaling", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
    "AutoResX/Y cached from headset; NIS render scale",
    "vrframework 14 SetWindowSize", "MEDIUM",
    "L3 render size matches target; L4 clear but smooth")

# ---------------------------------------------------------------- PERFORMANCE
C = "PERFORMANCE"
add(C, "Steady-state framerate", "MUST", "P0", "L3_VERIFIED",
    ".vrrec gameplay-only: p50 16.0 ms (62.5 fps), p95 16.0 ms, p99 47 ms. Docs: 87.3 fps with 0.06 ms hooks",
    "Dishonored-VR PERFORMANCE.md", "MEDIUM",
    "L3 p50/p95/p99 per window; L4 >=90 fps sustained")
add(C, "Frame pacing / stutter", "MUST", "P0", "PARTIAL",
    "43 deltas >100 ms in .vrrec, ALL in frames 5416-5478 (menu+UiValid): 140-172 ms sustained + one 3000 ms stall",
    "Dishonored-VR FLICKER_FRAME_DROP_AND_RESUME_PLAN", "HIGH",
    "L3 zero >50 ms deltas in a scene; L4 no judder")
add(C, "Menu transition stall", "MUST", "P0", "PARTIAL",
    "PERFORMANCE_MENU_TRANSITION: 47.82 ms window, 46.53 ms residual Present, 442 split draws, 26 offscreen targets",
    "Dishonored-VR DESKTOP_PRESENT_PERFORMANCE", "HIGH",
    "L3 transition ms <= 2x steady; L4 menu smooth")
add(C, "Per-draw hook overhead", "MUST", "P1", "L3_VERIFIED",
    "hotpath sampler: draw 0.27-0.33 ms/f, menu 0.009, other_set 0.10, set_texture 0.05, shadow 0.000",
    "code-tools/hotpath_audit.py + perf_compare.py", "LOW",
    "L3 sampler families all <1 ms/frame", "N/A")
add(C, "Present overhead", "MUST", "P1", "PARTIAL",
    "present_frame 6.7-7.7 ms sampled; residual estimate ~0.9-1.1 ms after instrumented stages",
    "Dishonored-VR present path", "HIGH",
    "L3 residual share small; L4 no double-present cost")
add(C, "OpenXR overhead", "SHOULD", "P2", "L3_VERIFIED",
    "wait_frame 4.5-5.7 ms/frame (idle wait), xr_submit 0.23-0.24 ms, pose_input 0.17-0.19 ms",
    "vrframework 07 frame timing", "MEDIUM",
    "L3 per-XR-call budget", "N/A")
add(C, "D3D9->D3D11 copy cost", "MUST", "P1", "L3_VERIFIED",
    "d3d_copy 0.58-0.70 ms; round-trip split: gpu wait 0.34-0.97, recoverable 0.01 ms",
    "dlss-nr-xr-layer EyeInterop", "HIGH",
    "L3 recoverable path near zero", "L4 link latency low")
add(C, "Redundant stereo work", "SHOULD", "P2", "PARTIAL",
    "perf_compare can show dup/split ratio; 442 split draws in the menu window is 3-4x gameplay",
    "code-tools/perf_compare.py", "MEDIUM",
    "L3 draws/frame similar across scenes", "N/A")
add(C, "Resource creation spikes", "MUST", "P1", "UNKNOWN",
    "No resource-creation timing in current instrumentation (NIS path has persistent resources)",
    "Overlord ring lifecycle", "MEDIUM",
    "L3 creation events per frame; L4 no hitch on transitions")
add(C, "State-block overhead", "SHOULD", "P3", "L3_VERIFIED",
    "State blocks captured per overlay draw; measured inside our work bucket",
    "Overlord restore_inherited_state", "LOW",
    "L3 state-block ms", "N/A")
add(C, "Logging overhead", "MUST", "P1", "L3_VERIFIED",
    "Log() is async-batched; spike reporter default OFF; FrameSpikeReport=0",
    "vrframework 14", "LOW",
    "L3 log ms/frame", "N/A")
add(C, "Shader work", "SHOULD", "P3", "PARTIAL",
    "Shader CTAB parsed ONCE at creation and cached per shader (Hook_CreateVS)",
    "vrframework 14", "MEDIUM",
    "L3 no per-draw CTAB parse", "N/A")
add(C, "Menu lifecycle performance", "MUST", "P0", "PARTIAL",
    "SolvePropertyLayout/RefreshArmedState/UpdateFgLatch/ResolveGameplayOffsets gated on InGameplay()",
    "Dishonored-VR menu handling", "MEDIUM",
    "L3 menu hook ms ~= 0; L4 menu 90 fps")
add(C, "p50/p95/p99 reporting", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
    "perf_compare.py emits avg/median/p95/p99/min/max + deltas",
    "code-tools/perf_compare.py", "LOW",
    "L3 compare report", "N/A")
add(C, "Foveation", "NICE", "P4", "NOT_APPLICABLE",
    "D3D9 engine render path; foveation needs runtime support we do not control",
    "dlss-nr-xr-layer", "HIGH", "N/A")
add(C, "Sampler/regression harness", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
    "hotpath_sampler + perf_compare tools in code-tools/ (tools branch)",
    "code-tools/", "LOW",
    "L3 two-run comparison", "N/A")
