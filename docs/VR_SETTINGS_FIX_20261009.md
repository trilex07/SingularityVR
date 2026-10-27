# VR Settings input and upscaler fix — 2026-10-09

The current x86 build fixes three panel problems reported during the Steam test:

- `RECORD SESSION ON/OFF` is now on the root `VR SETTINGS` page. It is a panel row, so it is
  activated with the right-controller laser and R2 rising edge; L3+R3 remains screenshot/recenter
  only.
- The captured native menu quad is suppressed while the mod-owned panel is open. The panel therefore
  stays in front of the game's menu instead of being covered by a stale menu copy.
- The panel laser no longer depends on the GFx hook being ready and always reads the right Sense
  controller trigger. This keeps the ray and R2 click available during the early pause/main-menu
  transition.

`ADVANCED` now contains `UPSCALER NIS OFF/NIS/NISSharpen`. The row changes `g_nis.mode` live and
writes `[Upscaling] Mode=Off|NIS|NISSharpen` when the panel closes. `RenderScale` still requires a
relaunch because it changes the UE3 source resolution; `NISSharpen` is runtime-switchable.

Validation:

- x86 PE32 DLL built with MSVC 18.8.2 and `/MD /std:c++17`.
- `test-pause-vr-settings.exe`: pass.
- `test-vr-interaction.exe`: pass (2400 menu ray/render/click samples and controller transitions).
- `test-vr-session-recorder.exe`: pass.
- `test-nis-upscale.exe`: 167 passed, 0 failed.

Steam deployment:

- `C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\d3d9.dll`
- SHA-256: `BCDBD72CBC695C4CB10D63CC0BAEBC114269B7A549535383A18C5F9D46507CFC`
- PE32: machine `0x014C`, optional header `0x010B`
- Existing `dinput8.dll`, `dinput8_orig.dll`, `texture_override.ini`, and 93 DDS overrides were preserved.
- Rollback backup: `deployment/install-backups/20261009-132140`

