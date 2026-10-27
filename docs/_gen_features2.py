# -*- coding: utf-8 -*-
"""Feature taxonomy part 2: input, hands, weapons, TMD, interaction, movement."""

def register(add):
    # ---------------------------------------------------- CONTROLLERS / INPUT
    C = "CONTROLLERS/INPUT"
    add(C, "OpenXR actions", "MUST", "P1", "L3_VERIFIED",
        "Action set attached; xrSyncActions counted; .vrrec records trigger/stick/pose",
        "vrframework guide 12", "MEDIUM", "L3 actions created and bound", "N/A")
    add(C, "Trigger (analog)", "MUST", "P1", "L3_VERIFIED",
        ".vrrec left_trigger 0..1 (12 active), right_trigger 0..1 (121 active)",
        "KHARVOX trigger/value", "LOW", "L3 analog range; L4 pressure works")
    add(C, "Grip", "MUST", "P1", "L3_VERIFIED",
        ".vrrec left_grip 0..1 active on 112 samples; right_grip never pressed in session",
        "KHARVOX grip near/far", "LOW", "L3 grip state; L4 two-hand works")
    add(C, "Sticks", "MUST", "P1", "L3_VERIFIED",
        ".vrrec left stick -0.998..1.0 both axes; right_stick_x 0.0 always; right_stick_y -0.446..0.254",
        "KHARVOX stick mapping", "LOW", "L3 axes; L4 turn and move work")
    add(C, "Deadzones", "MUST", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "Radial deadzone documented (run 70 fix) to stop packet churn",
        "REFramework", "LOW", "L3 resting state bit-identical", "N/A")
    add(C, "Handedness / left-handed mode", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "LeftHanded row + AimHand()/OffHand() role swap; swap-sticks rows",
        "KHARVOX Left Hand modes", "MEDIUM",
        "L3 roles swap; L4 left-handed playable")
    add(C, "Menu button (Triangle/pause)", "MUST", "P1", "PHYSICALLY_VERIFIED",
        "Triangle = single START rising edge; user-required to preserve. docs/PSVR2_PROMPT_MAP.md",
        "Dishonored-VR", "LOW", "L3 menu flag; L4 pause opens")
    add(C, "SteamVR bindings", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "Index profile bindings suggested (xrinput: 40 bindings for valve/index_controller)",
        "KHARVOX", "MEDIUM", "L3 bindings suggested", "L4 Index playable")
    add(C, "PSVR2 bindings", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "PSVR2 prompt icons + prompt map doc; adaptive trigger backend abstraction exists",
        "KHARVOX PSVR2 toolkit", "MEDIUM", "L3 bindings; L4 Sense icons correct")
    add(C, "Glyphs", "MUST", "P2", "PARTIAL",
        "PSVR2 prompt icons implemented for menu/interaction; runtime records interaction/R1 as action 1",
        "Dishonored-VR", "MEDIUM",
        "L3 glyph selection matches action", "L4 correct icon on chest")
    add(C, "Controller reconnect", "SHOULD", "P2", "MISSING",
        "No reconnect handling found", "KHARVOX", "MEDIUM", "L3 disconnect/reconnect cycle", "N/A")
    add(C, "Haptics (basic)", "SHOULD", "P3", "IMPLEMENTED_NOT_VERIFIED",
        "XInputSetState hook forwards game rumble to XR haptics; OnReloadStarted/OnReloadFinished hooks referenced",
        "KHARVOX haptics", "MEDIUM", "L3 fire -> haptic pulse", "L4 feel")
    add(C, "Adaptive triggers", "NICE", "P4", "IMPLEMENTED_NOT_VERIFIED",
        "psvr2_trigger.h + adaptive trigger backend abstraction + weapon-specific resistance mentioned",
        "KHARVOX PSVR2 toolkit", "HIGH", "L3 resistance applied", "L4 trigger feel")

    # ---------------------------------------------------- HANDS / BODY
    C = "HANDS/BODY"
    add(C, "Right hand tracking", "MUST", "P1", "L3_VERIFIED",
        ".vrrec right pose stream moves on every sample; weapon rides right hand (aim hand)",
        "Overlord controller_pose_reference", "MEDIUM", "L3 pose continuity; L4 weapon tracks")
    add(C, "Left hand tracking (input)", "MUST", "P1", "L3_VERIFIED",
        ".vrrec left_x/y/z/left_yaw/pitch/roll ALL move across 5573 samples -> input path alive",
        "Overlord", "LOW", "L3 left stream monotonic", "proves input is not the fault")
    add(C, "Left hand rendering (palm follows)", "MUST", "P0", "BROKEN",
        "User reports frozen left hand. VR_SESSION_REVIEW: fault is render/palette not recording. Post-review build removed the aim-dev gate in PrepareIndependentLeft",
        "Overlord hand palette", "VERY_HIGH",
        "L4 left palm visibly follows the controller", "top P0 after perf")
    add(C, "Independent hands", "MUST", "P1", "PARTIAL",
        "IndependentHandsPrototype=1 shipped; GloveIsolate=2 in deployed ini",
        "Overlord", "VERY_HIGH",
        "L3 both hands move independently of HMD", "L4 both hands track")
    add(C, "Wrist position", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "g_handLastPos -> milli-UU offsets consumed by BuildGunC/BuildTmdC",
        "Overlord", "MEDIUM", "L3 wrist target vs controller", "L4 hand at controller")
    add(C, "Wrist orientation", "MUST", "P1", "PARTIAL",
        "g_handDevYaw/Pitch/RollUU populated; quat->bone conversion is NOT complete per LEFT_HAND_SCALE_AUDIT",
        "Overlord", "HIGH", "L3 orientation applied", "L4 hand points where controller does")
    add(C, "Shoulder/elbow IK", "SHOULD", "P2", "MISSING",
        "arm_ik::Solve exists but has no production caller; ArmIKMode=0 default",
        "Dishonored-VR SkelControlLimb/CCD_IK", "VERY_HIGH",
        "L1 reach/clamp tests (already pass)", "L4 elbow natural")
    add(C, "Arm length accuracy", "SHOULD", "P2", "UNKNOWN",
        "No arm-length calibration exposed", "KHARVOX", "MEDIUM", "L3 segment lengths", "N/A")
    add(C, "Hand visibility policy", "MUST", "P1", "PARTIAL",
        "Glove-only mode (ShowControllerArms=0) shipped as default; TEST_RESULTS says visual headset-pending",
        "Dishonored-VR ARM_HAND_SPLIT", "HIGH",
        "L3 glove-only flag", "L4 hands visible, arms cut")
    add(C, "Arm visibility policy", "SHOULD", "P2", "PARTIAL",
        "Arm split at wrist concept exists; GloveIsolate collapses bone ranges",
        "Dishonored-VR", "HIGH", "L3 collapse slots", "L4 arms look right")
    add(C, "Body visibility", "NICE", "P4", "NOT_APPLICABLE",
        "First-person only game; no body model to show", "KHARVOX", "HIGH", "N/A")
    add(C, "Head hiding", "MUST", "P1", "NOT_APPLICABLE",
        "First-person game: no player head mesh in view", "N/A", "LOW", "N/A")
    add(C, "Finger posing", "NICE", "P4", "IMPLEMENTED_NOT_VERIFIED",
        "Procedural finger posing from curl inputs staged (f7be176) + finger pose on verified glove sections",
        "Overlord", "HIGH",
        "L1 glove_finger_test PASS", "L4 fingers curl naturally")
    add(C, "Hand/controller alignment", "MUST", "P1", "PARTIAL",
        "GunScalePct 70 tuned by user; per-weapon GunAlign slots exist",
        "KHARVOX grip calibration", "MEDIUM", "L3 anchor values", "L4 gun sits in hand")
    add(C, "Support-hand grip", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "TwoHandGrip latch + grip_press/release/latch counters; .vrrec TwoHand flag 112 samples",
        "KHARVOX two-hand", "HIGH", "L3 latch counters", "L4 both hands steady")
    add(C, "Support-hand release", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "gripReleaseEdges + gripDetachCount counters tracked",
        "KHARVOX", "MEDIUM", "L3 release counter", "L4 weapon returns")
    add(C, "Full-body policy", "NICE", "P4", "MISSING", "N/A (first-person game)", "KHARVOX", "VERY_HIGH", "N/A")

    # ---------------------------------------------------- WEAPONS
    C = "WEAPONS"
    add(C, "Weapon/controller alignment", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "BuildGunC pivot + per-weapon GunAlign[i] (fwd/right/up/yawTenths/pitchTenths)",
        "KHARVOX per-weapon alignment", "HIGH", "L3 pivot math", "L4 barrel on crosshair")
    add(C, "Per-weapon offsets", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "GunAlign slots serialized from the hub (PanelSaveRow writes all 3 slots)",
        "KHARVOX", "MEDIUM", "L3 per-weapon table", "L4 each weapon aligned")
    add(C, "Muzzle origin", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "VFX muzzle offset socket table: pistol 300 / shotgun-rifle 350 / sniper 950 / rail 750 mm; + ini override",
        "KHARVOX muzzle", "MEDIUM",
        "L3 offset chosen matches weapon", "L4 flash leaves the barrel")
    add(C, "Projectile origin (trace from gun)", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "g_traceFromGun default 1; SingleLineCheck/LineCheck rotation by hand deviation",
        "Dishonored-VR VR-57 aim pipeline", "VERY_HIGH",
        "L1 rotation tests PASS", "L4 bullet lands where you point")
    add(C, "Recoil", "SHOULD", "P3", "MISSING",
        "No recoil-specific handling (game own recoil untouched)", "KHARVOX", "MEDIUM", "L3 recoil event", "N/A")
    add(C, "Haptic recoil", "NICE", "P4", "UNKNOWN",
        "Haptics forwarded generically; no recoil-specific profile", "KHARVOX", "MEDIUM", "L3 haptic on fire", "N/A")
    add(C, "Two-hand grip", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "g_supportLatch with acquire/hold/grace; .vrrec TwoHand flag present",
        "KHARVOX two-hand support", "HIGH", "L3 latch lifecycle counters", "L4 steady aim")
    add(C, "Virtual gunstock", "NICE", "P4", "MISSING",
        "No virtual stock implementation", "KHARVOX Virtual Gunstock", "HIGH", "L3 stock anchor", "N/A")
    add(C, "ADS", "SHOULD", "P2", "PARTIAL",
        "AdsPullbackUU + AdsMaskAnim (c5-c22 tail 400 ms) exist",
        "KHARVOX ADS", "HIGH", "L3 ADS engage", "L4 scope usable")
    add(C, "Dominant-eye ADS", "NICE", "P4", "MISSING", "Not addressed", "UEVR-style mods", "MEDIUM", "N/A")
    add(C, "Laser sight", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "LaserSight ini default 1; LaserHoldMs=1000 toggle; laser drawn per-eye from gun pivot",
        "KHARVOX laser sight", "MEDIUM", "L3 laser toggle", "L4 laser from muzzle")
    add(C, "Reticle / crosshair", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "HideCrosshair=1 default; crosshair census + LooksLikeCrosshair + drop rule",
        "Dishonored-VR reticle", "MEDIUM",
        "L3 xh8Seen/xh8Drop/xh8MaxR counters", "L4 no misleading reticle")
    add(C, "Scope", "SHOULD", "P2", "PARTIAL",
        "ScopeQuadPerEye + scope register remap + ScopeActive gate exist",
        "KHARVOX", "HIGH", "L3 scope overlay per eye", "L4 scope circle")
    add(C, "Reload (button)", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "Native X = Use/reload (Raven label); no separate GBA_Reload in input table",
        "MANUAL_RELOAD_RESEARCH.md", "HIGH", "L3 reload fires on X", "L4 reload works")
    add(C, "Manual reload (gesture)", "NICE", "P4", "IMPLEMENTED_NOT_VERIFIED",
        "manual_reload.h pure state machine + test-manual-reload.cpp PASS; deliberately NOT wired into production",
        "MANUAL_RELOAD_RESEARCH.md", "HIGH",
        "L1 recognizer tests PASS", "L4 physical reload feels right")
    add(C, "Weapon switching", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "Weapon swap edge from Pawn.Weapon pointer (g_weaponSwapped); R3 weapon wheel added",
        "KHARVOX weapon wheel", "HIGH", "L3 swap edge", "L4 switch is instant")
    add(C, "Weapon wheel", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "R3 weapon wheel implemented (0a76670) with slot counter ini",
        "KHARVOX", "HIGH", "L3 wheel rows", "L4 wheel usable")
    add(C, "Holsters (physical)", "NICE", "P4", "IMPLEMENTED_NOT_VERIFIED",
        "Motion controller weapon holster (0b33629) + hip transform + rate-limited log",
        "KHARVOX physical holsters", "HIGH",
        ".vrrec: WeaponHolstered flag on 397 samples BUT hip_x/y/z all zero -> recorder never writes hip",
        "L4 weapon disappears at hip")
    add(C, "Grenade throw", "SHOULD", "P2", "MISSING",
        "No throw gesture (left trigger throws carried objects? unverified)", "KHARVOX", "HIGH", "L3 throw", "N/A")
    add(C, "Melee", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "Impulse button arms a 700 ms melee window (g_meleeArmsMs) and logs gun verts. .vrrec Melee flag 2 samples",
        "Dishonored-VR PHYSICAL_SWING", "MEDIUM",
        "L3 melee window logged", "L4 swing triggers attack")
    add(C, "Alt fire", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "Grip in front / behind mapping exists via PadButtonAction masks",
        "KHARVOX", "MEDIUM", "L3 alt fire event", "L4 alt fire")
    add(C, "Weapon-specific interactions", "NICE", "P4", "MISSING", "N/A", "N/A", "HIGH", "N/A")

    # ---------------------------------------------------- TMD / SINGULARITY-SPECIFIC
    C = "TMD-SPECIFIC"
    add(C, "TMD hand", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "TmdOnOffHand=1, TMD anchor F/R/U + trims; BuildTmdC rides off hand when TmdRaised()",
        "Singularity-specific", "HIGH", "L3 TMD pose; L4 TMD in left hand")
    add(C, "TMD aiming", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "g_offHandDevYaw/Pitch/RollUU consumed by BuildTmdC; aim-mode coexistence documented",
        "Singularity-specific", "HIGH", "L3 TMD aim delta", "L4 TMD points where you point")
    add(C, "TMD effects / VFX", "MUST", "P1", "PARTIAL",
        "VFX plugin has blood atlas + item list; TMD-specific VFX routing not proven",
        "Singularity-specific", "HIGH", "L3 TMD VFX item", "L4 TMD beam/flash right")
    add(C, "TMD haptics", "SHOULD", "P2", "UNKNOWN", "No TMD-specific haptics found", "Singularity-specific", "MEDIUM", "L3 TMD haptic", "N/A")
    add(C, "TMD prompts", "SHOULD", "P2", "UNKNOWN",
        "Same prompt classifier problem as world prompts", "Singularity-specific", "MEDIUM", "L3 TMD prompt", "N/A")
    add(C, "TMD weapon coexistence", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "Run 235c measured: TMD is HIDDEN, not swapped - Pawn.Weapon stays on the gun. Weapon/HANDS status enum",
        "Singularity-specific", "HIGH", "L3 hands status transitions", "L4 TMD up, gun hidden")
    add(C, "TMD visual alignment", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "TMD ALIGN hub page (TmdFwd/Right/Up/Yaw/Pitch/Roll) + per-slot bone/tr window pages",
        "Singularity-specific", "HIGH", "L3 trims applied", "L4 TMD on forearm")
    add(C, "TMD two-hand interactions", "NICE", "P4", "MISSING", "Not addressed", "Singularity-specific", "HIGH", "N/A")

    # ---------------------------------------------------- INTERACTION
    C = "INTERACTION"
    add(C, "Context prompt classification", "MUST", "P0", "BROKEN",
        "LooksLikeContextPrompt is a hard geometric predicate (stride==20, prims 16-40, |x|<0.24, y in -0.25..-0.05). Its own comment says the first anchor was the NOTE close button, not the gameplay prompt",
        "vrframework guide 11 (name the movie, hash it)", "MEDIUM",
        "L3 prompt show counter with correct action", "L4 chest/fountain prompts visible")
    add(C, "Prompt action identity", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "promptAction hardcoded to 1 (interaction/R1) - confirmed by .vrrec: Prompt flag 164 samples, prompt_action 0 on all 5573",
        "Dishonored-VR per-element ownership", "MEDIUM",
        "L3 action id per prompt", "L4 [X] icon matches the prompt")
    add(C, "Crates/chest prompts", "MUST", "P1", "BROKEN",
        "User: prompts missing near crates/chests. Predicate above cannot distinguish them",
        "Dishonored-VR HANDOFF-HUD-ELEMENTS", "MEDIUM",
        "L3 chest prompt visible counter", "L4 real chest prompt")
    add(C, "Fountain prompt", "MUST", "P1", "BROKEN",
        "User: prompt missing at fountain (same root cause)",
        "Dishonored-VR", "MEDIUM", "L3 fountain prompt", "L4 fountain prompt")
    add(C, "Hand proximity interaction", "SHOULD", "P2", "MISSING",
        "No hand-proximity path; native XInput route is used", "KHARVOX", "HIGH", "L3 proximity test", "N/A")
    add(C, "Ray interaction", "MUST", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "Laser + ui hit plane drives native viewport input (Coordinates -> MapCanvas)",
        "REFramework overlay intersection", "HIGH",
        ".vrrec ui_u/ui_v have 1269/1272 distinct values -> pointer moves",
        "L4 laser selects menu items")
    add(C, "Interaction feedback (haptic)", "NICE", "P3", "UNKNOWN",
        "Generic haptics only", "KHARVOX", "MEDIUM", "L3 haptic on prompt", "N/A")
    add(C, "Prompt lifecycle", "SHOULD", "P2", "PARTIAL",
        "g_contextPromptVisible + show counter; removal when the draw stops",
        "Dishonored-VR HUD_ELEMENTS_HOWTO", "MEDIUM",
        "L3 show/hide transitions", "L4 prompt disappears")
    add(C, "No duplicate prompt", "MUST", "P1", "UNKNOWN",
        "Cannot be assessed until classification is semantic",
        "vrframework guide 11", "MEDIUM", "L3 one prompt per frame", "L4 no double icons")

    # ---------------------------------------------------- MOVEMENT / LOCOMOTION
    C = "MOVEMENT"
    add(C, "Smooth locomotion", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "Left stick move via synthetic pad; walk direction + swaps",
        "KHARVOX movement", "MEDIUM", "L3 move vector", "L4 walk works")
    add(C, "Snap turn", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "TurnMode=2 shipped (per ini), TurnAngle + TurnSpeed rows in hub",
        "KHARVOX snap", "LOW", "L3 turn step", "L4 comfort")
    add(C, "Smooth turn", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "TurnMode 0/1 with TurnSpeedDeg", "KHARVOX smooth", "LOW", "L3 speed", "L4 smooth")
    add(C, "Configurable snap angle", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "PR_SNAPANGLE hub row + TurnAngle ini", "KHARVOX", "LOW", "L3 angle change", "N/A")
    add(C, "Configurable turn speed", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "PR_TURNSPEED hub row", "KHARVOX", "LOW", "L3 speed change", "N/A")
    add(C, "Head-relative movement", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "WalkDirection=0 default", "KHARVOX head direction", "MEDIUM", "L3 direction basis", "L4 intuitive")
    add(C, "Off-hand relative movement", "SHOULD", "P2", "IMPLEMENTED_NOT_VERIFIED",
        "WalkDirection=1 + hand direction transform", "KHARVOX", "MEDIUM", "L3 hand basis", "N/A")
    add(C, "Sprint", "SHOULD", "P3", "UNKNOWN",
        "No sprint toggle found; engine default", "KHARVOX sprint toggle", "MEDIUM", "L3 sprint", "N/A")
    add(C, "Crouch (button)", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "Right-stick-down emits native Crouch/B; face button remapped to Health",
        "KHARVOX crouch", "MEDIUM", "L3 crouch fires", "L4 crouch works")
    add(C, "Physical crouch", "SHOULD", "P2", "MISSING",
        "No physical-crouch path (real crouch -> game crouch)", "Dishonored-VR physical crouch", "HIGH",
        "L3 real crouch detected", "L4 sneak by crouching")
    add(C, "Jump", "MUST", "P1", "IMPLEMENTED_NOT_VERIFIED",
        "PadButtonAction 'jump' A/click; sprite/ring artifacts were seen at jump time (12:54 capture)",
        "KHARVOX jump", "LOW", "L3 jump fires", "L4 jump works")
    add(C, "Teleport", "NICE", "P4", "MISSING", "Not implemented", "Many mods", "MEDIUM", "N/A")
    add(C, "Comfort vignette", "NICE", "P4", "MISSING",
        "No vignette implementation", "Dishonored-VR comfort vignette", "MEDIUM", "L3 vignette", "N/A")
    add(C, "Ladder behavior", "NICE", "P4", "UNKNOWN", "Not assessed", "N/A", "HIGH", "N/A")
    add(C, "Swimming", "NICE", "P4", "UNKNOWN", "Not assessed", "N/A", "HIGH", "N/A")
    add(C, "Vehicles", "NICE", "P4", "NOT_APPLICABLE",
        "No vehicle sections identified in Singularity", "N/A", "HIGH", "N/A")
