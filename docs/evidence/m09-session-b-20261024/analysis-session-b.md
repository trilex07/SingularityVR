# M09 Session B — physical FAIL record and pipeline analysis (2026-10-24)

Session window: `2026-10-24` (log ends 09:29:24, 1,725,622 bytes, 13,180 lines).
Complete log archive SHA256:
`ED29829DB33947EFB3216C26B882342C4217E25F0799CBC1A7B10CB521C5AFB0`.
Game closed before archiving; the log truncates per session, so this file IS
the complete Session B run. Archived alongside: `installed.ini`
(`DB2C6CFF…854B`, the Session-B INI with `[Haptics] TmdPulses=1`),
`install-report-sessionb.json` (guarded install 20261024-004108),
`d3d9.build.json` (copied from the session-A evidence dir; same DLL bytes
`92D1BABC…C9C4` — Session B changed the INI only).

## Corrected grades (user report)

- Normal gun feedback: **PHYSICAL PASS** (adaptive-trigger effects; unchanged
  accepted-baseline behavior — see session-A analysis).
- Stationary-hand firing feedback: **PHYSICAL PASS** (fired with the
  controller completely motionless; triggers still produced effects).
- **Advanced TMD power pulses: PHYSICAL FAIL** — the user feels absolutely
  no vibration when using TMD powers, whole session.
- No stuck vibration after pause: PASS. No gameplay/input regressions.

## Pipeline trace — first proven failing stage: B (event captured, rejected at classification)

1. **Native event source — WORKS.** `TMD haptics: verified native
   Start/StopFirePower observers installed` (log L372); 8 live hook hits
   during the session. Detour signatures correct; the native event path IS
   alive.
2. **Power classification — FAILS, the rejection point.** Every captured
   event ends as:
   `TMD haptics: unclassified power object=0F8E2A00 class= [ignored]`
   (7× recurrent address — a stable TMD-related instance, almost certainly
   the player's persistent power object) and 1× `object=139D9760`.
   `class=` is EMPTY in all of them.
3. **Ownership / scheduler / OpenXR submit — never reached.** `TMD EVENT`
   receipts exist only for CLASSIFIED powers; none exist. Pulse engine never
   armed.
4. **SUBMIT: 0, STOP: 0, XINPUT RUMBLE: 0** — and the caps prove NOTHING was
   hidden: SUBMIT's own cap is 32 starting from zero; any successful
   submission anywhere in the session would have logged receipts into an
   empty cap. Zero receipts ⇒ zero submissions for the entire session.
5. Comparator within the same session: `cutscene-state:
   controller=2EB03200 class=RvPlayerController …` and `scope PiP:
   captureClass=0588B680 …` — `P(obj+OBJ_CLASS)`, `NameOf`, `ClassHas` all
   function correctly on live objects in THIS session. So the failure sits
   inside `NativeTmdHaptics::Classify((uintptr_t)self)` returning
   `Power::None` for the power object.
6. Sub-stage granularity — the current unclassified receipt does not print
   `IsLive(power)` result or the raw class pointer, so these four
   possibilities cannot be separated from this log: (a) `IsLive(power)`
   false; (b) `P(power+OBJ_CLASS)` null; (c) class-object name resolution
   empty for THIS object; (d) name present but not in the hardcoded
   `kMap` (7 exact names) and not equal to the class's own name. Evidence
   until now is therefore stage-B-confirmed with sub-stage INCONCLUSIVE —
   this is precisely the one missing bit of instrumentation.

## Disproved alternatives (evidence)

- NOT stage A (no native event): observers installed + 8 hook events logged.
- NOT stage C/D (accepted but nothing submitted / XR rejected): nothing was
  ever armed; the pulse engine path never reached a nonzero amplitude.
- NOT premature STOP or focus loss: nothing to stop; ClearAll on
  non-FOCUSED is irrelevant to an engine that never armed.
- NOT receipt-cap masking: caps start empty for SUBMIT and STOP; only the
  unclassified receipt pool (cap 8) reached its ceiling, and 8 ignored
  events cannot arm anything.
- KMap entries themselves were proven not the full cause by the FIRST five
  receipts: even when the class node would be readable, an empty `class=`
  column indicates resolution failure before map comparison.

## Exact session-B configuration (for the record)

DLL `92D1BABC…C9C4` (unchanged from Session A), INI `DB2C6CFF…854B`
(effective `Enabled=1 Level=1 TmdPulses=1` — visible in session log
`ini: Haptics Enabled=1 Level=1 TmdPulses=1`). Rollback point for this
session: `deployment/install-backups/20261024-004108` (Session-A pair: DLL
`92D1BABC…C9C4`, INI `D2E91B0B…03D8` — verified before rollback).
