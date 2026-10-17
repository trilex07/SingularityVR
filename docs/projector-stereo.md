# First-building projector: producer investigation

User observes the projected film near an eye edge when approaching the first
building's screen after the crash. The image does not remain on the canvas.
Status: BROKEN visually; responsible live shader UNKNOWN.

## Offline evidence

Read-only decompression of installed `SP_VI2_kismet01_a.xxx` and `_b.xxx`
finds `VI3_projector.mat_projector_living` as an imported material. In `_a`,
export 22 (`LightFunction`) has `SourceMaterial` referring to import -84.
Its outer is export 257 (`SpotLightComponent`), whose outer is export 259
(`SpotLightMovable`). These links come from the object tables and a validated
tagged SourceMaterial property. Generic property parsing of the component
itself fails; its transform/radius properties MUST NOT be inferred from the
partial parse. `tools/projector/inspect_projector_assets.py` reproduces the
validated ownership links without publishing original package contents.

Logical package SHA-256: `_a`
`3636911f20aaa556edd045e6ee0bac43a884509b69f1369d6eff7d4d9c927f03`, `_b`
`ca959b9c0dd93cac2a579ffd28fd0db3de54a778e9f10ee3f2727d613d5673e3`.
The second package independently has the same material ownership, via
export 22 -> 255 -> 257 and import -83 -> -118. Both observed packages pass
the read-only inspector. Scheduler PowerShell syntax and git diff checks pass;
its new two-cue mode still requires live execution. C++/DLL sources unchanged.

The same package imports `prop_furniture.Materials.movie_screen_leadin` and
contains a separate `fx_motes_spot_projector` particle system. Those are distinct
candidate producers. `SP_VI2_zone02a` also references the native movie-screen
mesh and textures. Names do not identify the actual GPU draw or prove which
pixels the user observed.

Working hypothesis: per-eye screen/depth reconstruction in the native light
function receives an inappropriate mono or HUD transform. Falsifier: the
projector contribution uses correct per-eye reconstruction and lands on the
same world surface, or the visible fault belongs to the separate beam/material
draw instead. No matrix, shader hash or register is selected yet.

The reviewed muzzle post-shot trace PID22704-2 contains 96 shaders but no CTAB
names matching ScreenToLight, LightFunction or WorldToLight. This is not a
projector capture and cannot justify a projector fix.

## Next bounded capture (MAIN, fresh start)

Use the already built, physically accepted muzzle candidate DLL `709F64A7...CE1FBA`
and its exact INI `38642BD6...D73B3E`; no renderer change for this measurement.
The muzzle fix remains enabled in this candidate. Restore destination remains
the existing accepted deployment profile (DLL `789749E5...F361B`, INI
`8B0B0B4C...CCFAF`), since code publication/profile promotion has not occurred.
Recheck live files/profile before a new lease. This projector branch starts
from origin/main and locally fast-forwards to the accepted muzzle source;
it does not change remote main or bypass the refused main push.

`tools/run-shadow-holster-sequence.ps1 -Mode Projector -GamePid <live PID>`
uses the existing one-frame trace plus SBS capture, twice per session, with
independent fresh pause/resume cues. It starts at the log's current end, pins
the game PID, times out, refuses an existing request, and copies completed
traces. No controller input or automatic movement is sent.

Headset route: enter the projector room and face its canvas while the film is
running. Open/close Pause, hold the view for 10 seconds. Move a little closer
or to the side where the misplaced image is visible, open/close Pause again,
hold 10 seconds, then exit normally. No chat switching; no need to fire.
One brief readback hitch per capture is possible.

Afterward: require complete trace/shader budgets, identify light-function
shader constants using CTAB and disassembly, inspect sampler/resource chains
and both-eye constants, and match candidates to the SBS image. Only then use
a narrowly scoped discriminator/correction. Do not reuse muzzle shader hashes
or apply the cinema/Bink presentation fix to this light function.
