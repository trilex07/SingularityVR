# L3 sandbox report — 2026-10-10

## Verdict

**L3: BLOCKED**.

The test harness and oracle were exercised, but the copied UE3 game process
crashed before the first render frame. No capture from this run is claimed as
valid. No Steam file, production DLL, or process owned by another agent was
modified or stopped.

## Sandbox and provenance

Sandbox:

`C:\Users\Trilex\vr-mods\l3-sandbox-20261010`

The sandbox contains copied `Binaries`, `RvGame`, `redist`, and `Docs`. Its
`APPDATA`, `LOCALAPPDATA`, `TEMP`, and `TMP` were redirected into the sandbox.
The process was launched directly, not through Steam, with its own
`XR_RUNTIME_JSON` and `DVR_XRSIM_DIR`.

| Artifact | SHA-256 |
|---|---|
| sandbox `Singularity.exe` | `BD459ACD87D4B82693928DEE4ED437C086D5928A8E0428F3AD54B6FC114C0F51` |
| sandbox copied `Binaries\d3d9.dll` | `8C1E0F30C6BE7721DFA972F7EAA893348FC2E326B3DEBBE726D6B2DF683C7A5E` |
| sandbox `xrsim\dvr_xrsim32.dll` | `5D86A93B7D27E2E6ECC0DC7501AAD492468D4C554CEA119965D2E4B9870D1ECC` |
| sandbox `xrsim\dvr_xrsim32.json` | `860645F4D4CE67FC2DD86362755CD79CF286FD64C6A3111B13D245512725AE78` |
| final crash dump | `0BD93569B3B9E783A1FD92601E57AAE3B7883C08BDE1C9A1CDE38EEEC3C1CEFA` |

## Launch attempts

Direct sandbox launches:

| PID | Environment | Result |
|---:|---|---|
| 35684 | xrsim + copied d3d9 | exit `0xC0000005` before render |
| 38852 | copied game, d3d9 disabled | exit `0xC0000005` |
| 40640 | d3d9/dinput8/OpenXR loader disabled | exit `0xC0000005` |
| 31016 | Steam root added to child `PATH` | exit `0xC0000005` |
| 34692 | full sandbox + redist/Docs + xrsim | exit `0xC0000005` before render |

Crash dump for the final attempt:

`C:\Users\Trilex\AppData\Local\CrashDumps\Singularity.exe.34692.dmp`

The crash is reproducible independently of the VR DLLs: the base-game probe
also crashed after `d3d9.dll`, `dinput8.dll`, and `openxr_loader.dll` were
temporarily disabled **inside the sandbox only**. This rules out the new
capture oracle as the cause. The remaining blocker is the copied UE3
executable's bootstrap/Steam path dependency or another host-specific launch
requirement.

## xrsim and replay evidence

The requested `.vrrec` was validated read-only:

`C:\Users\Trilex\vr-mods\deployment\agent-test-bridge\20261009-192634\session_2026-10-09_14-18-07.vrrec`

- 5573 source samples;
- 1420 replay batches at stride 4;
- duration 80.641 seconds;
- fire 8, melee 2, recenter 1, UI clicks 6;
- canonical dry-run: **PASS**.

No live replay was sent to another agent's process. The existing
`xrsim-scratch-run/state.json` identified PID `33184`, not any sandbox launch
PID, so it was not used as L3 evidence.

## Capture results

- Real sandbox capture L/R: **0 / 0**.
- Valid `Collect` manifest: **none**.
- Valid `VerifyCapture` oracle run against a sandbox frame: **none**, because
  the game did not render a frame.

The bridge was nevertheless exercised against an old xrsim capture:

`xrsim-scratch-run\capture\hands-section-leftmove3.json`, frame `5424`.

It was rejected before manifest creation because the acknowledgement belonged
to frame `2837`, and the current xrsim state belonged to PID `33184`; it could
not establish command → acknowledgement → frame → capture identity. This is
the expected stale/mixed negative result.

The oracle's L1 negative test separately confirmed that a valid old manifest
with the same identity/image hashes returns `FAIL` for `--previous`.

## Tests executed

- L1 oracle suite: 5/5 passed.
  - synthetic fresh L/R: PASS;
  - stale capture: expected FAIL;
  - swapped eyes: expected FAIL;
  - missing effect against trusted baseline: expected FAIL;
  - unknown baseline: expected INCONCLUSIVE.
- L2 Python compile: PASS.
- L2 PowerShell parser: PASS.
- Canonical `.vrrec` dry-run: PASS.
- L3 sandbox launch: BLOCKED before first frame.

## Required follow-up

Do not loosen the identity checks. The next L3 attempt needs a supported way
to launch an isolated UE3 copy — for example, a project-provided portable
bootstrap or a filesystem/container strategy that preserves the executable's
required Steam-relative context without writing to Steam. Once the game stays
alive, the existing sequence is ready:

`launch → replay → ack → render → capture → Collect → VerifyCapture`.
