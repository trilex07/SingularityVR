# HANDOFF — gloves-only model never appeared: `[Render] GloveIsolate=2` (2026-10-09)

**Scope:** original diagnosis of why the hands-only (glove) model did not render,
plus independent-hands and holster findings. The current state is in the addendum.

## Owner update — 2026-10-09 23:04

A new visible Steam+xrsim capture at 4224×2304 shows the actual title menu in
both eyes. The report passed all eight preflight gates; state PID 32608 matched
the game, state freshness was true, two views submitted with zero mono/stale
submits or errors, and the runner restored the Steam baseline. Screenshot:
`deployment/agent-test-bridge/gloves-only-holster-47265-20261009/captures/p2-stereo-4224x2304_sbs.png`.
This proves a current menu/compositor image, not gloves or holster in gameplay.

The next control boundary is clearer: this native menu's VR pointer ray comes
from the right-hand aim orientation, and selection is driven by the trigger
path. One acknowledged `aimtrim` change did not visibly move the ray. The later
explicit grip-rotation command reached the end of the bounded run without an
acknowledgement, so it has no result. Test the grip rotation with a fresh capture
in a new run; click only after the ray is visibly on a known item. No save was
loaded or changed in these runs.

## Owner update — 2026-10-09 22:53

The 22:51 Steam+xrsim visible-window hold reached a live focused session (two
views, 8,292 submits, zero errors) but the 1280×720 window disabled stereo
because the scene target is 4224×2304. It stayed menu-focused and provided no
fresh hand or holster pixels/events. Do not count this as visual acceptance.
The 22:48 direct windowed attempt exited before XR readiness; its earlier
readiness indication came from stale state written by another PID. The runner
now requires matching process PID plus current-run state-file mtime. See
`docs/HANDOFF_STATE.md` for the consolidated route and remaining gate.

The earlier recommendation to set `ExtraCamReg=13` is superseded. Run 43
already rejected it; correct value is `ExtraCamReg=-1`. Keep this hand test
focused on gloves/holster only.

## Current owner update — 2026-10-09 22:28

The historical source/deployment mismatch below has been corrected in the production
source INI: `GloveIsolate=2`. The source now also sets `ShowControllerArms=0` (the
previous value `1` explicitly restored native animated arm meshes), with
`IndependentHandsPrototype=1` and `ArmIKMode=0`. A Steam+xrsim candidate loaded all
four values, and fresh L1 glove/finger, reference-integration, and holster tests
passed. The source recording labels 5,570/5,573 samples as gameplay and reports
weapon field 5799 throughout, but its 112 left-grip samples and zero right-grip
samples mean it could not issue the right-grip holster toggle. The live replay
produced no `GloveCensus`, independent palette-write window, holster event, or fresh
visual capture, so rendered gloves and weapon-only hip storage remain unverified.
A sanitized 2,800-sample derivative now contains separate left-only/right-only pose
phases and two right-grip rising edges; offline replay passes with triggers, sticks,
UI clicks, and recenter cleared. This route has now run through Steam+xrsim: all 702
sample batches applied, two projection views remained focused, and both controller
poses were independent of the head. The game stayed menu-focused; the log has no
`GloveCensus`, successful independent palette-write window, holster store/draw event,
or fresh screenshot. Thus L1 math and xrsim integration pass, while visible gloves,
hand independence in the mesh, and weapon-only holster remain unverified. The user
authorized a brief current-save test, but no menu selection was guessed because the
active CUA session exposed no app windows. A follow-on launch was stopped before UI
input; Ctrl+C interrupted the runner before its restore, so the exact backup
`runner-backup-20261009-223023` was restored manually. Post-restore DLL/INI/VFX hashes
match the earlier baseline. No menu selection or save interaction was made.

Current evidence and next route: `docs/VR_STATUS.md` and the independent glove/holster
workstream in `docs/HANDOFF_STATE.md`. The stale `ExtraCamReg=13` table below is
historical; current source and installed baseline are `ExtraCamReg=-1`.

Latest completed route artifact: `deployment/agent-test-bridge/gloves-only-holster-47265-20261009/reports/steam-xrsim-run-20261009-223845.json`,
`hands-holster-gameplay-replay-v2.json`, and the collected log/state under
`../../deployment/agent-test-bridge/20261009-224031/`. It delivered the menu-select
probe and holster/pose samples but remained menu-focused. The attempted xrsim
`recenter` is not the game L3+R3 capture chord and produced no screenshot. CUA still
exposes no app windows, so a future P2 replay needs the game already in visible
equipped-weapon gameplay.

Latest startup-only verification: report
`deployment/agent-test-bridge/gloves-only-holster-47265-20261009/reports/steam-xrsim-run-20261009-224451.json`,
collection `../../deployment/agent-test-bridge/20261009-224712/`. The same x86 `/O2`
candidate passed all post-deploy preflight checks and ran for 116 s with xrsim
FOCUSED, two projection views, 8,306 submits, and zero runtime errors. No replay
commands were sent; controls remained neutral and `captureSeq=0`. The log remained
menu-focused and had no glove census, palette write or holster event. CUA exposed
no app windows while the process was running. Only stale captures from 17:50 and
18:23 were collected. The runner restored the exact DLL/INI/VFX baseline and
stopped the game; no save was loaded or changed.

---

## 1. The answer, log-proven

The last session's log (`view_matrix.log`, 19:09) contains:

```
glove isolation=0 (0=off, 1=census would-collapse slots, 2=collapse arm flesh onto live wrist; finger pose enabled on verified glove sections)
independent hands prototype=1; arm IK mode=0 (...)
```

`GloveIsolate=0` = **off**, and the very first thing `ApplyGloveIsolation` does is:

```cpp
static void ApplyGloveIsolation(..., int mode) {
    if (mode <= 0 || !map || slots <= 0) return;      // d3d9.cpp ~14470
```

So no arm-flesh slot is ever collapsed onto the wrist → the **native arms render**
→ "hands-only model" is not visible. `IndependentHandsPrototype=1` is ON, so the
independent-hand path itself is armed; it is the isolation step that is disabled.

### Two symptoms explained by the same line

- The `GloveCensus ... left=X/Y right=X/Y` line **never prints** — it lives *inside*
  `ApplyGloveIsolation` *after* the `mode <= 0` gate (~14498). This is why
  `GLOVES_FINGERS_SELFTEST_20261009.md` records "no `GloveCensus` line was emitted".
  **Absence of that line at mode 0 is expected, not a sign the code is dead.**

## 2. Root cause: the source INI was never updated, so every deploy reverts it

`git diff -- spikes/view_matrix/SingularityVR.ini` shows the glove agent added this
block (value **0**) to source:

```ini
ArmIKMode=0
; 1 = census-only; 2 = collapse classified arm-flesh slots onto the live wrist.
; The verified SP section maps and static census are complete; use the live wrist path.
GloveIsolate=0            <-- comment says use mode 2; value says 0
```

while `GLOVES_FINGERS_INTEGRATION_20261009.md` states *"Installed INI contains
`GloveIsolate=2`"* (14:38 install).

⇒ `2` existed only in the deployed copy; a later deployment copied source (`0`) over
it. **Same failure mode as `ExtraCamReg`**: config that lives only in the deployed
file does not survive the next deploy.

## 3. What was changed (deployed INI only)

`GloveIsolate=0` → **`GloveIsolate=2`** in
`C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\SingularityVR.ini`,
with an inline note recording the above. Read once at DLL load, so it applies on the
**next** game launch.

**Recommended (owner's call — not done here):** mirror `GloveIsolate=2` into
`spikes\view_matrix\SingularityVR.ini`, otherwise the next deployment reverts it.
That is exactly what was done for `ExtraCamReg=13` after the previous handoff.

**REVERT:** `GloveIsolate=0`.

### ⚠️ Two variables are currently armed

| Key | Value | Subsystem |
|---|---|---|
| `ExtraCamReg` | `13` | shadow/decal seam (see `HANDOFF_EXTRACAMREG_SHADOW_20261009.md`) |
| `GloveIsolate` | `2` | gloves-only model |

They are independent code paths, but state which one you are judging in any given
session — do not draw conclusions about one from a run where both are changed.

## 4. Independent hands — findings

- `IndependentHandsPrototype=1` is confirmed ON by the log.
- `PrepareIndependentLeft` (`d3d9.cpp` ~14543) gates hard. It returns 0 unless
  **all** of: `g_independentHands`, `motionMode ∈ {0,1,3}`, `g_fgRideAll == 1`,
  `g_gunVerts != 0`, `g_camMatrixBound`, `g_offHandValid`,
  `g_handPositionValid[OffHand()]`, `FgMatchWindow()`, `!ArmsAnimActive()`,
  **`WeaponInHands()`**, and then an exact shader layout
  (`bones==5, boneRegs==225, local==230`) plus exact mesh identity
  (verts/first/triangles) for one of five known sections.
- **Holstering does NOT disable it.** `WeaponInHands()` reads `g_handsStatus`, which
  is the game's own `mPlayerHandsStatus` (~2185). The holster is mod-local
  (`g_weaponHolstered`) and does not touch it — so glove posing survives storing a
  weapon. Putting the weapon away *in the game* (status ≠ `HANDS_WEAPON`) or an arms
  animation (`ArmsAnimActive()`) will stop it.
- **Diagnostic gap:** the counter that would prove the path ran is deliberately
  discarded. `PrepareIndependentLeft` increments `g_independentLeftDraws` (~14630),
  but `ReportFgRide` does `(void)independent;` (~15265) to avoid per-frame log
  records. So the log can neither confirm nor deny that independent hands executed.
  If a headset run needs that answered, re-enabling that one counter print (slow tick)
  is the cheapest evidence.

## 5. Holster — findings

- `GunPivotWorld` (~14212): when `g_weaponHolstered`, returns `HolsterHipWorld(H)`
  immediately — no controller follow, no ADS pullback, no hand translation. Position
  is body-locked at the hip (`fwd/right/up = 8/12/-30` UU, radius 12 UU).
- `BuildGunC` (~14365): rotation is **latched at STORE time**
  (`g_holsterYaw/Pitch/RollUU`), so the stored weapon keeps its stored orientation.
- Caveat: `BuildGunC` still uses `G = GunAnchorWorld(G)` as the transform's frame,
  and that derives from `g_wantYaw`/`g_wantPitch` (**head**), not the hand. So a
  stored weapon is head-coupled, not purely body-fixed. If the reported symptom is
  "follows my head", this is the term; if it is "follows my hand", it is not.
- `FgRidesGun` (~15165) exempts, while holstered, only meshes matching
  `g_armsVerts` **or** `FgIsExtraArm(verts)`. `FgIsExtraArm` (~13624) additionally
  requires `g_hideExtraArms` latched and the count to be in the learned baseline
  `g_fgBaseVerts[]`. **Whether every glove section (3314/2924/390/7444/2626/4818/4781/37)
  is covered depends on which role got latched** — an unexempt glove mesh would ride
  the gun pivot while stored. This is the most likely mechanism for a hand/weapon that
  stays attached while holstered, and it is **unproven without a run**.
- **No holster event in the log at all** (no `weapon holster: STORED/DRAWN/RESET`),
  only the startup line. So in the 18:58 session the toggle never fired — the earlier
  report cannot be re-confirmed from this log.

## 6. Test

1. Confirm both keys are present in the deployed INI (a deployment wipes them).
2. Launch, same room. **Judge gloves only** — do not also judge the shadow artifact
   in the same observation.
3. Expected with `GloveIsolate=2`: arm flesh collapses onto the live wrist, leaving
   the glove/hand; `GloveCensus ... mode=2 left=A/B right=C/D` should now appear
   (it was unreachable at mode 0).
4. Then holster (right grip rising edge) and watch whether the stored weapon stays at
   the hip when the hand moves away — that is the unconfirmed item in §5.
