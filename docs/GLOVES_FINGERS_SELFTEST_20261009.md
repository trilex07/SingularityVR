# Gloves/fingers self-test — 2026-10-09

HEAD=f7be176
DLL_SHA=40CE5E13796BCDB2993537FA50456AC714E3148D95A76C334DF5B7DEB7ECB097
Frame=XR simulator frame 1739 capture; runtime foreground windows reached 90.0 fps
Eye=projection layer, 2 views; left/right non-black 97.09% / 97.09%
WorldScale=80 (installed `SingularityVR.ini`)
CurrentPass=foreground first-person mesh seam; 21-mesh foreground census, later peak 136 split draws
MenuState=main menu in scene capture; `btn a press 250` accepted by simulator

## What was tested

1. Steam path: `steam://rungameid/42670` launched the installed game process and wrote
   `C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix.log`.
2. The installed binary matched the build (`d3d9.dll` SHA-256 above), and the texture
   injector remained present (`dinput8.dll` SHA-256
   `939E46948B60CC64A029E17480364A65C09E8FAEAA185DE6A33D15CF45EF21F8`).
3. The dvr-xrsim runtime reached `FOCUSED`, `actionsAttached=true`, 90 Hz and two
   projection views. The following atomic input batch was accepted (`cmdSeq 0 -> 7`):
   `head rot 14 0 0`, independent left/right grip poses, left/right squeeze and
   left/right trigger values.
4. The compositor capture is saved at:
   - `C:\Users\Trilex\vr-mods\deployment\selftest-20261009\scene_sbs.png`
   - `C:\Users\Trilex\vr-mods\deployment\selftest-20261009\scene_left.png`
   - `C:\Users\Trilex\vr-mods\deployment\selftest-20261009\scene_right.png`
   - `C:\Users\Trilex\vr-mods\deployment\selftest-20261009\scene.json`

## Observed evidence

- The runtime log reports both sticks ACTIVE, both poses valid, `sync-fail 0`, and
  trigger samples `LEFT 0.40` / `RIGHT 0.85`.
- The same run reports `handsymmetry ... valid 1/1` after the head was rotated 14°,
  so the two hand pose streams were not collapsed into one controller by the input
  layer.
- The loaded scene reaches `perf: 90.0 fps (11.11 ms/frame)` with `draws/frame peak
  136` and `split 136`; zero-copy upload is 0.03 ms in that window.
- The scene capture has one projection layer, both eyes non-black, and equal 63 mm
  eye separation. The menu is visible in both eyes and the white pointer/laser is
  present near the menu plane.
- `btn a press 250` was accepted (`cmdSeq 8 -> 9`). A post-click capture was not
  claimed because the harness ended before the next frame became available.

## Limitation and next step

The run exercised the foreground pass and the verified glove/finger code is compiled,
but no `GloveCensus ... left=X/Y right=X/Y` line was emitted. The deterministic census
and verified SP section maps classify the arm-flesh slots, so the deployed configuration
now enables `GloveIsolate=2`; the runtime line remains a diagnostic check for the next
headset pass. The static deterministic test remains passing:

`PASS: glove isolation + finger posing`

The menu/scene screenshot is suitable for the next user headset check, while visual
finger pose and true arm appearance still require a headset capture after the runtime
has entered gameplay.
