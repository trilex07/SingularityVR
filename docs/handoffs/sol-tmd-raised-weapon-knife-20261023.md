# TMD-raised right hand and knife — MAIN handoff (2026-10-23)

Branch: `fix/sol-tmd-raised-weapon-knife`, based on `3929868`. Steam and the installed INI were not touched. These are offline candidate changes, **not** PSVR2 acceptance.

## Evidence and corrections

1. **Right glove with a held gun (`9593169`).** `PrepareIndependentLeft` selected the authored `BuildGunC` right-grip target for the no-TMD glove section but not for the TMD material-0 section. The latter instead used the unadjusted controller target, although both display the same right hand. The TMD section now takes the same weapon-specific single-hand gate and target (Centurion, Valkyrie, Volk, Kasimov); active two-hand support still wins. The independent free-wrist correction is skipped when the weapon grip owns the target. This is a verified source-path mismatch and math correction; physical alignment remains to be checked.
2. **Knife on the TMD glove (`0cf6c17`).** The archived runtime log records `held standalone route` followed by native-joint submissions for both eyes in the **same pickup frame 2726**. That proves two presentation routes were active, though it alone does not prove which triangles caused the observed fragmented glove. The overlay now reserves a TMD-cuff native-joint route per pickup when its prerequisites are present; a joint absent for two frames switches to a locked standalone fallback. The native cuff path does not also draw during fallback. New receipts include `tmdCuff` and triangle count. No knife mount offsets or glove geometry were changed.
3. **L2 diagnostic (`6e15e77`).** The old `L2` value was the LT/ADS channel, intentionally suppressed during TMD Impulse, so it was not evidence of the physical Impulse button. Diagnostics now sample the existing OpenXR Impulse action before suppression and label `Impulse`, `ADS`, and `R2` separately. Gameplay mapping is unchanged.

Archived evidence: `deployment/crash-evidence/20261023-sol-3929868-psvr2/view_matrix.log` in MAIN's evidence checkout, SHA-256 `55BCAA84C6CC4282E1D51469E9C0BC86D2FDF7056A99AC40296B8E716166C318`.

## Verification

- `spikes/view_matrix/build.ps1`: PASS, x86 PE32 `/O2`; DLL SHA-256 `018C132FDAB31BA7EC92A31EAF535DF2EFA6E2D57EFAB5B2E383611291DD8138` at source commit `6e15e77`.
- `tools/tests/test-dual-weapon-tmd.ps1 -SkipOled`: PASS (dual-TMD, support, reload, WeaponLocker and wiring regressions).
- `tools/tests/test-dual-tmd-gun-scale.ps1`: PASS (real x86 MinHook producer and safety gates).
- `tools/gloves/test-tmd-short-gauntlet.ps1 -Asset <private SVTG>`: PASS (7110-triangle stereo dispatch, 49-run fallback, palette/visibility tests).
- `spikes/view_matrix/tests/run-test-oled.ps1`: PASS (CPU and D3D11 GPU comparison).
- `tools/gloves/test-tmd-runtime-regressions.ps1 -Asset <private SVTG>`: CPU tests passed until the D3D9 device test; `CreateDevice(...HAL...)` failed before exercising the modified path on this desktop. D3D9/D3D9Ex runtime restoration is therefore **not verified here**.
- Focused `/O2` hand-anchor and knife-route tests: PASS. No game launch or headset test.

## One controlled MAIN session after review

Use the reviewed DLL in a backed-up, hash-verified candidate; retain accepted INI and assets. With a right-hand weapon and left TMD: raise TMD, hold L2, press R2, and rotate the right wrist; the gun and right glove should stay together without an R2 snap, and both eyes should agree. Take and swing the knife after acquiring TMD, then release it: one knife, intact glove and no floating fragments. Check `tmd-right-wrist final` for `Impulse=1` on L2 and `knife-hand: reserve TMD cuff joint` followed by `native joint ... tmdCuff=1` in both eyes. Any missing joint, one-eye draw, duplicate knife, gun displacement, broken reload, or stereo regression is FAIL; restore the accepted installation and report logs.

No merge recommendation until MAIN reviews the diff and the physical session passes.
