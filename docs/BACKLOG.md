# Delivery backlog

## P1 — Reproducible native pause UI installation on a clean owned game copy

Status: NOT IMPLEMENTED. Runtime layout is physically accepted on PSVR2, but
merging PR #13 is not a complete end-user installation workflow.

Verified gaps: packaging/make-release.ps1 stages the two DLLs and INI, not the
native UI package generator. The SIDE research generator is absent from main.
README's VDXR-only/SteamVR-planned guidance predates PSVR2 SteamVR OpenXR tests.

Acceptance criteria:

- Bring in and review the minimal research generator/patch logic, not game assets.
- Generate the RTT modification locally from the user's own UI_SinglePlayer_SF.xxx.
- Validate supported input versions/hashes; reject unknown/already-modified inputs
  safely, with explicit diagnostics and idempotent reinstallation behavior.
- Refuse writes while Singularity.exe is running; verify backup hashes before
  modification, stage and verify output, and atomically install where supported.
- Maintain a manifest of input/output SHA-256 and generator version; provide a
  verified rollback/uninstall that does not overwrite unrelated user changes.
- Package the generator/installer and rollback scripts with the release; do not
  redistribute original or modified proprietary game assets.
- Verify a fresh supported installation, repeat install, interrupted/failing patch,
  and rollback, plus a real headset pause/menu test.
- Update README/runtime guidance: PSVR2 via SteamVR's OpenXR runtime has been
  physically tested; distinguish this from a separate native OpenVR backend.
  Document UI-file modification, backup, requirements and current limitations.

Until these criteria pass, do not claim the existing three-file release archive
alone reproduces the accepted native pause menu on a clean installation.
