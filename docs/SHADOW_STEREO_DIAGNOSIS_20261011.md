# Stereo shadow artifact — root-cause analysis (research, no Steam)

Owner: side agent (`research/shadow-stereo-artifact`). No Steam, deployment,
or headset access. Production `spikes/view_matrix/d3d9.cpp` untouched.

Source evidence: `docs/SHADOW_SFX_HANDOFF_20261008.md`,
`docs/HANDOFF_STATE.md`, `spikes/view_matrix/d3d9.cpp`,
`spikes/view_matrix/shadow_mode3.h`, `shadow_pass_policy.h`,
`shadow_policy.h`, `fp_shadow.h`.

## 1. ShadowMode implementations vs measured outcomes

| Mode | Sampling-pass treatment (`d3d9.cpp:11944`) | Measured outcome (repo docs) |
|---|---|---|
| 0 | Full split per eye, every valid camera slot remapped | Measure-only arm; artifact present |
| 1 (default) | Drawn ONCE, unremapped (`draw()`) | Squares PERSIST under 1 |
| 2 | Draw dropped (`S_OK`) | 21–221 sampling draws/window dropped; user verdict: modes change nothing; shadows off |
| 3 | Falls through to split path; census slots excluded from remap (`d3d9.cpp:11964`) | Census log-proven `mask=0x0` live → protection never engages |

Two exclusion results bound the search space:

- `SHADOW TEST = DROP MAP PASSES` removed ~800–840 classified
  shadow-layout draws/window while the ceiling/wall pattern REMAINED
  (`SHADOW_SFX_HANDOFF_20261008.md:7`). The visible pattern is therefore
  NOT proven to come from the ordinary shadow-map generation pass.
- Run-44/NUMPAD0 note (`d3d9.cpp:11912`): the artifact is suspected to be
  the decal pass on the G16R16 scene-sized target — it vanishes with
  "High Quality Decals" OFF. Unproven which surface; see validation plan.

## 2. Suspected render path (exact seam)

1. Generation draws return before the seam via hook early-outs; the mode-3
   census refresh lives in `ShadowMode3NoteGeneration()` (`d3d9.cpp:18782`,
   called at `:11935`, `:13114`, `:16477`, `:17922`, `:18062`).
2. At the seam (`d3d9.cpp:11944`), draws issued while the projected
   512x512 R32F map is bound (`g_shadowStageMask`) are: split (0), drawn
   once unremapped (1), dropped (2), or split with census exclusion (3).
3. The split path collects every valid camera slot (`d3d9.cpp:11967`)
   and re-uploads a PER-EYE version via `ApplyEyeRemap` (`d3d9.cpp:3818`).
4. Camera-slot tracking is deliberately invalidated on shadow layouts
   (`d3d9.cpp:4339`, `:11132` — `InvalidateCamMats()` when the bound
   shader declares `ProjectionMatrix` + depth bias). Receiver draws that
   reuse the same registers afterwards can therefore carry stale/invalid
   slots (→ mono fallback flicker) or a light matrix mistaken for a camera
   matrix (→ per-eye shadow-UV shift).

## 3. Light-space treated as eye-space — three concrete cases

- (a) Modes 0/2 remap EVERY valid slot with no light exclusion (exclusion
  exists only in mode 3). Any light-space matrix tracked as a camera
  matrix — the register reuse is named in `shadow_mode3.h:14` — is
  eye-shifted, so both eyes sample different shadow coordinates for the
  same world point: blocky, flickering, one eye darker, differing per eye.
- (b) Mode-3 census is live-empty (`mask=0x0`; only `c0` tracked and
  explicitly exempt at `d3d9.cpp:18816`). `SlotMayRemap` then returns true
  for every slot, i.e. mode 3 degrades exactly to mode 0 for light slots.
  Pinned by `shadow_stereo_audit_test.cpp` ("empty census remaps").
- (c) Mode 1 (mono sampling) persists squares, which falsifies "sampling
  remap" as the SOLE cause and keeps the decal/G16R16 pass and the
  FP-caster depth draw (`fp_shadow.h` — needs headset-verified capture,
  explicitly speculative there) on the table.

No shader registers were guessed and no shadow path was removed: modes 1
(mono) and 3 (protect) are classification policies, and mode 2 (drop)
stays a diagnostic arm, never a fix.

## 4. Falsifiable diagnosis

PRIMARY: the eye-to-eye mismatch comes from a light-space matrix sharing
a remapped camera slot (case a), with mode 3 failing to exclude it
because its census is empty in live runs (case b). Falsified if a MAIN
run shows a NONZERO census mask (`ShadowMode3 census: light-space slots
mask=0x...`) together with unchanged per-eye mismatch — that would prove
exclusion engaged and the mismatch comes from elsewhere.

ALTERNATIVE: the squares are the decal pass on the scene-sized non-colour
target, not shadows at all (run-44 note + High Quality Decals OFF
observation). Falsified if `DropExtraTarget=1` / decals-OFF A/B leaves
the pattern unchanged while shadow A/B moves it.

## 5. Offline tests (this branch, no game)

`tools/reference-tests/shadow/shadow_stereo_audit_test.cpp` — pure-logic
tests over the checked-in headers only (`shadow_mode3.h`,
`shadow_pass_policy.h`, `shadow_policy.h`, `fp_shadow.h`):

- `ShouldRemapRegister` / `SlotMayRemap` exclusion truth table, including
  carry-mask and the empty-census-degrades-to-split invariant.
- `DecideDraw` seam routing (shadow/offscreen/non-scene → native-once;
  camera → per-eye; unknown/non-finite → mono fallback).
- `Classify` / `Decide` policy gates (depth-only and shadow-shader →
  preserve native; gameplay + sampled shadow → mono receiver; non-finite →
  conservative mono; drop is explicit-only).
- `IsProjectedShadowMap` size/format filter and the FP-caster conjunction
  (`IsFpShadowCasterDraw`: FP verts AND shadow layout AND offscreen AND
  same-geometry sibling of a this-frame FP colour draw), with negative
  cases.

Result: PASS (x86 `cl`, see §6). No production code changed; any future
candidate stays opt-in behind a new INI switch defaulting to log-only,
isolated from `d3d9.cpp`.

## 6. Minimal validation plan (MAIN-owned, one variable per run)

1. Baseline: `ShadowMode=1`, `VrShadows=0`, `DropExtraTarget=0`. Record
   `shadow-sample`, `ShadowMode3`, `camera matrix registers tracked` lines.
2. Mode 3 alone (`ShadowMode=3`, nothing else): verdict is the census
   line — `mask=0x0` reproduces the empty-census finding; nonzero mask +
   same mismatch falsifies PRIMARY per §4.
3. Restore baseline, then `DropExtraTarget=1` alone, then game
   High Quality Decals OFF alone: moves-or-not decides ALTERNATIVE.
4. Never combine with `VrShadows`/`ShadowMode` changes in one run
   (`SHADOW_SFX_HANDOFF_20261008.md:9`); never mark headset-verified
   without the headset run.
