# PSVR2 client loader discovery

Baseline: `fa4c3151`, the exact source of the currently distributed renderer.
The x86 renderer passes a fixed C-drive Steam/CAPIApps path to the x64 bridge.
The installer previously extracted CAPIApps into a random, unrelated AppData
directory. A calibrated Toolkit driver therefore did not guarantee a loadable
client. Falsifier: a fresh per-user fixture must resolve a loader without any
legacy Steam path; an explicit existing loader must still win.

The bridge now tries `%LOCALAPPDATA%/Trilex/PSVR2Toolkit/capi/` only when the
requested loader is missing. No CWD/PATH search, ABI change, renderer, IPC token,
gaze pose or adaptive-trigger change. It logs the selected path once per load.
The installer provisions the pinned experimental-3 loader at that location.

Validation: optimized x64 PE32+ bridge; `test-loader-path.ps1` PASS for missing,
explicit, Unicode per-user and absent-environment cases. Fixtures never load
CAPI or launch a game. Bridge SHA256:
`6B5BCBF5ACE2C2A69DF20E9B65C87366BC32C1D63CBEBB617D8C1FED9162DA93`.
The distributed x86 renderer remains byte-identical
`E0E08775939176C090BDAD8FEC47C6BA81B3E34A8B288A7A4A80EDCE76253220`.

No installed Steam file changed. Remote Toolkit version, SteamVR driver active
state, calibration and physical gaze acceptance remain unverified. This fixes a
proven packaging failure; it does not establish the cause of every player report.
