# Shadow artifact — headset capture, analysis and test plan — 2026-10-09

Input: `C:\Users\Trilex\vr-mods\review-screens\capture_2026-10-08_*.jpg` (29 frames,
5264x2684 SBS). The frames are captured from the headset, so they are the
"headset-verified capture" that `fp_shadow.h` had been waiting on — that file's
SPECULATIVE note is upgraded to observed fact in this change.

Analysis was programmatic (pixel sampling, region crops, eye-by-eye comparison);
the crops under `review-screens\_crop\` are the evidence referenced below.

## What the artifact looks like

Two shapes, **both strictly one eye**, each with the other eye clean at the same
eye-relative region:

| Frame | Eye with artifact | Eye without | Signature |
|---|---|---|---|
| `14-42-37` | **LEFT** | right | a shadow-shaped silhouette on the floor filled with evenly spaced parallel diagonal bands |
| `14-12-54` | **RIGHT** | left | thin near-vertical dark lines crossing floor tiles *and* the paper debris on them |

Why this is an artifact and not scene content:

- Same floor, same world, same frame — one eye has the bands, the other does not.
  A floor texture or a real shadow is present in both (only displaced by disparity,
  which is far smaller than the crop).
- The bands keep a constant angle and spacing while the floor tiles visibly converge
  in perspective. A world-locked pattern would converge with them. **The pattern is
  screen-space.**
- In `14-12-54` the lines cross over geometry (tile edges, loose paper) rather than
  terminating at it, so they are not edges of an object.

## Why this matters to the existing research

`fp_shadow.h` recorded the per-eye square/block visual as *SPECULATIVE ... needs
headset-verified capture*. That evidence now exists, so the open question is closed on
existence and signature. **The FP-mesh hypothesis itself is still unproven** — the
capture does not say *which* pass emits the bands.

Two further data points line up with what the code already asserts:

1. `d3d9.cpp` run-44 note: *"The artefact is decals, not shadows — it vanishes with
   `High Quality Decals` off."* The band shape follows a shadow-like silhouette while
   behaving like a projected lookup, so "decal-like projected lookup" is the leading
   reading.
2. `d3d9.cpp` light-slot note: *"remapping a light matrix per eye shifts the shadow
   lookup and is exactly the blocky/flicker/one-eye corruption."* The capture is a
   single-eye corruption of exactly that kind, so a per-eye-remapped lookup matrix
   remains the mechanism most consistent with the evidence.
3. `SHADOW TEST = DROP MAP PASSES` removed ~800–840 classified shadow-layout draws and
   the pattern stayed (`SHADOW_SFX_HANDOFF_20261008.md`), so **generation** of the
   shadow map is not the source — it is the *lookup* side.

## Two code findings that are blocking the next step

### 1. `SplitColourTargetsOnly` excludes nothing on this game

Both the split decision and the census label filter on the same format whitelist:

```cpp
// d3d9.cpp (Hook_SetRenderTarget, and again in ReportRenderTargets)
scene = (d.Format == D3DFMT_A8R8G8B8 ||
         d.Format == D3DFMT_X8R8G8B8 ||
         d.Format == D3DFMT_A16B16G16R16F);
```

The live census shows **two** scene-sized surfaces already carry `A16B16G16R16F`:

```
2C81D060  4128x2208  A16B16G16R16F  41520 draws  <- SPLIT   (HDR scene colour)
2C81CA60  4128x2208  A8R8G8B8       20280 draws  <- SPLIT   (LDR scene colour)
2C81CCA0  4128x2208  A16B16G16R16F  16935 draws  <- SPLIT   (the extra)
```

so the whitelist admits the extra along with the legitimate HDR target. The feature's
own logged intent is *"split ONLY the two scene colour targets — excludes the third"*;
**it cannot deliver that**, because format cannot tell the two float targets apart.
It has also only ever been logged as `=0` — never enabled.

### 2. …which makes the constant dump unreachable

`g_dumpShadowConsts` fires only when `g_rtIsScene == 0` for a scene-sized target —
i.e. only for a target the filter above *excluded*. Since nothing is excluded, the
condition can never hold and the dump that would reveal the reconstruction matrix has
**never run** (verified against `%LOCALAPPDATA%\SingularityVR\view_matrix.log`). The
investigation has been stalled behind an unreachable diagnostic.

Note: simply fixing the whitelist so the extra target is excluded is **not** by itself
the fix. Run 41 recorded that excluding that target from splitting *"moved the artefact
from one eye to both"* — the pass is wrong either way, only the number of eyes changes.
What is needed is the per-eye correctness of the pass, which is what the dump is for.

## Test plan — one variable at a time

All rows are on the ADVANCED page. Same room, same weapon, same spot, L3+R3 capture at
each step. Restore to baseline between steps.

**Baseline:** `ShadowMode = FULL SPLIT`, `VR SHADOWS = OFF`, `SCREEN FX TEST = OFF`,
`SHADOW TEST = NORMAL`. (The `SHADOW_SFX_HANDOFF` safe baseline is `ShadowMode=1` +
`VrShadows=0`; the deployed INI currently has `VrShadows=1`, which is *not* that
baseline — put it back before comparing anything.)

| # | Row → value | What a positive result proves |
|---|---|---|
| 1 | `VR SHADOWS` → **OFF** | bands vanish ⇒ the projected-shadow sampling texture is the emitter (it is neutralised in `Hook_SetTexture` when off). |
| 2 | `SHADOW` → **3 PROTECT LIGHT** | bands vanish ⇒ per-eye remapping of the light matrix was the cause — this is the mode written specifically against the blocky/one-eye signature. |
| 3 | `SCREEN FX TEST` → **ON** | bands vanish ⇒ the third scene-sized target *is* the emitting pass; bands remain ⇒ it is ordinary scene draws and that surface is something else. |

Step 3 is the one `SHADOW_SFX_HANDOFF` left open, and it costs one button press.

## What must not be done

Taken from the repo's own record, so the same ground is not re-trodden:

- Do not remap light-space matrices by guessed register numbers
  (`SHADOW_SFX_HANDOFF_20261008.md`).
- Do not revive 512x512-R32F map detection / texture substitution — it was measured
  wrong (`fp_shadow.h`).
- Do not change `ShadowMode` and `VrShadows` in the same test; that is not a valid
  comparison (`SHADOW_SFX_HANDOFF_20261008.md`).
- Do not treat "exclude the third target" as a fix on its own — run 41 already showed
  it moves the artifact from one eye to both.

## ~~Fix attempt 1 — `ExtraCamReg=13`~~ → **RETRACTED (same day)**

> ### ⛔ This attempt was already run — and failed. Do not re-arm `13`.
>
> `SingularityVR.ini.example` (committed) records **run 43** having tried exactly this:
>
> > *"Setting this to 13 remaps it too. Result: **geometry stretched to the edge of the
> > screen AND the shadows were unchanged.** … the shadows not moving at all argues
> > **c13 is NOT the carrier** for the mis-projected shadow geometry, so **the shadow's
> > cause is elsewhere.**"*
> > `;   13 = also remap vs c13 (reproduces the run-43 failure)`
>
> **What went wrong in the analysis below.** The claim *"never enabled before"* was
> verified against the **INI files** and the **runtime log** — both genuinely `-1` —
> but **not** against `SingularityVR.ini.example`, which is where trial history is
> recorded for this project. Absence from current configuration is not absence of a
> past experiment. Every supporting fact below remains true (c13 is a real unremapped
> camera matrix; `mono-fallback=0` normally), but they established *c13 is unremapped*,
> not *c13 carries this artifact* — and run 43 had already falsified that second step
> **by direct experiment**. Experiment beats inference.
>
> **Net:** arming `13` costs a visible regression (geometry stretched to screen edge)
> and buys no shadow change. Deployed INI is back to `ExtraCamReg=-1`.
>
> **Still open:** the one-eye shadow-floor artifact. Run 43 only ruled out c13; §"Two
> code findings that are blocking the next step" and the test plan above were **not**
> invalidated. New experiments may supersede run 43, but must be recorded in
> `.example` before being treated as evidence.
>
> Retained below as the analysis trail.

### The lead (historical — superseded by the retraction above)

### The lead

`d3d9.cpp` run-43 block (line ~11048) describes this artifact almost verbatim:

> *"The run-42 dump found a **second view-projection at vs c13**: identical projection
> columns to c0-c3, but a world-space translation (c16 = 5179, -1738, 26024) where c3 is
> translated-world and small. **Geometry riding it is never remapped, keeps full-frame
> coordinates, and its shadow-pass output lands split across the seam.**"*
> *"…one cached slot is the whole fix."*

That register is controlled by `[Render] ExtraCamReg`, which reads `-1` (off) by default:

```cpp
int g_extraCamReg = -1;
g_extraCamReg = GetPrivateProfileIntA("Render", "ExtraCamReg", -1, path);   // ~line 25460
Log("ini: ExtraCamReg=%d (%s)", ..., "off - only the scan-locked register is remapped");
```

### Evidence chain

| # | Finding | Source |
|---|---|---|
| 1 | `ExtraCamReg` absent from both INIs → default `-1` → **off** | deployed INI + worktree INI |
| 2 | `ini: ExtraCamReg=-1 (off - only the scan-locked register is remapped)` | `view_matrix.log` (both old and current sessions) |
| 3 | The game's own register scan selected **c13**: `** c13 ROW w=+0.0000 fwdLen=1.0000 dotFwd=+1.0000 shape=ok hits=2 zero at camActor` (the `**` marks the chosen candidate; the COL reading is `shape=BAD`, so ROW is the correct convention) | `view_matrix.log` scan block (captured before the log was rotated at 18:55) |
| 4 | Only the scan-locked register (c0) is ever remapped — *"camera matrix registers tracked: … is 1 by construction, not by measurement"* | `d3d9.cpp` ~11050 |
| 5 | `mono-fallback` is **0** in 129 of 134 gameplay windows (rare 1429/153/3116/1 spikes are load/menu transitions) → the remap *is* normally active, so "nothing is being remapped" (the cheap H1 hypothesis the code warns about) is **not** the steady state | `view_matrix.log` perf lines |

### Why this matches the capture

- *"lands split across the seam"* = content positioned in full-frame coordinates inside a
  half-frame eye buffer → visible in **one eye only**, matching both captured frames.
- The artifact's eye **alternates between frames** (`14-42-37` left, `14-12-54` right),
  consistent with which transform register (c0 vs c13) a given pass happened to ride that
  frame — a fixed-cause bug would not alternate.
- *"its **shadow-pass** output"* matches a shadow/decal-shaped silhouette on the floor,
  and `SHADOW TEST = DROP MAP PASSES` not removing it: dropping *generation* does not
  touch the receiver draw that rides c13.

### What was NOT chosen, and why

- **`ShadowMode=3`** — its census is provably dead: the mark loop runs only for
  `s < g_camMatUsed` (1 = c0) and exempts `reg != g_lockedReg` (c0), so the mask is
  always `0x0`. Removing the exemption would mark the *only* slot → `nSlots==0` →
  mono-fallback → stereo dies. It cannot be fixed incrementally; it needs multi-register
  camera tracking, which is a redesign, not a fix.
- **Excluding the third scene-sized target** — run 41 already showed it only moves the
  artifact to both eyes.
- **Fixing `SplitColourTargetsOnly`'s format whitelist** — `D3DSURFACE_DESC` offers no
  field that distinguishes the two `A16B16G16R16F` surfaces (same format, size, pool,
  usage). Any classifier would be a guess.

### The change (ONE variable)

`ExtraCamReg=13` added to `[Render]` of:

- `C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\SingularityVR.ini`
  (deployed — **this is the one the game reads**)
- `SingularityVR-shadow-fix\spikes\view_matrix\SingularityVR.ini` (source copy)

**No rebuild required** — the deployed DLL already parses the key (proven by the log line
above). Takes effect on next game launch.

Why 13 is safe by construction: run-43 deliberately does *not* run this register through
`ApplyProjection` — *"Eye offset and half-frame remap only — the minimum that could fix
the seam split, so a positive result means something specific."* The forced VR frustum
belongs to the verified register, so this cannot break it. Note the run-43 author's own
caveat: c13 is *inferred* to be a camera matrix from its shape, and the scan's
`zero at camActor` (rather than a light position) is what supports that inference.

### Test

1. Relaunch the game (INI is read at load).
2. Same room / same spot as the captures, L3+R3 screenshot.
3. Compare against `review-screens\capture_2026-10-08_14-42-37.jpg` and
   `..._14-12-54.jpg`.

**Positive:** bands gone in both eyes ⇒ confirmed.
**Negative:** bands unchanged ⇒ c13 is not the emitter; next step is the constant dump
(needs a classifier for the two float targets) — do not stack another variable on top of
this one first.

**REVERT:** set `ExtraCamReg=-1` (or delete the key) in the deployed INI.
