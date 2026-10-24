# M02 P0 — physical grading record (2026-10-24)

## Session and installs

- Session window: log attached `2026-10-24 09:54:18`, ends `09:55:27`
  (2,055,667 bytes). This is the first headset session on the **safety
  build** (installed 09:53:16, guarded dll-only deploy, INI unchanged).
- Complete log archive SHA256:
  `D9D5C52327B235C850AD520A05ECA12B54932C7077FE5420501DE820991984D9`.
- Installed and verified this session: dll
  `107FA9C01047F8BBFED4D53BB9C872964173EAE72E89308CBF89713002B71120`
  (commit `7d156f8` = `6d08f4d` + cherry-pick `-x b49380d`), INI
  `EB8EEA0AD2F33B503BE2CE643924B74715E51A9BB7B1C3F4FC1E7E8657D4CA0B` with
  `UiRemasterRuntime=1`, `UiRemasterSettings=0`, `UiRemasterWheel=0`,
  `Haptics/TmdPulses` absent.
- Archive companions: `installed.ini`, `install-report-safety.json`
  (20261024-095317), `d3d9.build.json` (provenance `7d156f82114c`).

## Receipt evidence

- `M02 UI: neutral atlas ready 4096x2048 (runtime draw still requires panel)`
  (L2891) — the fail-closed WIC atlas load succeeded and the runtime gate
  armed exactly where the panel needs it.
- `M02 UI: plate eyes=2/2 restore=1 back=5640x2876` ×4 (L2892–L2898) —
  the plate drew in BOTH eyes on every sampling and restored device state.
- Failure scan `M02 UI.*fail|unavailable|unsupported`: **0 hits**.

## Physical grade (user report, quoted)

> "The new VR Settings works and looks good."

**P0 (VR Settings plate runtime slice): PHYSICAL PASS.**

Scope honestly recorded: the user confirmed the plate visuals and that the
Settings screen works. The **Reset path and repeated open/close
interaction edge cases were not explicitly reported**; no evidence of a
blocker exists (restore=1 and zero failure receipts), but those edge cases
remain untested-this-session and are covered by the standing P0 gate list;
any regression there would be caught by the P1 session's repeat open/close.

## Next stage (deployment record, verified)

P1 enable: guarded INI-only deployment — `Render/UiRemasterSettings`
`0`→`1`, dll written byte-identical (`107FA9C0…`), every other accepted key
untouched (Runtime stays `1`, Wheel stays `0`, TmdPulses absent), atlas
hashes pinned (`E4DC0E7D…`, `6436AA25…`, `53B570C4…`). Rollback switch:
set `UiRemasterSettings=0`.
