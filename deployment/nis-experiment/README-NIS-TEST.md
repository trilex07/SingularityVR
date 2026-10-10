# NIS experiment deployment — DO NOT copy over the normal gameplay build

Experimental prototype from branch `research/nvidia-vr-upscaling`.
Official NVIDIA Image Scaling v1.0.3 (MIT, see `nis/licence.txt`).

## Contents

| File | Notes |
|------|-------|
| `d3d9.dll` | x86 PE32, `/O2`. Git-ignored build output, ships only in this folder |
| `openxr_loader.dll` | Win32 loader from the OpenXR SDK (git-ignored) |
| `SingularityVR.ini` | Dev ini + `[Upscaling]` defaults (`Mode=Off`) |
| `nis/NIS_Main.hlsl` | **Required at runtime.** The DLL compiles it once via `D3DCompileFromFile` from `<dll-dir>\nis\NIS_Main.hlsl`. Keep the `nis\` folder beside the DLL |
| `nis/licence.txt` | NVIDIA MIT license notice |

Prototype DLL (this build):
- PE32 x86 (`0x014C`), `/O2`, 622,592 bytes
- SHA-256: `C20EC3A9F37DECD6453896BF59125713ED8DCA782DBE1F3CADC3E6A2BBC1E10D`
- Shaders: embedded official bytecode (primary). `nis/NIS_Main.hlsl` ships
  as fallback only and is compiled only if embedded creation fails.

## Install (isolated game copy only)

1. Use a **separate copy** of the game for this experiment. Never overwrite
   the known-good gameplay `d3d9.dll`.
2. Copy the whole folder contents (including `nis\`) next to `Singularity.exe`.
3. Launch once with `Mode=Off` to confirm the game still boots and VR works.

## A/B test sequence (ONE PSVR2 session, 90 Hz)

1. **A — native Off.** `[Upscaling] Mode=Off`. Baseline: clarity, text,
   distant geometry, weapon, hands, aliasing, frame stability.
2. **B — native + sharpen.** `Mode=NISSharpen`, `Sharpness=0.35`. Same
   resolution, NIS sharpening only. Press `N` in-game to flip back to Off
   without relaunching (one log line per change).
3. **C — NIS 0.85.** `Mode=NIS`, `RenderScale=0.85`. **Relaunch required**
   (UE3 resolution comes from the command line). NIS reconstructs each eye
   to the recommended output.
4. **D — NIS 0.77.** `Mode=NIS`, `RenderScale=0.77`. Relaunch required.
   Eye input ≈ 2028×2068 → output 2632×2684 per eye.

Suggested sharpness presets: soft `0.2`, balanced `0.35`, sharp `0.5`.
Do not start above `0.5` — VR ringing is unpleasant.

Optional experiment (not default): `RenderScale=1.0`, `OutputScale=1.2`
produces a larger-than-recommended swapchain. It does NOT create native
detail; it only tests whether the compositor path tolerates it.

## What to record per step

- Source SBS resolution (log: `auto-resolution: injecting -ResX=...`)
- Output eye resolution (log: `XR swapchains (2) ...`)
- `NIS upscaling [MEASURED GPU]` line: GPU mean/peak ms, stage-copy ms
- `frame copy` + `frame budget` lines: overall ms vs 11.11 ms budget
- Subjective: text readability, distant detail, shimmer, weapon/hands

## Rollback

Delete this folder's DLLs from the game copy and restore the normal
gameplay `d3d9.dll` + ini (release zip). This experiment never touches
`C:\Users\Trilex\vr-mods\SingularityVR` gameplay branches, saves, or
SteamVR settings. `RenderScale` only changes the UE3 command line the mod
injects; it never modifies `steamvr.vrsettings` or global supersampling.

## Base

- Branch: `research/nvidia-vr-upscaling`
- Base commit at prototype time: see `git log` (`feat(upscaling)` commits)
- Gameplay branch untouched: `fix/vr-menu-input-grip-performance`
