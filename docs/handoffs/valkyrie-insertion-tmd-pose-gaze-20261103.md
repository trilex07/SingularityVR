# Valkyrie insertion, TMD weapon attachment and optional gaze aim

## Accepted base

User accepted installed A808198C and explicitly authorized main promotion on 2026-11-03. Remote main was fast-forwarded to ecec5ebcc24c5dce4cc7cffd4c935b5b7e89deb5. This is grouped user acceptance, not invented granular observations. New changes remain on fix/valkyrie-insertion-tmd-pose-gaze and require PSVR2 acceptance. Steam still contains A808198C; no game launched for this work.

## Evidence and smallest corrections

1. Valkyrie: actual session repeatedly reaches grab-fresh with nativeMagazineDraws increasing, then discards on release at controller-to-socket distances 0.204/0.208/0.210 m against a 0.200 m radius. Acquisition is working. Runtime used the controller origin for Valkyrie, whereas drawing uses the source-derived palm-to-magazine mount. This coordinate-contract mismatch is PROVEN; its responsibility for every physical insertion failure remains to verify. Extend the existing Kasimov measured insertion route to Valkyrie: use the carried magazine joint from the same wrist/transaction witness and weaponcarry::Matrix as rendering. No guessed offset, radius enlargement, ammo write or second FSM. Missing/stale/foreign witness still rejects. A bounded release receipt records handDist/clipDist/valid/radius (16 maximum).

2. TMD: matrices are row-vector transforms. The old inverse(wrist)*gun relationship and wrist*relative application only cancel at the reference pose; moving the controller rotates/translates the attachment incorrectly. Store gun*inverse(wrist), apply relative*predictedWrist. Native child/recoil animation is preserved through the common right-side rigid delta; native camera tail cancels. The captured-matrix fixture with a translated/rotated controller demonstrates old error 90.5018 UU versus corrected 0.000008 UU. Unchanged-pose tests alone missed this. This matrix defect is PROVEN; physical outcome remains PENDING.

The actual failed log also reports Bone refusal with shader capacity=75. CTAB capacity is not the mesh bone map. Reuse the original cooked/reload spec's active slots (Valkyrie 6, Volk 5, Kasimov 6) ONLY when the currently bound original IB equals its hash-validated cached identity and the exact body signature matches. Other contexts retain full validation. Save, transform and restore precisely that active range; unused constants remain untouched. Active non-finite matrices still reject. Offline source evidence: prepare_weapon_animation.py compares cooked palettes with PSK contributors; audit_next_weapon_space.py checks all slot/weight/index correspondence. No guessed bone indices.

3. Optional [Input] TmdGazeAim=1 (default 0): reuse current Toolkit shared gaze reader and verified local TMD GetAimVectors/GetAgingVectors hooks. VIEW gaze is rotated by the current OpenXR head quaternion, then converted through the existing tracking-to-game convention. Only direction changes; hand origin, native power/energy rules and weapon aim stay authoritative. Provider timestamp survives publication: samples older than 100 ms, future samples, blink/invalid data or missing reader fall back to the tracked hand ray. Hook/context failure disables both native/gaze gates. No new input bindings or shared weapon trace hook.

Gaze limitation: this follows gaze direction from the hand origin, not a depth-converged exact point on a near object; no guessed convergence distance or fabricated target. Toolkit/bridge must actually publish gaze. Physical direction, target selection and fallback require one HMD check. The option does not enable or invent unavailable powers.

## Offline validation

- x86 /O2 dual-TMD/reload/aim/holster/WeaponLocker policies and OLED D3D11 fixtures PASS.
- Actual native adapter fixture exercises gaze-only enable, hand-origin retention, invalid fallback, nested weapon-fire refusal, ownership/thread/pause/stale/SEH contracts.
- Row-vector motion fixture PASS, historical order counterexample FAIL as expected. Stereo identical palettes, camera-tail cancellation, child/recoil relative transforms and stale identity gates retained.
- Active-palette fixture: unused poisoned register accepted only with exact audited identity; active poisoned register still refused; unknown identity retains conservative validation.
- Reload fixture: clip at socket with controller 0.269 m away inserts; invalid witness rejects; controller at socket with clip 1 m away rejects. Existing 30,000 synthetic ammo-conservation transactions PASS, not native inventory proof.
- Gaze/cinema/damage/mutant x86/x64 fixtures PASS. Actual named shared mapping tests timestamp propagation and blink invalidation. D3D9Ex stereo/cinema pixels and state restoration PASS; legacy D3D9 HAL NOT RUN (host unavailable).
- Final build and compiled ABI identity: see private candidate.json and build provenance. No runtime, FPS or physical PASS inferred from these tests.

## Consolidated physical acceptance

Use the accepted settings snapshot plus only TmdGazeAim=1 for this candidate. Keep existing gaze reader/Toolkit enablement. Retain bank B8126A75 and bridge F58CB3A8; no asset regeneration.

1. Valkyrie: Circle, fresh magazine from left shoulder, visibly seat magazine and release Grip. Verify one native ammo transfer, no flicker/duplication; repeat with the rifle rotated.
2. L2 then release: translate body, turn head with right pad stationary, then move/rotate right pad. Weapon stays attached, VFX remain at muzzle. Check ordinary two-hand grip afterwards.
3. Valid TMD target: look left/right with left hand held steady and activate Age/Impulse. Check gaze direction; briefly invalidate gaze and verify hand fallback. Native energy/gates remain normal.

No automatic launch, test timer, Steam deployment or main merge of the new branch performed. Current Steam A808198C and INI hashes verified unchanged.

## Final delivery

- Source commit: `479c041b6e0ae6f8f9065c7536fa409c5522988d` (clean build).
- x86 PE32 /O2 DLL: `CB43F6DCD1D8FD062981ECB840023432656AC5AD95F770C1D23D0CCE77A204BC`.
- Compiled WeaponLocker ABI PASS: literal `[eax+20Ch]` and exact FName stack setup retained.
- Private candidate, immutable artifact snapshot, provider/pose tests, captured session and exact deployment profile/plan: `build/next-weapon-reload/valkyrie-tmd-gaze-20261103/`.
- Staged INI changes only `Input/TmdGazeAim=1`; code default remains OFF. Exact deployment guard preflight PASS. Steam A808198C and accepted INI remain unchanged; no game launch.
