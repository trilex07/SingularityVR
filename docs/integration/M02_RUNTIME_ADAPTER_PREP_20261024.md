# M02 runtime UI adapter — preparation and first default-OFF slice

Branch base: `48fba76`, separate from the Valkyrie/Volk insertion comfort branch. No Steam changes.

## Verified existing material

- Neutral atlas: 91 sprites; localized PL and EN: 36 each. `python asset-tools/test_ui_atlas_qa.py` passes bit-exact source/atlas extraction, alpha, UV and localization checks.
- `docs/integration/MASTER_INTEGRATION_MANIFEST.json` records the three atlas SHA256 values. Current HUD/wheel/locker uses its own `g_hudAtlasTex` (`d3d9.cpp:2559`) and drawing paths around lines 29431–30173. The M02 atlases are not bound there.
- The accepted menu, GFx, laser, wheel and VR Settings geometry must remain authoritative for hit-testing. Loading an atlas is not evidence that it is drawn in both eyes.

## Safe first runtime slice

Implement `UiRemasterRuntime=0` under `[Render]`; default and missing key OFF. When OFF, do not load M02 textures, alter native GFx, replace the HUD atlas, or change any draw/input route. When ON, start with a single non-interactive decoration inside the existing VR Settings panel, using its already-computed per-eye rectangle. The adapter must use D3D9 state capture/restore (texture, sampler, blend, depth, scissor, viewport, render target), create resources outside the hot draw loop and release/recreate them on Reset. Failure to load or create either atlas must leave the accepted UI untouched. Do not reuse old SIDE button-role assumptions; text labels must come from the current producer mapping.

Before expanding beyond that slice, add a CPU layout test for both eyes at 5264x2684 and a D3D9/D3D9Ex draw-state test showing two submitted eyes and restored state. MAIN then validates one screenshot per eye plus laser/R2 alignment in a short PSVR2 session. Only after that gate should the adapter cover main/pause menu, wheel, or localized prompts. Native GFx interaction must remain active throughout. No M02 asset should be mixed into the M01 magazine candidate.

Status update: the first runtime slice is implemented on this branch. It draws only the neutral settings plate; PL/EN atlases and the other UI surfaces remain out of scope. See `docs/handoffs/side-m02-runtime-first-slice-20261024.md` for build/test evidence and the MAIN physical gate. This is not PSVR2-accepted or a full M02 deployment.
