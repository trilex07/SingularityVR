# HANDOFF — shadow-floor artifact: `[Render] ExtraCamReg` — **RETRACTED, see correction** (2026-10-09)

> ## ⛔ CORRECTION (same day) — the recommendation below was wrong; do not arm `13`
>
> `SingularityVR.ini.example` (committed) records **run 43 already tried this**:
>
> > *"Setting this to 13 remaps it too. Result: **geometry stretched to the edge of the
> > screen AND the shadows were unchanged.** … the shadows not moving at all argues
> > **c13 is NOT the carrier** for the mis-projected shadow geometry, so **the shadow's
> > cause is elsewhere.**"*
> > `;   13 = also remap vs c13 (reproduces the run-43 failure)`
>
> **What I got wrong.** The body of this document claims `ExtraCamReg` was *"never
> enabled before"*. That was checked against the **INI files** and the **runtime log**
> — both genuinely `-1` — but **not** against `SingularityVR.ini.example`, which is
> where historical trial results are recorded. Absence from current config is not
> absence of a past experiment. The `.example` record is a direct experiment; my
> "c13 drives the seam artifact" line was an inference from it. **The experiment wins.**
>
> **Consequence:** arming `13` produces a *visible regression* (geometry stretched to
> screen edge) with *no* shadow benefit. Deployed INI returned to `ExtraCamReg=-1`.
>
> **Status:** run 43 stands. The one-eye shadow-floor artifact's cause is **elsewhere**
> and remains open — see §4 (the two provable blockers) and the capture doc's test
> plan, which was never invalidated. A new experiment may supersede run 43, but it must
> be recorded in `.example` before it is treated as evidence.
>
> Everything below is retained as the analysis trail, with this correction attached.

---

**Scope:** the one-eye banded/hatched shadow artifact on the floor, captured in
`C:\Users\Trilex\vr-mods\review-screens\` (29 SBS frames). Side-worktree work on
`feature/shadow-artifact-fix`; **that side-worktree commit did not modify production
source.** Owner reconciliation set the production source INI back to `ExtraCamReg=-1`;
the installed INI independently reads `-1` (SHA-256 `ED85F34A…138A2D`).

Read this if you are about to run a headset session, or if you see `ExtraCamReg`
appear/disappear in `SingularityVR.ini`.

---

## 1. Current disposition — keep `ExtraCamReg=-1`

Do **not** arm `ExtraCamReg=13`. Run 43 already tested it and recorded a visible
screen-edge stretch with unchanged shadows. The production source and installed INI
are both now `-1`; the source comment records why. INI values are read at DLL load,
so changes made while the game is running do not affect that session.

The earlier 18:58:55 deploy report described a different transient configuration.
Current state was rechecked after the latest DLL-only runtime test: installed INI
SHA-256 `ED85F34A728F9C986B5D0AD07BB6E1C130CCAD2D1CD26836A8F15873DA138A2D`, value
`ExtraCamReg=-1`. Keep checking the effective value before tests that depend on an
INI change. The deployment script's `-DllOnly` path now leaves INI and textures alone.

---

## 2. Historical hypothesis (refuted by run 43)

Source: the *run 43* block in `d3d9.cpp` (~line 11048), which describes this artifact
nearly verbatim:

> *"The run-42 dump found a **second view-projection at vs c13** … Geometry riding it
> is **never remapped, keeps full-frame coordinates, and its shadow-pass output lands
> split across the seam.**" … "one cached slot is the whole fix."*

The following observations motivated the now-rejected hypothesis; they do not
override the run-43 A/B result:

| # | Finding | Source |
|---|---|---|
| 1 | The earlier handoff missed the tracked `SingularityVR.ini.example` entry for run 43; run 43 already tried c13 and shadows were unchanged. Source and installed INIs are now `-1`. | deployment log, `spikes\view_matrix\SingularityVR.ini.example`, current working INI |
| 2 | Game's own scan selected **c13**: `** c13 ROW w=+0.0000 fwdLen=1.0000 dotFwd=+1.0000 shape=ok hits=2 zero at camActor` (COL reads `shape=BAD`, so ROW is the right convention) | `view_matrix.log` scan block |
| 3 | Only the scan-locked **c0** is ever remapped — *"1 by construction, not by measurement"* | `d3d9.cpp` ~11050 |
| 4 | `mono-fallback = 0` in **129 of 134** gameplay windows → remap *is* active, so the cheap "nothing is being remapped" hypothesis the code warns about (~12444) is **not** the steady state | perf lines in log |

### Why it initially looked plausible (but was falsified)

- *…lands split across the seam* = full-frame coordinates inside a half-frame eye
  buffer → **visible in one eye only**, matching both captured frames.
- The artifact's eye **alternates between frames** (`14-42-37` left, `14-12-54`
  right). A single fixed cause would not alternate; which transform register a pass
  happened to ride that frame would.
- *"its **shadow-pass** output"* matches the shadow/decal-shaped floor silhouette, and
  `SHADOW TEST = DROP MAP PASSES` not removing it — dropping *generation* does not
  touch the receiver draw that rides c13.

### Limits and contradictory prior result

Run 43 deliberately does **not** run c13 through `ApplyProjection`, but still applies
eye offset and half-frame remapping using the locked register's storage convention.
That is a limited experiment, not proof of safety: the adjacent
`SingularityVR.ini.example` describes a prior c13 run that stretched geometry to the
screen edge while leaving shadows unchanged. Resolve the exact DLL, INI, and capture
for that run before calling the proposed change untried or safe.

Caveat carried from the original author: c13 is *inferred* to be a camera matrix from
its shape; `zero at camActor` (rather than a light position) is what supports that.

---

## 3. Run-43 outcome — do not repeat the c13 test

Run 43 already varied only `ExtraCamReg` and found stretched geometry with no shadow
change. Treat c13 as rejected; do not repeat or stack another variable onto it. Any
future shadow test needs a different, independently justified variable and a fresh
capture compared in both eyes.

---

## 4. Two provable blockers found on the way (do not rediscover)

1. **`ShadowMode=3` is a no-op.** Its census marks a slot only if it is a tracked
   camera slot (`s < g_camMatUsed`, which is 1 = c0) *and* `reg != g_lockedReg`
   (also c0) → mask is always `0x0`. Removing the exemption marks the *only* slot →
   `nSlots == 0` → mono-fallback → **stereo dies**. It cannot be fixed
   incrementally; it needs multi-register camera tracking.
2. **`SplitColourTargetsOnly` excludes nothing.** Its format whitelist
   (`A8R8G8B8 | X8R8G8B8 | A16B16G16R16F`) admits *both* scene-sized float surfaces
   (live census showed two `A16B16G16R16F` at scene size), so nothing is excluded —
   which in turn makes `g_dumpShadowConsts` (gated on `g_rtIsScene == 0`) structurally
   **unreachable**. `D3DSURFACE_DESC` has no field that separates those two surfaces.

Also: excluding the third target alone is **not** a fix — run 41 recorded it only
*moves* the artifact from one eye to both.

---

## 5. Where the full record lives

- `SingularityVR-shadow-fix\docs\SHADOW_ARTIFACT_CAPTURE_20261009.md` — full analysis,
  evidence table, test plan, rejected candidates.
- Commit **`bbcc83c`** on `feature/shadow-artifact-fix` (worktree
  `C:\Users\Trilex\vr-mods\SingularityVR-shadow-fix`) — capture doc, `fp_shadow.h`
  CAPTURED evidence block, source INI.
- `view_matrix.log` was **rotated at 18:55 today**; the scan block quoted above was
  captured before rotation and is quoted in the capture doc.
