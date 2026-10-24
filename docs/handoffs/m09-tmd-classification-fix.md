# M09 TMD power classification — SIDE

## Observation and falsifiable hypothesis

Session B captured eight original native Start/StopFirePower calls, all rejected before the pulse engine with an empty class name. No OpenXR submission occurred. Source inspection shows `Classify()` returns immediately when `IsLive(power)` is false. `IsLive` requires an Outer chain reaching `PersistentLevel` within six links; that is an instance-lifetime heuristic, not a required condition of the fingerprinted native power method. The current classifier also does not compare the immediate class name before relying on superclass reflection (`g_upNext`).

Hypothesis: a valid native power receiver can fail this unrelated `IsLive` heuristic or have unavailable superclass reflection. The next bounded runtime receipt must separate `object readable`, `IsLive`, raw class pointer/name, `IsLocal(tmd)`, and final decision. If the object is unreadable, or the hardened local-TMD ownership check fails, Generic must remain rejected. If class resolution fails on a readable receiver while `IsLocal(tmd)` is true, the existing successful StartFirePower result may schedule a bounded Generic pulse.

This is a controlled correction to the first proven failing stage (classification); the exact live sub-stage cannot be proven from Session B because that log omitted the class pointer and IsLive result. No Steam or PSVR2 work is performed here.

## Correction and safety

- The class reader now checks a readable native power receiver and resolves its immediate class name before attempting superclass reflection. `IsLive` is diagnostic evidence, not a class-identity veto for a TMD-owned subobject.
- When the class remains unknown, `SelectLocalPower` chooses Generic only if the receiver is readable and the unchanged hardened `IsLocal(tmd)` check positively confirms local TMD ownership. Unreadable receivers and foreign or unproven TMD owners remain rejected. Successful native StartFirePower (`ret=0`) is still required to arm a pulse.
- Generic uses a single 0.40-amplitude, 70 ms pulse. The existing 200 ms OpenXR refresh duration and all focus, tracking, pause, menu, cinematic, cooldown, and `TmdPulses=0` gates remain unchanged. StopFirePower still cannot arm a pulse.
- A bounded `TMD CLASSIFY [n/24]` receipt records object, readability, `IsLive`, class pointer/name, TMD, local ownership, classified power, and final decision. No per-frame logging was added.

## Offline evidence

- Native Start/Stop binary fingerprint tests: 7/7 PASS.
- Haptics policy tests: PASS for all seven known names, local Generic fallback, foreign/invalid rejection, STOP behavior, default OFF, and gate clear.
- Dual-TMD, reload, two-hand and WeaponLocker regressions: PASS.
- D3D9 and D3D9Ex short-gauntlet fixtures: PASS (2x7110 draws; IB restored; 3304/3304 pixels).
- x86 PE32 `/O2` DLL: PASS; SHA256 `DA9386C606A4EEE87597ACDE21AF86563BCBE132E9420823ECD27970F75FAE3F`.

## MAIN acceptance (separate M09 session after M02)

With `TmdPulses=1`, use one local power Start/Stop, then pause and resume. Expected: `TMD CLASSIFY ... local=1 ... decision=<known power or Generic>`, `TMD EVENT ... accepted=1 armed=1`, nonzero `SUBMIT ... xrResult=0`, and `STOP` or natural expiry with no stuck vibration. A foreign or unproven owner must show `decision=None` and no submit. L2_PASS only; physical PSVR2 result remains NOT TESTED for this correction.
