# Decal diagnostic — shader lifetime claim (2026-10-12)

## What the removed `GetPixelShader` census did

In build `B387611A…`, `NoteDecalShaderCensus` called:

```cpp
IDirect3DPixelShader9* ps = nullptr;
dev->GetPixelShader(&ps);
if (ps) ps->Release();
```

## D3D9 COM semantics (verified)

Per IDirect3DDevice9::GetPixelShader: the method **adds a reference** on the returned
interface. The caller **must** `Release()` that reference. The device keeps its own
reference to the bound shader; a balanced Get/Release pair does **not** unbind or destroy
the shader the device is using.

So the earlier claim that this pattern “destroyed the game's currently bound shader” was
**not supported by normal D3D9 ownership rules**. There was no double-Release and no
Release of a pointer the mod did not own from GetPixelShader.

## Startup crash on build `83061AAD…` / `47A2A81F…` (fixed)

`HashPixelShaderBytecode` called `Fnv1aBytes(bytes, size)` after `GetFunction` without
capping `size` to `sizeof(bytes)`. D3D9 can return **`D3DERR_MOREDATA`** with `size` larger
than the buffer; hashing with that size **reads past the stack array** and can trigger MSVC
“Runtime Error! … terminate in an unusual way” at startup.

Fix: only hash when `SUCCEEDED(gf) && size > 0 && size <= sizeof(bytes)`. Guard
`g_origCreatePS` / `g_origSetPS` before forward. Install PS hooks only when
`DecalPassDiag=1` or `StereoDecalPassFix=2`.

## What could still explain “shadow follows hand” (unproven)

- **Correlation, not causation** — same session as other branch changes; no A/B isolating
  only the census.
- **Reentrancy / ordering** — querying device state mid-draw is unusual; any wrapper bug
  would need a captured failing trace (not observed in code review).
- **Pre-existing shadow sampling** — `ShadowMode=3` with census mask `0x0` remains a
  separate receiver-path issue (see `SHADOW_SFX_HANDOFF_20261008.md`).

## Safe replacement (shipped on branch)

- Hook **CreatePixelShader** once: FNV-1a hash of bytecode at creation.
- Hook **SetPixelShader**: track bound pointer + hash (no Get/Release in draw path).
- **DecalPassDiag** logs `psHash=0x…` with RT format, hook source, prim type/count
  (including large indexed draws on G16/HDR).

## Fix modes (`StereoDecalPassFix`)

| Value | Behaviour |
|-------|-----------|
| 0 | Off (default) |
| 1 | Legacy G16R16 shape gate (v2 — **classified=0** in PSVR2 log; do not treat as validated) |
| 2 | Scene-colour `TRIANGLELIST` / 2 prims **only** when `DecalTargetPsHash` matches bound PS hash |

Mode 2 requires a hash from `decal-diag` evidence — never guess.
