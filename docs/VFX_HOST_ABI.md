# VFX HOST ABI (Variant A) — muzzle flash / barrel smoke / blood

Frozen contract between the production `d3d9.dll` (the **HOST**) and a separate,
optional **`SingularityVfx.dll`** (the **PLUGIN**). This is the "Variant A"
split: the host keeps everything that needs the engine or D3D9, and the plugin
holds only effect logic and atlas metadata.

Branch: `feature/vfx-host-abi` (worktree `C:\Users\Trilex\vr-mods\SingularityVR-vfx-host`),
based on production `793087c`.

## Why this split

* The plugin author iterates on effects without touching `d3d9.cpp`.
* The producer of bullets/blood/draw is the one place that already knows the
  muzzle transform, the hit location and the per-eye matrices.
* Removing `SingularityVfx.dll` is a complete kill-switch: the host contains no
  effect code and every seam is a no-op.

## Files

| file | side | contents |
| --- | --- | --- |
| `spikes/view_matrix/vfx_plugin.h` | shared | frozen ABI: version, value types, `VfxFireEvent`, `VfxHitEvent`, `VfxDrawItem`, clip ids, export names |
| `spikes/view_matrix/vfx_host.h` | HOST | `host::Plugin` loader + seams (`Load/OnFire/OnHit/Tick/Collect`), no D3D9 |
| `spikes/view_matrix/vfx_draw.h` | HOST | `host::VfxDrawer` + `VfxAtlas`: per-eye billboard quads, state save/restore, additive/alpha batching |
| `tools/reference-tests/vfx/vfx_host_test.cpp` | test | ABI layout, no-op seams, kill-switch |

## Frozen ABI (version 1)

Exports the plugin must provide (see `VFX_EXPORT_*`):

```
uint32_t VfxPluginAbiVersion();                       // must return VFX_PLUGIN_ABI_VERSION
bool     VfxPluginInit();
void     VfxPluginShutdown();
void     VfxPluginOnFire(const VfxFireEvent*);
void     VfxPluginOnHit(const VfxHitEvent*);
void     VfxPluginTick(float dtSeconds);
int      VfxPluginCollect(uint8_t renderPass, VfxDrawItem* out, int capacity);
```

All structs are plain fixed-width POD. `VfxDrawItem` is 40 bytes with frozen
offsets (asserted by the test); change it only by bumping the ABI version.

The host refuses a plugin whose `VfxPluginAbiVersion()` differs, or that is
missing a required export, and unloads it.

## HOST integration points (main agent)

1. `Plugin::Load()` once, after the device exists (or lazily on first fire).
2. In the existing fire path (`FireAmmunition`), fill `VfxFireEvent` with the
   **verified muzzle transform** and call `plugin.OnFire(...)`.
3. In the hit path (`SetFlashLocation` / confirmed flesh hit), call
   `plugin.OnHit(...)`; never spawn blood for non-flesh.
4. Once per frame: `plugin.Tick(dt)`.
5. **Inside the existing per-eye world draw** (the stereo duplication path — not
   beside it): `int n = plugin.Collect(VfxRenderPass::Gameplay, items, cap);`
   then `drawer.DrawItems(device, items, n, atlas, camRight, camUp)` for that eye.
6. Keep `HideMuzzleFx` (engine first-person flash suppression) — otherwise the
   original centred emitter comes back and doubles the effect.
7. Load the atlas (from the manifest below) into `VfxAtlas`; map `VfxClip` ids to
   texture + grid.

Do **not** call `Collect`/`DrawItems` for menu, shadow, depth-only or flat-cinema
passes — `Collect` already refuses them, and the host must not draw in those
passes either.

## PLUGIN contract (agent 2)

* Include `vfx_plugin.h` verbatim; do not modify it.
* Implement the exports above; keep all state internal; no D3D9, no engine hooks,
  no file I/O in the frame loop, no per-frame allocation.
* `VfxPluginCollect` fills `VfxDrawItem`s (kind, blend, clipId, frame, age,
  position, direction, size) and returns the count (<= capacity). Bounded pool.
* Ship an atlas manifest (next section) and a clipId -> UV mapping description.

## Atlas manifest (produced by agent 2)

Small 1024×1024 RGBA atlases (one for muzzle/additive, smoke/alpha, blood/alpha)
plus a JSON/compile-time table giving, per `VfxClip`: texture file, grid
`cols`/`rows`, `frames`, `fps`, pivot, and `blend`. The HOST builds `VfxAtlas`
from it. No PNG decode in the frame loop.

## Clip ids (frozen order)

`MuzzleFireSmall, MuzzleFireLarge, MuzzleFireHeavy, MuzzleFireEnergy,
SmokeWhite, SmokeBlack, BloodSplash, BloodHitAlt`. Append new clips at the end
and bump the ABI.

## Build / test

```
# host header test (x86, /O2, /std:c++17)
cl /nologo /EHsc /W4 /O2 /std:c++17 \
   /Fe:vfx_host_test.exe tools/reference-tests/vfx/vfx_host_test.cpp
```

Expected: `PASS: VFX ABI layout stable, host seams no-op without plugin, kill-switch clean`

The full `d3d9.dll` build needs the OpenXR SDK and is out of scope here.

## Non-goals / notes

* `vfx_draw.h` is a working skeleton: correct state save/restore, billboard math,
  additive/alpha batching. The host still owns FOV scaling and may switch blood to
  a hit-normal-aligned quad.
* No VFX DLL is deployed to Steam until the plugin's desktop tests pass and the
  host wires the seams on the production branch.
