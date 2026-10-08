# `asset-tools/texture-injector` — standalone first-person texture override (research)

A small, self-contained **32-bit `dinput8.dll` proxy** that substitutes
pre-upscaled DDS files for specific first-person textures at runtime.

It is **not** the VR mod and does **not** touch `d3d9.cpp`, the production
worktree, the game packages, `coalesced.tfc`, or the executable. It co-exists
with the VR `d3d9.dll` because it is injected through a different import
(`DINPUT8!DirectInput8Create`, which the EXE statically imports and for which no
`dinput8.dll` ships in `Binaries`).

Status: **verified working.** A game launch produced
`SUBSTITUTE inview_glove_n (1024x1024 -> 2048x2048)` and
`SUBSTITUTE inview_glove_d (1024x1024 -> 2048x2048)` with hashes matching the
extracted originals exactly.

## How it works

1. `dinput8.dll` loads at process start (static import) and forwards
   `DirectInput8Create` to a local copy of the real DLL, `dinput8_orig.dll`.
2. A worker thread hooks `Direct3DCreate9` / `Direct3DCreate9Ex` in the loaded
   `d3d9.dll`, then `IDirect3D9::CreateDevice` (vtable 16), then
   `IDirect3DDevice9::SetTexture` (vtable 65) — MinHook, chaining on top of the
   VR mod's own hooks.
3. On each `SetTexture`, for a texture that is 1024×1024 DXT1 (i.e. a candidate),
   it locks mip 0 read-only and computes an **FNV-1a 64 hash of the compact DXT
   bytes**. If the hash equals a target's known original hash, it creates a
   2048×2048 replacement texture from the configured DDS and binds that instead.
   Classification is cached per texture pointer.

## Deploy / restore

```powershell
# build + install (adds dinput8.dll, dinput8_orig.dll, texture_override.ini,
# texture-override\*.dds to the game's Binaries; nothing is overwritten)
powershell -ExecutionPolicy Bypass -File asset-tools/texture-injector/build.ps1 -Install

# remove everything and return to vanilla
powershell -ExecutionPolicy Bypass -File asset-tools/texture-injector/restore.ps1
```

## Config (`texture_override.ini`, next to the DLL)

| key | meaning |
| --- | --- |
| `enabled` | 1 = active, 0 = loaded but idle |
| `mode_apply` | 1 = substitute, 0 = **log only** (no visual change) |
| `texturesdir` | folder next to the DLL with the replacement `.dds` |

`texture_override.log` records every candidate hash and every substitution, so
`mode_apply=0` can confirm the match before enabling the swap.

## Targets

| texture | usage | original | replacement |
| --- | --- | --- | --- |
| `inview_glove_d` | first-person glove/hand diffuse | 1024² DXT1 | 2048² DXT1 (candidate A, Lanczos) |
| `inview_glove_n` | first-person glove normal | 1024² DXT1 | 2048² DXT1 (candidate A, renormalised) |

## Risks / notes

* Substitution matches by **content hash**, so it cannot hit an unrelated
  texture. A mismatch simply logs and does nothing.
* It binds a larger texture than the engine created; sampling is UV-based so
  this is transparent for these assets, but it is the main technical risk and the
  reason this stays a separate, removable test tool.
* Replacement DDS files are upscaled derivatives of game art and remain
  **local only** (never committed).
