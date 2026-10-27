# Gloves/fingers integration checkpoint — 2026-10-09

HEAD=c421375+working-tree
DLL_SHA=3A4F88AE25A73EA61524DAFBD8D2E12E01C0293590FCEA5607D14425D64FBC7E
Frame=static unit/integration validation
Eye=shared palette path (colour/depth and stereo restore)
WorldScale=80 (source INI)
CurrentPass=foreground arms seam
MenuState=not applicable during palette tests

## Asset verification

research/native-hands-20261007/mesh-evidence.json confirms the production section maps:

- noTMD section 0: 2586 vertices / 4830 triangles, 35 slots
- noTMD section 1: 366 vertices / 674 triangles, 6 slots
- TMD section 0: 2285 vertices / 4402 triangles, 36 slots
- TMD section 1: 4340 vertices / 7704 triangles, 15 slots
- TMD section 2: 37 vertices / 17 triangles, 2 slots

The implementation uses those explicit bone-index maps and the verified TMD reference parents; it does not assume slot identity.

## Runtime integration

- The first census was run with `GloveIsolate=1`; it records would-collapse slots once per second and leaves native arm geometry intact.
- The verified section maps and deterministic census are complete, so the deployed configuration now uses `GloveIsolate=2`. It collapses only `BoneRole::Collapse` entries onto the live wrist center. Unknown `Review` entries and TMD device bones remain unchanged.
- Finger input is sampled once per OpenXR frame from trigger/value, analog squeeze/value (with boolean fallback), trigger/stick touch, then passed through BuildInput and SmoothCurls.
- FlexDelta is applied after the existing wrist/arm IK delta, to each verified finger slot and its IsDescendantOrSelf descendants. Palette writes are restored after the draw/eye pair.

## Tests

- tools/reference-tests/hands/glove_finger_test.exe: PASS
- all existing spikes/view_matrix/tests/*.exe: PASS
- git diff --check: PASS

Physical headset validation remains the visual gate for checking the collapse shape and finger curl, but the code path is now enabled.



## Deployment

- DLL: `C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\d3d9.dll`
- PE32: `true` (`Machine=0x014C`, `OptionalMagic=0x010B`)
- Installed SHA-256: `3A4F88AE25A73EA61524DAFBD8D2E12E01C0293590FCEA5607D14425D64FBC7E`
- Install report: `deployment/install-reports/20261009-143832.json`
- Rollback: `deployment/install-backups/20261009-143832`
- Installed INI contains `GloveIsolate=2`, `IndependentHandsPrototype=1`, `WorldScalePct=80`.

