# HD weapons and inner-wrist follow-up

## Physical baseline and evidence

Installed candidate A2D9042A206C681A1ABE0A1C606129DD7EB335DAFCE5ACC6BDC02EB7D2F27720 failed the user's headset test: Kasimov scope/support/reload, Centurion aim, carried magazines/cylinder flicker, outer/upside-down wrist display. This supersedes earlier pending status. Session evidence is archived privately at `build/wrist-main/session-20261030-followup-fail/view_matrix.log`.

The session contains successful Centurion native reload acknowledgements and firing; no crash was recorded in this session. That does not establish that every earlier crash is resolved. Kasimov shoulder acquisition succeeds, but insertions are discarded at controller-to-socket distances 0.225–0.254 m. Scope activation remains zero. Wrist visibility queries return pixels in both eyes, so visibility alone is no longer the failure.

## Confirmed contracts and corrections

- **Centurion:** the original native `muzzle_flash` bind point has Y=-114.69569557 UU. Runtime used +114.69569557, 229.3914 UU across the mesh. The old asset test reflected an already native bind matrix again. Correct the witness point/axis only; keep the accepted native ray machinery, spread and recoil. The actual source-mesh test rejects the old point as more than 200 UU from the mesh.
- **Kasimov:** held-root handling replaced only cog_sniper while the real native vertex buffer has 179 mixed-weight wedges. All six palette slots now receive the same right delta `inverse(nativeRoot) * heldRoot`, preserving their relative animation. Actual source frame/rotation tests reproduce up to 90.0074 UU deformation with the old algorithm, versus 0.0000307264 UU maximum error with the common delta.
- **Kasimov final pose:** include the exact body in the existing dual-TMD gun correction. Publish muzzle, optic, support and insertion socket from the final body palette, after that correction. Scope publication no longer depends on a fresh reload-FSM visual snapshot. The separately drawn native lens uses the same-frame displayed cog/local frame, with its own local matrix removed; stale frame/identity and singular matrices are refused. Restore its original registers on completion/failure. No scope threshold or grip offset is enlarged.
- **Kasimov insertion:** compare the real carried clip bind origin (existing source-derived palm mount) with the native clip socket; the controller origin is not that point. Keep raw controller coordinates for shoulder pickup. Missing or foreign wrist data cannot issue a native reload permit. Other weapons retain their insertion policy.
- **Carried parts:** freeze both pose and validity across every draw/eye of a frame/transaction. Previously an invalid first pass could become a valid later pass in the same frame. Centurion now also uses the common frame sampler before size matching. This fixes an offline-proven consistency violation; whether it explains all physical flicker remains PENDING, not proven by the tests.
- **Wrist panel:** opt-in `EquipmentWristInner=1` reverses the surface normal and glyph tangent together. Existing outer mode remains the default for compatibility. Candidate uses measured +69 mm X clearance, -80 mm Y, Z=0 and quarter-turn=1; source footprint starts 2.5 mm behind the wrist. `measure_wrist_panel_clearance.py --inner` derives clearance from original SVNC/SVTG source geometry, recorded in `wrist-panel-inner-clearance-20261030.json`. No controller/hand position is changed. Bind clearance and orientation math still require animated headset acceptance.

## Offline validation

- `tools/tests/test-hd-physical-followup.ps1 -ReloadAssets <private reload root>`: actual Kasimov SVKS weighted vertices, 5 source animation frames x 2 rotations, lens-frame equivalence/singular refusal, insertion FSM, Centurion origin, frame wrist identity/validity and all three existing native reload roundtrips PASS.
- `tools/tests/test-kasimov-hd-runtime.ps1`: source geometry/143 weighted points/80 bones, mechanics and real D3D9/D3D9Ex stereo pixels/restoration/failure/reset PASS.
- `tools/tests/test-centurion-hd-runtime.ps1`: Centurion reload/cylinder/native ABI, aim, scope, wheel, controls, WeaponLocker and real D3D9/D3D9Ex pixels/restoration/failure/reset PASS.
- `tools/tests/test-dual-weapon-tmd.ps1`: hands, support, TMD, accuracy, reload, WeaponLocker and OLED D3D11 pixel/lifetime regressions PASS.
- `tools/tests/test-hud-live.ps1`: placement/final palette, real D3D9/D3D9Ex world/glove/magazine occlusion, state restoration, failure and reset PASS. GPU tests required normal desktop GPU access, not the restricted sandbox.

No gameplay was launched during this follow-up. No physical PASS, crash clearance, FPS gain or production health-HUD replacement is claimed. Keep legacy health suppression OFF and the currently retained audio/HD/localization/haptics/NIS/OLED files unchanged. Build/deployment identities follow in the candidate receipt.

## One consolidated physical acceptance

Kasimov: bring scope to eye/R1, grip/release with both hands, fire once, Circle -> shoulder Grip -> carry -> insert -> fire. Check that all parts stay assembled and the magazine is stable in both eyes. Centurion: aim at a fixed wall target at medium range, fire, then carry/insert a fresh cylinder and fire again. Raise the inner left wrist: text must read upright, lie outside the inner cuff, and be occluded by wall, weapon and carried part. Observe TMD/support/reload transitions. Close normally and correlate the actual session with the exact candidate hash. Any failure remains open; do not promote to main on offline evidence.
