# M02 P2 + M09 combined session — physical grading record (2026-10-24)

Single session on the combined candidate (game closed before archiving).

## Session and installs

- Session window: log attached `2026-10-24 10:12:09`, ends `10:13:23`
  (1,520,174 bytes). Complete log archive SHA256:
  `305C4C2D56C13744E946EED8A146644A96FF04AFBA70421B1AD44D06E29A96C4`.
- Installed and verified: dll
  `E304F22559C6EEF2D8055B73B48518870F1B24199C8BDD2753CCB5A9798E63C6`
  (commit `d104a33`), INI `DA63A2A9…9FBC` (`UiRemasterRuntime/Settings/
  Wheel=1`, `[Haptics] TmdPulses=1`), atlas hashes unchanged (E4DC0E7D…,
  6436AA25…, 53B570C4…). Archive companions: `installed.ini`,
  `install-report-combined.json` (20261024-101113).

## M02 P2 weapon wheel — PHYSICAL PASS

- Receipts: 5 `M02 UI` lines (neutral atlas ready + `plate eyes=2/2
  restore=1 back=5640x2876` ×4); the wheel path logs only on failure —
  failure scan (`fail|fallback|unavailable|unsupported`): **0 hits** ⇒ no
  duplicate wheel, no failed composite, no fallback ever engaged.
- User (quoted): "PASS, everything works." Wheel, weapon switching,
  firing/reload and held-weapon ammo checked by the user in the same run.
- **R2/weapon icons visually too simplified: ACCEPTED COSMETIC FOLLOW-UP**
  (SOL; not a blocker; no physical claim beyond the wheel slice).

## M09 TMD pulses — PHYSICAL PASS (+ root cause proven fixed)

- `TMD CLASSIFY [1..3/24]: event=start object=0F8E2A00 readable=1 **live=0**
  classPtr=0D619880 class=RvTMDPower_Impulse tmd=1C69AE00 local=1
  classified=Impulse decision=Impulse` — Session B's failure is now
  fully explained: the OLD code returned early on `IsLive=0`; the receiver
  was readable all along and its class resolves as `RvTMDPower_Impulse`.
  Known-power path used (Generic fallback not needed).
- `TMD EVENT [1/24]: start power=Impulse ret=0 [activated] accepted=1
  armed=1`; `[2] ret=3 [cooldown_active] accepted=0 armed=0` — the re-trigger
  cooldown was correctly refused (gate working).
- `SUBMIT [1/32]: hand=0 (LEFT) … overlay=0.800 merged=0.800 | gate(all-1)
  | amp=0.800 -> xrResult=0` — REAL OpenXR submission, nonzero amplitude,
  accepted (5 submits, 4 events, LEFT/TMD hand).
- `STOP [1/16]: hand=0 wasAmp=0.800 -> xrStop result=0` ×3 — bounded stop,
  nothing stuck (receipts + user report consistent).
- `TMD NON-LOCAL`: 0. `[Haptics Trace] XINPUT`: 0 — VR-session xinput
  transport behavior unchanged; gun firing feedback (adaptive triggers)
  unchanged per user. Physical: user felt actual controller vibration.

## Grades (recorded independently)

- M02 P2 weapon wheel: **PHYSICAL PASS** (scope: wheel visuals/switching/
  ammo; icons = accepted cosmetic follow-up).
- M09 TMD controller vibration: **PHYSICAL PASS** (Impulse activations; the
  user reported pass for everything; per-power enumeration of non-Impulse
  powers was not itemized by the user — no deeper claim).

## Rollback state

Backup `deployment/install-backups/20261024-101113` = P1-accepted pair
(dll `107FA9C0…`, INI `994B111A…`) verified by the install report.
