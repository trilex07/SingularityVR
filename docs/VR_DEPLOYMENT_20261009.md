# SingularityVR deployment — 2026-10-09

The current branch is `fix/vr-menu-input-grip-performance` at commit
`7107d04`. The deployed DLL is the x86 PE32 build with SHA-256
`53BCD9CA3AECFF8236EEF1EB9F4CA8ED82AC32153D5176E35F053688B1224AF7`.

Deployment target:

`%STEAM%\steamapps\common\Singularity\Binaries\d3d9.dll`

The staging directory is `deployment/vr-recorder-20261009/`. It contains the
deployed DLL, the previous DLL, and INI rollback copies. The active INI keeps
`VfxSmoke=0`, `VfxScreenFlash=1`, `ShowControllerArms=0`, and
`VrShadows=0`; the Advanced page can enable VR shadow sampling for an isolated
headset A/B without changing the light-space mode.

Build checks passed with the x86 `/O2` script. The interaction, menu hit-test,
motion, transition, holster, wheel, runtime, VFX, and recorder tests passed.
The last Steam runtime log also confirmed the missing DirectInput mouse guard,
OpenXR initialization, VFX atlas load, and a 89.9–90.0 FPS desktop-HMD
diagnostic. A fresh optical check of shadows, muzzle placement, prompts, and
glove appearance still needs the user's PSVR2 session.
