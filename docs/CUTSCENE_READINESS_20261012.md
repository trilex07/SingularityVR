# Cutscene detection — integration readiness (research, unmerged)

Branch `research/cutscene-detection`, base `origin/main` @ `e07367a`.
Side agent: no renderer, Steam, weapon, or pause-menu changes. Production
`d3d9.cpp`, `vr_scene_mode.h`, and all pause work untouched. MAIN's
integration target is the CURRENT VR line **`fix/pause-gfx-head-overlay`**
(@ `58e9c65`); all line numbers in §7 are from that branch.

Companion docs: `docs/CUTSCENE_DETECTION_20261012.md` (investigation),
`docs/CUTSCENE_FLAT_MODE.md` (presentation, reference fixed — see §3).

## 1. 92 ON / 88 OFF imbalance — unmatched states cannot persist

STATUS C records 92 × `0x1 -> CINE ON` against 88 × `0x0 -> CINE OFF`
in one session (delta = 4 trailing ONs). Consistent, evidence-backed
sources for the delta (no new guessing):

- Menus report `CINE YES` for whole menu sessions (HISTORY run 176)
  without a matching OFF; the OFF arrives implicitly via the
  camera-change clear on the next level load, which logs no OFF event.
- A session that ends mid-cinematic leaves one trailing ON.

Neither source can leave a PERSISTED cinematic state through the voter:

- Menu-context hook assertions (no video) are demoted by
  `cutscene::Qualify` and can never engage the latch (replay: 600
  menu-CINE frames, still disengaged — test `run-176 replay`).
- Trailing ONs clear on the next camera transition with a dead old
  instance, or after 4 clear frames, or on the first exit edge
  (replay: `imbalance replay`, `run-138 replay`).
- State is per-launch (volatile globals); nothing survives a restart
  except through a fresh hook assertion, which is a live signal, not
  residue.

Residual risk: a cinematic that asserts the hook forever, sends no exit,
and never changes camera would hold flat indefinitely — but that is the
engine truthfully reporting "still cinematic", and `CutsceneAuto=0`
remains the kill-switch.

## 2. Matinee camera cuts vs level transitions — distinguished

The run-211 camera-clear assumes instance change = level transition.
UE3 Matinee director tracks CAN cut between camera actors mid-cinematic,
which would change the tracked instance and falsely terminate cinema
mode under the naive rule. The distinguisher uses the production
liveness machinery (STATUS D: `IsLive` exists precisely because stale
level cameras stay mapped and readable):

- `RawSignals.cameraChanged` + `oldCameraLive=false` → level
  transition → immediate clear (`kCameraClear`).
- `cameraChanged` + `oldCameraLive=true` → director cut → latch HOLDS,
  clear counters restart so a cut cannot shorten hysteresis.

MAIN feeds `oldCameraLive` from the existing `IsLive(oldCam)` check at
the `s_lastCam` transition site. Covered by `director-cut hold` and
`matinee replay` (two live-old cuts + exit edge) tests.

## 3. Doc reference + dead `IsVideoQuad` dimension test

- Fixed: `docs/CUTSCENE_FLAT_MODE.md` validation now points at the real
  paths — `spikes/view_matrix/tests/test-vr-scene-mode.cpp` (exists on
  MAIN's line, absent from bare `origin/main`) plus the new
  `tools/reference-tests/cutscene/cutscene_detect_test.cpp`.
- Dead code (`d3d9.cpp:17726-17727` on MAIN's line: after the early
  `texFmt != kFmtL8` return, `(texFmt == kFmtL8 || ...)` is constant
  true): NOT edited here (renderer freeze). Specified removal for MAIN
  (one line, behind the existing classifier A/B discipline):
  `return texW && texH && rtW && rtH;` with the L8 + sole-draw comment
  kept as the documented signal. Do NOT "activate" the size arm without
  A/B evidence — run-259 regressed from a broadened gate.

## 4. Recorded-sequence validation — PASS

`tools/reference-tests/cutscene/cutscene_detect_test.cpp` replays,
frame by frame: run-138 intro (engage → 4-clear restore), run-176 menu
session (600 frames, never engages; load clears), 92/88 session-end
trailing ONs (relaunch clears), Bink single-quad immediate entry +
4-clear exit, Matinee with two director cuts + exit edge, pause-during-
cinematic (pause is diagnostic; hook drives). The Bink replay caught one
real spec deviation during development (video took 2-frame entry);
fixed so video engages on frame 1, matching `VideoFlat` semantics.
Result: `PASS: cutscene detect (qualify + latch + recorded replays)`
(x86 `cl /std:c++17`, executed; artifacts removed).

## 5. Flat presentation — BLOCKING finding for MAIN

Verified against MAIN's branch code (not assumed):

- `StereoPairBody` flat gate (`d3d9.cpp:11854`) draws each scene draw
  ONCE across the full SBS target with no per-eye split.
- The submit bridge hands each eye its HALF by rect in SBS-dup mode
  (`d3d9.cpp:11030-11040`).
- The mono path that submits one crop to both eyes (`cropEyes`,
  `d3d9.cpp:10882`) requires `frame->menuFlat`, which is set ONLY from
  the GFx menu class (`d3d9.cpp:5516`); nothing in the cutscene path
  sets it.

Consequence: during `CutsceneFlat` each eye currently receives HALF the
cinematic image (left eye = left half, right eye = right half) — not
one correct mono image per eye, and not a duplicated SBS image either.
`CUTSCENE_FLAT_MODE.md`'s "full source image in both halves" claim is
wrong for scene draws. (Bink quads are unaffected: `DrawVideoQuadPerEye`
rewrites per eye.) MAIN must extend the menuFlat-style mono crop to
scene-flat frames (or duplicate the full frame per eye) BEFORE
`CutsceneFlat` is user-visible; detection work is unaffected and stays
valid regardless.

## 6. Integration plan (target: `fix/pause-gfx-head-overlay`)

1. After pause-menu work is stable, rebase this research branch onto
   MAIN's tip; the only expected overlap is `docs/CUTSCENE_FLAT_MODE.md`
   (validation section) — no code overlap (new files only).
2. Wire `RawSignals` from existing globals at `UpdateSceneModeFrame`
   (`d3d9.cpp:20018`): `g_inCinematic` (`:20016`), exit-edge observation
   at the script-hook site (`:19940` table entry path), `g_sceneVideoSeen`,
   `g_vrUiMenu`, `g_gamePaused`, and the `s_lastCam` transition site
   (`:19406` comment block) extended with `IsLive(oldCam)` for
   `oldCameraLive`. Feed the latch output into the existing controller
   path behind `CutsceneAuto` (default preserves current behaviour);
   extend `SceneModeTrace` logging with the reason bits.
3. Implement the §5 mono-submit fix FIRST (scene-flat frames must reach
   the one-crop-to-both-eyes path); then validate on hardware one
   variable per run: intro Bink → `VideoFlat`; Matinee → `CutsceneFlat`
   with head tracking; menu → never scene-flat; director-cut scene →
   holds flat; exit/level load → `GameplayStereo`.
4. Apply the §3 one-line classifier cleanup only with A/B evidence.
5. Do NOT broaden `IsVideoQuad`, touch weapon/glove/input/pause-menu
   systems, or add per-level timings.

## 7. Integration points + overhead (all on `fix/pause-gfx-head-overlay`)

| # | Point | Line | Change |
|---|---|---|---|
| 1 | Script-hook table (`SetCinematicMode`) | `d3d9.cpp:19940` | Observe/record the `0x0` exit edge (read-only) |
| 2 | `g_inCinematic` state | `d3d9.cpp:20016` | Read into `RawSignals` (no semantics change) |
| 3 | `UpdateSceneModeFrame` sampler | `d3d9.cpp:20018` | Call `Qualify`+`Latch`, feed controller behind `CutsceneAuto` |
| 4 | Camera-transition clear | `d3d9.cpp:19406` block | Add `IsLive(oldCam)` → `oldCameraLive` |
| 5 | Flat gate | `d3d9.cpp:11854` | Unchanged by detection; §5 fix lands here/bridge |
| 6 | Mono-submit bridge | `d3d9.cpp:10882`, `:5516` | §5 fix: scene-flat frames into one-crop path |
| 7 | Cinema matte | `d3d9.cpp:23317` | Unchanged (already gated on `Flat()`) |
| 8 | `IsVideoQuad` dead arm | `d3d9.cpp:17726` | §3 one-line removal, MAIN with A/B |

Overhead: per-Present O(1) — one `Qualify` (≤6 boolean ops) + one
`Latch::Advance` (≤8 integer compares/increments), zero allocations,
zero per-draw cost (seam gate at `:11854` already exists; no new hook).
Submit path unchanged except §5 fix, which reuses the existing
menu-crop copy (one extra GPU-side `CopySubresourceRegion`, same as the
proven menu path). No measurable frame-time impact expected; confirm via
the existing hotpath sampler after wiring.
