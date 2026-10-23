# Volk 8fbb9f4: physical capture and restored baseline (2026-10-23)

## Result: E for producer attribution; whole-palette collapse confirmed

No renderer change, new launch or headset request. Physical feedback: L2 hides
the right weapon in BOTH eyes; R2 restores it, including firing while L2 remains
held. Left Grip truncates the left glove/wrist: separate unresolved defect.

Full original log secured locally before any launch:
`deployment/crash-evidence/20261023-volk-skin-8fbb9f4/view_matrix.log`
in the `SingularityVR-volk-skin-evidence` worktree. 1,884,193 bytes.
SHA256 `5D3CA90DE512F8293276045E23EC5B6F757CB98564690DB16FF1706BC444CBC6`.
Full log and diagnostic DLL/INI stay private; line numbers below refer to it.
Initialization line 103 confirms `VolkImpulseSkinProbe=1`, max64 draw samples.
MAIN-built deployed DLL was `6B085CBAC21BDCAAD57A7B72FDC510C9403B166660DE4C366D7BC7ADA15F2CAB`,
not SOL's separately built `2DC5EF57...`. Source: 8fbb9f46b5e993fa4f3a360f723fa90fe9eceae1.

## Consumed geometry, not just c5

Original cooked/PSK index identity and influence audit is in
`sol-volk-impulse-skinning.md`: 8297 vertices, 10835 faces, palette
[55 clip, 56 Slide, 53 cog_shotgun, 54 release, 57 Trigger]. All vertices
have one influence and no triangle crosses rigid groups. Respectively:
c5-c7 (198 faces), c8-c10 (2718), c11-c13 (7859), c14-c16 (20), c17-c19 (40).
The current log does not hash runtime IB contents: original-asset identity is
offline evidence, not a runtime checksum claim.

All 64 selected draw headers retain weapon 68002400, IB 73171720, VB 36948B40,
VS 390EB240, stream 0/32, bone-register base 5, local base 230, first0/tris10835.
All 128 eye-segments return HRESULT 00000000 and the same bound IB.
All completions report eyes2/segments1, nextRegs0, cylinder-1.
Success HRESULT is submission evidence, not visible-pixel proof.

## Frame/tick comparison (GetTickCount64 milliseconds, not wall-clock time)

Determinants below always list clip / Slide / main body / release / Trigger.
The two submitted eyes agree in all 64 samples.

- f4375, tick890750, line6839: hands2, support0, incoming and submitted [1,1,1,1,1].
- f4905-4929, ticks896812-897078: support1, all five remain nondegenerate.
  This does NOT validate the separate left-glove geometry.
- Fire receipts at ticks900656, 901187, 901984 (30 nested entry/exit records,
  not 30 shots); f5202-5336 stays nondegenerate. These precede the first L2.
- L2-down tick902921, line9138. f5397 at same tick, line9139: all five still1.
- f5398, tick902937, line9198: hands1, incoming all0; both submissions all0.
  All five matrices have exactly zero 3x3 and the SAME translation:
  [0,0,0,-3.25532; 0,0,0,-110.263; 0,0,0,4.50837].
  Therefore every vertex influenced by these slots collapses to one point;
  it is not an inference from the clip alone. A valid subsequent rigid c230
  cannot restore the lost spatial extent.
- f5399/5402/5422 through tick903203: same collapse, both eyes.
- L2-up tick904046, line9578; f5498-5522 through904312 remains collapsed.
- L2-down tick904859, line9877; f5572-5596 remains collapsed.
- L2-up tick905390, line10122; f5619 still collapsed.
- f5620 tick905406, line10180: all five1, even though header still says hands1.
  f5621 tick905421 changes to hands2 and remains1 through f5645.
  No matching fire receipt proves why this restoration happened. Do NOT label
  it the user's held-L2/R2 test. `hands==1` alone is not a valid collapse gate.
- f5655 tick905796 through f5679 tick906062: all five0 again.
- f6051 tick910437: all five1 again; no attributable fire event.
- f6055/6057: incoming and submitted are both nondegenerate but differ.
- L2-down tick910500; f6058 tick910515 through f6086 tick910812:
  incoming all0, but BOTH submitted palettes all1 and repeat exactly
  [0.99877,-0.04149,0.0271456,-13.8426;
   0.0274239,0.9184,0.394702,-46.6749;
   -0.0413069,-0.393472,0.918408,-65.7942].
  Do NOT omit this exception or claim the incoming palette is always unchanged.
- L2-down tick910968, f6100; L2-up tick911140, f6115: all five0 through submission.
- Final L2-down tick911234, line14338; f6123 line14339 is sample64, all five0.

Totals: 30 samples have all-five-zero incoming palettes. 24 also submit all-zero
palettes; six instead submit a valid repeating palette. 34 samples have valid
incoming palettes. Eight samples differ incoming versus submission (two valid
to valid, six zero to valid). No captured valid-to-zero submission.

## First observed violation, and why A/B cannot yet be separated

The first invalid stage captured for the first L2 is the actual D3D constant
read at DIP incoming (before the draw-time controller/stereo work), f5398.
The whole five-bone palette is degenerate there and remains so in both eyes.
This confirms geometric collapse before stereo, not its producer or intent.

Every one of the 64 `volk-skin upload` records says `matches=0`; there are no
`stage=hook-input` matrices. The recorded last setter caller is 00518F85 with
EXE base00400000 (RVA00118F85), but its tracked VS is zero or another shader,
not the actual draw's 390EB240. This is NOT an attributable gun-palette caller.
The single global witness in Hook_SetVSConstF is overwritten/invalidated by any
overlapping low-register upload, and its VS source is shadowtrace::bound.vs.
We cannot distinguish an invalid original native write from an intervening write
using these unmatched samples.

- A: UNPROVEN (no matched original-upload data).
- B: UNPROVEN (no matched original-upload data).
- C: disproven for the observed valid-to-invisible collapse; it is already
  present at incoming, not introduced by stereo. Some later submissions repair
  the linear component temporarily rather than causing it.
- D: disproven for the 24 all-zero submitted samples.
- E: final classification for original-producer attribution and held-L2/R2.

## Important existing-path confounder

Log line158 enables AdsMaskAnim=1, c5-c22, tail400ms. Source AdsMask() writes
the pre-aim cached constants; Hook_DrawIndexedPrim calls it after the incoming
probe and before submission. Its gate is raw alt trigger/ScopeActive, separate
from the injected Impulse edge. AdsFreezeLo/Hi are -1. This is a concrete
candidate explanation for f6055-6086 repeating a nondegenerate palette, NOT a
proven event-level attribution (no AdsMask receipt/trigger value at those draws).
Do not introduce another stale-pose fix or remove this behavior speculatively.

## Budget and held-L2/R2 gap

64 draws and39 events were captured. At draw64 the probe sets
g_volkSkinFinished=1, which also prevents further event receipts. All recorded
FireEnter/Exit events precede the first recorded ImpulseDown. Thus the required
R2-while-L2-held five-bone interval is NOT captured. Later generic dual-gun
records (e.g. f6216) show c5 recovery but not the full five bones or input edge;
they cannot replace the missing observation. Physical held-L2/R2 success remains
accepted as physical evidence, with no invented timestamp.

## Targeted next work for SOL (no new test requested here)

Trace the constant producer offline first. If a new measurement is essential,
retain the actual consumed register-write provenance (including partial writes,
pre/post-hook and original-setter HRESULT, sequence/frame/device) rather than
one unrelated last upload. Use a fixed ring/per-register witness; avoid assuming
constants are scoped to a shader, since D3D9 constants survive shader binding.
Reserve bounded capture capacity for the first L2 edge and subsequent R2 while
L2 is held; coalesce nested fire entries to one root event and do not spend the
budget on startup/Grip. Record raw alt trigger, Impulse held state and whether
AdsMask actually overwrote c5-c19. No GPU readback, native power mutation,
handsStatus forcing or cached-pose rendering fix is justified by this capture.

The newest user request changes support acceptance: retain the short sleeve AND
TMD on the supporting wrist. This is separate from gun collapse; implement on
a separate feature/fix branch, not by changing this evidence branch.

## Restore receipt

Game-closed check used full Win32 process context. Only DLL/INI restored through
tools/deploy-singularityvr.ps1 Rollback; preflight and postflight PASS.
Backup: MAIN deployment/deploy-backups/20261023-pre-volk-skin-8fbb9f4.
DLL SHA256: BEA5A5623E2995C5DEE85AC30D1736F0B3E3CEAF68AF20A84B86B7DA3E83D581.
INI SHA256: 00200C0572C99D1656E294C1F312D104F27A9B4CB44BD5CD0890AEAB22387A79.
Both reread from the installed files and exactly match the backup.
INI diff before restore: only diagnostic flag addition and BOM removal.
No game assets, saves, other DLLs or global accepted-profile promotion changed.
MAIN checkout lacked a current accepted-profile file: a private one-shot rollback
profile was built from the verified backup INI/DLL, with unchanged dependency
pins from the historical approved menu profile (all validated by the guard).
It authorizes ONLY this user-requested rollback, not a new accepted build.

## Reproduction

`python tools/analyze-volk-skin-capture.py <archived-view_matrix.log>` emits
full JSON with source line references; selects only probe records. Preserve the
distinction between previous-draw snapshots, no-eye incoming snapshots and the
actual incoming eye0 read. No production DLL was rebuilt by this evidence task.
