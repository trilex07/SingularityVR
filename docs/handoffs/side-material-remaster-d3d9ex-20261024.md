# SIDE material remaster bridge, first four maps

Branch `feature/side-material-remaster-d3d9ex` starts at accepted combined M02/M09 source `e379b7af`. Steam and MAIN's checkout were not changed. This is L2/offline evidence, not a black-material or PSVR2 pass.

## Evidence and correction

- The accepted source already has **one slot-65 owner**: MAIN's `Hook_SetTexture` classifies the original texture, then optionally calls `SingularityTextureBridgeV1` with the saved native forwarder. The old standalone MinHook path remains disabled when both bridge flags agree. No second slot-65 hook was added.
- MAIN's `CreateDeviceEx` path converts managed game textures to DEFAULT plus SYSTEMMEM shadows. The actual combined fixture proves raw DEFAULT `LockRect` fails, while the existing MAIN wrapper hashes the original bytes through its shadow. The injector's Ex replacement factory uses DEFAULT plus SYSTEMMEM/`UpdateTexture`, and its plain-device factory uses MANAGED. DXT1/DXT5 pixel/alpha readback and Reset/ResetEx pass offline.
- A successful *external* `UpdateTexture` could change a source after the bridge cached its hash/replacement, without passing through source `LockRect`. MAIN now invalidates source-owned positive/negative stamps after that native update. Internal shadow uploads already invalidate at the writable Lock; the hook skips a second invalidation while `g_stageLock` is held, avoiding a COM Release/deadlock cycle. The combined fixture exercises both paths.
- `priority_material_mode` adds a narrow startup-only gate in `texture_override.ini`: `0` = existing accepted base behavior (default), `1` = log exact four matches but keep the accepted base DDS, `2` = use four private remaster DDS. With `mode_apply=0`, all paths become full log-only. Keep `remaster_apply=0` so the older 24-map experiment stays off. Mode 2 falls back to the original game texture if its validated DDS cannot be loaded; it never borrows a different target.
- Exact first-four identities are `AR_shotgun_d`, `AR_shotgun_n`, `inview_glove_d`, `inview_glove_n`. Diffuse/normal only. Stage-protected menu sources, projected shadows, render targets, depth and dynamic textures continue to bypass substitution. Source-owned COM private data releases the replacement with the original; no pointer-only replacement cache is used by this route.

## Private assets

`python tools/textures/prepare_priority_materials.py --output <private-directory-outside-git>` extracts only the four verified source files from `0dcdcd5c6dc677573df24ea5616b69ef71ebb8ff`, checks SHA256, DXT format, true 1024x1024 mip0 and all 11 mip levels, and writes the runtime names `<identity>.dds`. It refuses Git-checkout and Steam destinations and different pre-existing files. No DDS or comparison PNG is committed to this branch.

The four SHA256 values, in the identity order above, are:

1. `6340E3245854C8CFA9D32DC9DF3B82E7BA41DCF035B0C37219C08FE9A9983C65`
2. `76A7D6EE4F44682880F28B7E1ED5C76130FE6015162D1AF2CF0872401B770834`
3. `E702E96CBAE6AE73B24BF8E01969D0AF41F15BCF1E04CCAB2B8508435FAFF026`
4. `754C1B0617DB838421C6E4B85FE0A9FBDFCCFD14B7DD886A8CF516497C2CF3CA`

Total DDS bytes: 3,495,832. This is a small first A/B, not the full remaster batch. Do not import the game-derived files from `feature/material-remaster-v2` into public main.

## Offline checks

- `spikes/texture_cache/build.ps1`: PASS, x86 PE32 `/O2`; exact scope/mode, native resource metadata, forwarding, actual plain and Ex DXT1/DXT5 pixel readback, Reset, ownership and protected-source bypass.
- `tools/test-texture-bridge-main.ps1`: PASS after adding `ole32.lib`/`windowscodecs.lib` to its link line for the current UI source. Actual translated DEFAULT lock failure, shadow read/hash, single forward, source original identity, external `UpdateTexture` invalidation, ResetEx and zero live shadows verified.
- Production `spikes/view_matrix/build.ps1 -GameBinaries <this-worktree>`: PASS, x86 PE32 `/O2`. No install.
- Existing M02 geometry/switch fixtures and D3D9/D3D9Ex pixel/state fixtures: PASS (both eyes 2240/2240 sampled pixels; state restored).
- Synthetic hot-path benchmark: five rounds of 2,000,000 noncandidate binds; stamped native lookup 0.0132–0.0137 microseconds/call versus descriptor/shape path 0.0505–0.0512 microseconds/call. **Not game frame time or GPU cost.**
- `tools/textures/prepare_priority_materials.py` read and verified the four private candidates; no committed DDS.

Offline DLL SHA256: `d3d9.dll` `37B3B4E07E90BFC4EBFF65D0027B4282BCD4F6C06E13251F4A8992E692AD6449`; `dinput8.dll` `9E69C9C1D4DBB858B7DBB1CFC57683976E0E24859FFA9A72BEBC8F187D670647`. These local builds precede the commit, so MAIN must rebuild exact committed source and record fresh provenance before any Steam test.

## MAIN's next bounded gate

Review DLL/source and prepare a guarded delta; keep all accepted INI keys and 107 base DDS intact. In the first session use `TextureBridgeOwned=1`, `bridge_owned=1`, `enabled=1`, `mode_apply=1`, `remaster_apply=0`, `priority_material_mode=1`: require four named exact-hash receipts and zero `remaster=1` substitutions. If any four are absent, stop and inspect the missing identity instead of guessing a different mip. In a separately backed-up cold start, change only `priority_material_mode=2` and supply the four hash-verified private DDS in `texture-override-remaster`; compare the same shotgun/glove scene to mode 1. Reject if black materials, wrong menu/shadows, stereo difference, D3DERR_INVALIDCALL, leak growth or p95 frame delta above 0.5 ms. Archive logs and hashes; restore accepted DLL/INI/assets on failure.

Centurion `handgun_d/n` captures are lower mips; sniper diffuse is lower mip; no verified Valkyrie material pair is in this first set. Do not claim these three weapon materials remastered until true mip0 identity and source data are proven. `feature/material-remaster-batch2` remains a separate, unenabled asset set.
