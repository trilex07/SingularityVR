# Left-handed VR controls — 2026-11-10

Branch `feature/left-handed-vr-settings-20261110`, stacked on German offline
continuation `90662118`. Offline candidate only; no deployment, game launch,
installed INI modification, registry/save edits or installer publication.

## Behavior

VR Settings > Controller > LEFT HANDED is present in full and minimal settings.
The existing pending/applied split remains: closing saves Input/LeftHanded;
`*RESTART*` means bindings and render roles change together on the next launch.
Default remains0. Enabling selects gun/controller0 (left), TMD/controller1
(right), L3 weapon wheel, L1 native Use/grip. Fire follows L2; Impulse follows
R2. Both Centurion and Valkyrie/Volk/Kasimov ammo sources move behind the right
shoulder and use the right controller and rendered wrist witness.

Face/menu buttons retain their existing physical actions, including Circle
reload/back and Triangle pause. Blind face mirroring would alias Triangle with
reload. Locomotion axes remain governed independently by SwapSticks. The wheel
uses the weapon controller's physical stick and hand anchor; the other click
vetoes it, and the L3+R3 recenter chord remains symmetric. The right-handed
SwapSticks wheel selection behavior is retained.

## Render and reload integration

- Source role and controller side are independent: native gun mount/reference
  remains the gun mount while sampled pose comes from the left controller.
  Native TMD source remains its original left-wrist rig, targeting the right
  controller. No negative mesh scale, topology edits or reflected textures.
- Anatomical glove wrists use their own bone9/51 pivots. TMD hardware retains
  the native-source correction also used by its socket attachment ride. CRC,
  exact section/shader, scene/foreground and resource restoration gates remain.
- Reload acquisition, ownership arbitration, knife exclusion, tracked carry
  sample stamps and render wrist publication follow the semantic off hand.
  No native inventory writes or new ABI assumptions. Missing identities/wrist
  receipts retain the existing refusal behavior.
- Shoulder yaw/capsule offsets mirror laterally; configured magnitudes and
  reach windows remain. Legacy ExtraLeft option now extends outward on the
  selected shoulder. PL/EN tutorials and reload hints show the active side.
- Existing optional native authored support pairs/single-hand weapon grip
  calibration are right-handed measurements. Left mode uses independent
  anatomical pivots and measured support targeting where available. The optional
  TmdSupportGauntlet hardware presentation stays hidden during left-handed
  two-hand support until a right-forearm fit is measured. No claim of mirrored
  palm fit or complete physical support-hand acceptance.
- Existing Seeker back-slot shortcut remains right-handed gated; this change
  concerns ammunition behind the opposite shoulder, not a new back holster.
  Physical-left equipment HUD and right-controller menu pointer remain as before.

## Evidence

L1_PASS: 26 CPU fixtures in tools/tests/test-dual-weapon-tmd.ps1 -SkipOled,
plus existing9+3 Python wiring checks and locker preview guard.
New layout fixture covers valid XR paths, L3/R3 wheel release/commit/chord,
both shoulder regions over25 yaw headings, and anatomical wrist pivots through
nonidentity component/ride transforms. Existing tracked-grip fixture covers
full quaternion rotations, near-vertical head pose and recenter invariance.

Additional PASS: PL/EN reload instructions, reload controls, hand anchor math,
11 exact cooked attachment identities/CRC collision refusal, wheel state,
settings panel14208 pixel/UV hits,7 settings/4 carry wiring checks. Actual
MinHook x86 /O2 fixture on OWN fake inventory:60000 native/fast ACKs across3
weapons and BOTH handedness layouts, conserved ammo, foreign-object/automatic/
duplicate/holster/old-Centurion permit refusal. Separate pure transaction fixture
runs60000 transactions across3 weapons and both layouts. None executes game code.

EN/DE real-PE core/input contract+mutation fixtures PASS; protected retail disk
refusal PASS; native cinematic fixture PASS. DE remains runtime-disabled as
described in gog-german-remaining-20261110.md. Historical menu-profile check
still expects72 while unchanged default is100; not rerun as a left-hand gate.

L2 build/provenance receipt is recorded after the final committed-source build.
Logs in private build/shotgun-disc-gog-20261109/left-*-final.log. OLED/GPU runtime,
Steam/GOG gameplay and physical headset acceptance NOT RUN. Existing installed
and installer artifacts were not replaced.

Physical acceptance after a separately managed deployment: enable LEFT HANDED,
close panel and restart; check left L2 gun/right R2 TMD through wrist rolls and
sky aim, L1 Use, L3 open/select/release, L3+R3 recenter, Circle/Triangle behavior,
then reach behind RIGHT shoulder and insert for each enabled manual-reload
profile. Test release/rearm after tracking loss/menu/support. Assess palm fit
and original animated TMD socket parts in both eyes separately from CPU PASS.
