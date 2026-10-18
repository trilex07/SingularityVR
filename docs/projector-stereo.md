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
running. Hold Pause open for 10 seconds, resume and hold the view for 10 seconds.
Move a little closer or to the side where the misplaced image is visible,
hold Pause open for 10 seconds again, resume,
hold 10 seconds, then exit normally. No chat switching; no need to fire.
One brief readback hitch per capture is possible.

Afterward: require complete trace/shader budgets, identify light-function
shader constants using CTAB and disassembly, inspect sampler/resource chains
and both-eye constants, and match candidates to the SBS image. Only then use
a narrowly scoped discriminator/correction. Do not reuse muzzle shader hashes
or apply the cinema/Bink presentation fix to this light function.

## Session 37168: capture failure, not a renderer result

User identifies the interval between the penultimate and final pauses as the
canvas view. In the archived log these are class3 at line 83884, gameplay
class0 at 85795, and class3 at 88301. This gameplay interval reports active
OpenXR stereo and roughly 88.4–90 FPS; it does not identify projector pixels.
The scheduler had already failed on an unread burst above 1 MiB after its
earlier pause cue. No shadow-pass-37168 trace exists. Early SBS captures cannot
substitute for this late canvas view. No projector shader fix is justified yet.

Session evidence is local/ignored at build/projector/evidence-37168. Log SHA256:
803AB1A8DF1024993E33D2886DC05EC3FF968A812D170895B3E6C361BEDB4997.
Game exited; restore watcher completed and independent installed DLL/injector/
INI/UI hashes match the accepted profile. No candidate remains installed.

Recorder now drains backlog in bounded chunks, preserving partial UTF8/lines.
Projector cues require six focused heartbeat records during Pause before
return to gameplay, so quick travel pauses do not arm captures. Hold Pause
for 10 seconds at each intended view. Other scheduler modes retain their cues.
Offline regression PASS: >1 MiB burst, UTF8 boundaries, partial-line append,
truncation, short/long pause and Options rejection. Actual session replay PASS:
11,575,857 bytes / 91,527 lines / no leftover fragment. New recorder live
validation remains pending; no C++ or installed-file changes in this repair.

## Repeat lease and performance triage (2026-10-18)

User requested test preparation plus read-only frame-drop analysis. PID15548
launched through Steam with verified DLL 709F64A7...CE1FBA, injector
5B948DDB...729D59, INI 38642BD6...D73B3E. Backup 20261018-001229;
install report 20261018-001230.json. Restore watcher PID35172 armed; logs named
projector-001229-{capture,restore}.log/.err. No performance flag was changed.

Previous-session performance evidence (not a causal attribution):
- Existing analyze-vr-perf-log.py does NOT parse the current `perf:`/`frame
  budget:` format. Its missing budget section must not be interpreted as no
  stalls. Direct inspection of these lines is required until parser updated.
- Slow windows include boot/no-pawn and menu/cinematic transitions. Class0
  alone is NOT sufficient to classify normal gameplay (e.g. line51398 has
  CINE ON and no split draws). This prevents mislabeling loading as gameplay.
- At line51398: 68.5 FPS, 14.61 ms total, wait5.46, copy0.01, walk0.11,
  submit0.29, gameplay hooks0.01, residual8.72 ms. Input means in the same
  report: sync0.154 + states0.042 + poses0.045 ms. No evidence of a sustained
  input polling collapse in that window. Residual includes unmeasured engine,
  driver and mod work; it is not an identified function or GPU duration.
- End-of-canvas view is ~88–90 FPS; InputDeviceCache still shows two cumulative
  cold WMI calls, rather than a recurring full enumeration per button hold.
- Six automatic 5904x3012 BMPs were saved earlier. SaveBackbufferCapture does
  GPU readback, allocation and ~71 MB file write on its calling render path;
  menu-placement proof triggers this without a user screenshot request.
  MenuDrawTrace also samples eight frames per generation and flushes grouped
  records on transitions (28 flushes, 16054 groups, 6826 dropped in old log).
  These are concrete diagnostic overhead candidates, NOT measured root causes.
  Logger uses AsyncLogEnqueue first; do not assume every MDG line is disk I/O
  on the rendering thread. Need scoped timings or isolated A/B after projector
  capture before claiming either explains the user's intermittent drops.

### Repeat result: complete canvas frame, native light-function candidate found

User closed normally and reports drops mainly just after the crash/collapsing
building, not after entering the first room. Do not attribute these exclusively
to screenshot/menu overhead. That remains a separate performance hypothesis.
Restore completed; independent hashes of DLL/injector/INI/UI all match the
accepted baseline. No Singularity process remained.

One capture succeeded at 00:14:59: shadow-pass-15548-1.txt, 911 draws, 271 groups,
111 shaders; complete_budget=true, shader_budget_overflow=0,
shader_unavailable=0. The SBS image visibly faces the illuminated canvas with
film content toward its right edge. Second capture did not fire: next Pause
transition went through class4 before class0 (exit flow), which the deliberate
cue rejects. First complete frame is usable; no need to repeat baseline now.
Private log, parsed trace, bytecode and preview: build/projector/evidence-15548.

Native candidate PS FNV64 7F00BC7925591BE8, SHA256
993d7174be6c2f947a395ab9f9d1eaecbc28209af8428e1cf32060dc28178f64.
CTAB proves ScreenToLight at PS c3..c6, ScreenPositionScaleBias c1,
MinZ_MaxZRatio c2; s0 is scene/depth source, s1 the 2048x2048 DXT1 atlas.
Disassembly reads s0 alpha, reconstructs depth through c2, multiplies screen
coordinates/depth by c3..c6, divides by resulting z, and selects atlas tile
using c8/c9 (4x4 in this captured frame). Thus this is animated light-function
projection, not a fullscreen Bink or ordinary movie-screen mesh UV path.

Groups171/172, draws634/635: same full 5904x3012 viewport with left/right
scissors, same textures, PS c1..c6 identical across eyes. Their VS SHA256
fceb34c84385427e464bb2e59ca82737934116b0892c6ff989d63f793bf23023
is a three-instruction clip-position/texcoord pass-through with local def c0;
the uploaded camera VS constants are NOT consumed by this VS.
No existing ScreenToLight correction was found in source.

Next: inspect the exact quad stereo route and reconstruction convention; prove
the screen-coordinate mapping and derive per-eye transform before touching
constants. Exact shader/resource scoped experiment only. Identical constants
are concrete evidence, not by themselves proof of the final pixel cause.
Falsifier: correct per-eye reconstructed projection still leaves the same
misplaced movie, or candidate suppression leaves that content unchanged.

## Scoped reconstruction candidate

`Render/ProjectorWorldStereo` is enabled by default in this integration, with
explicit `0` available as an opt-out. Only captured VS AC4827843287AECB
(208 bytes) + PS7F00BC7925591BE8 (1336 bytes), indexed four-vertex/two-triangle
quad, exact scene/atlas formats and full viewport/eye scissors. Native camera
VP is captured before head/FOV/offset correction at the validated locked c0
upload and paired with the modified base; stale/non-ROW/nonmatching camera
slots reject. Never infer a camera from unrelated uploaded constants.

For row vectors form S(VP) with columns VP.x, VP.y, VP.w, affine1. The audited
PS consumes [ndcX*linearDepth, ndcY*linearDepth, linearDepth,1]. Therefore
correctedLight = inverse(S(eyeVP)) * S(nativeVP) * nativeLight.
ScreenPositionScaleBias remains untouched: the fullscreen pass-through quad
already samples the corresponding SBS half. Only PS c3..c6 change inside one
draw scope, restoring on success/failure and latching any restore fault OFF.
Resource COM refs are balanced; bounded classification cache released on Reset.

L1 math PASS: identity, both-eye half remap, FOV/eye translation, world-point
projection equivalence, singular/NaN rejection. L1 mocked scope PASS: default
OFF, draw/camera gates, exact restoration, resource refs, failed upload and
restore-fault latch. L2 x86 PE32 /O2 build PASS; existing compiler warnings
remain (not a warning-clean build). No Steam deployment or visual acceptance
of this candidate yet. Do not combine a performance change with its test.

Large automatic screenshots explained: 5904*3012*4 = 71,131,392 pixel bytes
(plus 54 BMP header bytes). Menu proof capture and requested trace capture
produce uncompressed stereo BMPs, distinct from Steam screenshots. They remain
unchanged for this controlled rendering candidate.

## PID30976 physical failure and classifier repair

User reports film partially on canvas in one eye; projection changes with head
motion. Runtime enabled=1 only, with no applied/rejected samples. Therefore
the matrix correction was not demonstrated to execute, and this test cannot
validate or falsify its reconstruction math. Safe install restored and DLL,
INI, injector and UI independently verified after exit.

Source defect: Classify permanently returned false for every new shader once
128 distinct shader objects had been cached. Menus and earlier world draws
can exhaust this before first encounter with the projector. Hypothesis:
classification prevented reaching Scope; alternatives include pair/route gates.
Repair replaces the oldest cache entry with balanced COM references; capacity
remains 128. New aggregate counters identify classification, pair, route, shape
and camera failures once per second, with no per-draw log output. No matrix
or resource gates changed. Falsifier: exact pair hits remain absent after
eviction, or route rejection prevents both-eye correction. L1 regression feeds
128 nonmatching objects before a matching 129th, verifies recognition/cache hit,
and checks all references are released. Scope restoration/math tests also pass.

## MAIN integration decision

The later PSVR2 run confirmed the projector image landed correctly on its
canvas; the user reported the projector route looked good. Integrate the exact
shader/resource-gated per-eye reconstruction and enable it by default. A user
INI can still explicitly set `Render/ProjectorWorldStereo=0` to disable it.
Only the captured projector light-function quad is eligible; other draws retain
their original constants. The change preserves guarded failure and state restore.

The 128-entry linear shader cache was directly observed to churn: a runtime
window recorded 17,584 classifications and evictions in one second. Replace it
with the fixed 1024-set, four-way cache (4096 bounded COM references, at most
four pointer comparisons per shader bind) and release references on Reset.
L1 tests cover collisions, eviction, exact shader recognition, and COM balance.
The user later reported only negligible residual micro-stutters outside the
building-collapse event. That subjective improvement is useful acceptance
evidence, but the collapse capture used a diagnostic profile without this cache
enabled and does not establish a frame-time gain from it. The isolated collapse
dip remains unresolved; this change does not claim to fix it.

Integration validation on `fix/main-projector-optimizations`, source
`c6aa6c6670e94777c64f3c1ff9ddc289af6cae32`: projector math and cache/scope L1
PASS; six related renderer regressions and six Python checks PASS. Production
x86 PE32 `/O2` build PASS, DLL SHA-256
`D2778CA8652F00BECD8C88D9660C11352A523B32DBE151E33AB7BCEF49AA04E4`.
The build reports existing repository warnings; this is not warning-clean.
Steam remains untouched by this integration branch.
