# M02 P1 — physical grading record (2026-10-24)

## Session and installs

- Session window: log attached `2026-10-24 10:00:17`, ends `10:01:40`
  (2,129,797 bytes). First P1 styling session on the safety build.
- Complete log archive SHA256:
  `5E990066E966D94FE95ED2AD8E379DE1516D07AD89C81E8075317AD4E7264B61`.
- Installed and verified: dll
  `107FA9C01047F8BBFED4D53BB9C872964173EAE72E89308CBF89713002B71120`
  (unchanged safety build), INI `994B111AE7E65283E0D396D03657B02A566A073C
  59B0DE28CD93489B95866314` (`UiRemasterRuntime=1`, `UiRemasterSettings=1`,
  `UiRemasterWheel=0`, `TmdPulses` absent).
- Archive companions: `installed.ini`, `install-report-p1.json`
  (20261024-095837).

## Receipt evidence

- `M02 UI: neutral atlas ready 4096x2048` + `plate eyes=2/2 restore=1
  back=5640x2876` ×4 — P0 plate still drawing/restoring correctly.
- P1 styling path logs ONLY on failure (`M02 UI: <pl|en> atlas unavailable;
  native interface retained`, src d3d9.cpp L27705): **0 hits** ⇒ the
  localized atlas loaded and the styling draws executed.
- `ini: Polish localization initialized (PolishLanguage=1, Fallback=1)` —
  the PL atlas tab/footer was the exercised presentation. **EN presentation
  was NOT exercised this session; no separate PL/EN acceptance is claimed.**
- Failure scan `M02 UI.*(fail|unavailable|unsupported)`: **0 hits**.

## Physical grade (user report, quoted)

> "Everything works. The R2 icon is a little small, but it can stay like
> that."

**P1 (VR Settings localized styling): PHYSICAL PASS for the tested scope** —
- Settings appearance and interaction: PASS.
- R2 icon size: minor cosmetic observation **ACCEPTED as-is** (explicitly
  not blocking; no resize work requested).
- PL/EN language switch: not claimed without evidence.

## Next stage (deployment record, verified)

Combined M02 P2 + M09 session: P2 enable (`UiRemasterWheel` `0`→`1`) plus
M09 `f73b91d` integration (new dll) and `[Haptics] TmdPulses=1`, ONE guarded
deployment, ONE physical session with separately graded sections.
