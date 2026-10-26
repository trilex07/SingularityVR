# Final V3 startup failure: evidence and next gate

Status: live Steam startup FAIL; OpenXR NOT REACHED; physical PSVR2 NOT RUN.
Both attempts are fully rolled back. Current Steam baseline is AB348E06.
Root cause: UNPROVEN. No rendering, audio or UI source patch made from this fault.

## Exact candidate identity

- Branch: `integration/final-completion-20261026`.
- Deployed integration commit: `37d3bdf005834af2cee92f6c75a47004ff58472d`.
- Runtime source: `38ca72f88112bfcbd26addb0d6aa4f76095aa15e`.
- Renderer SHA256: `23A3A768C44F1C6EB5479D899141924C4C44D5C6A73A3175A2827241105370B1`.
- Bridge SHA256: `F58CB3A805E46C3D3F7E88137BD605AF21A8A8106C289385132F677A968E95FD`.
- Full manifest SHA256: `419975c167fcda2519866c5bbf4b45da6da2f9701ac1a65717e2db2cf640d97e`.

## Controlled observations

1. Full V3 session `20261026-124237`, PID 13480: all 130 assets and four
   runtime files verified. Guard READY before Steam AppID 42670 launch. WER
   reports `c0000005` at EXE RVA `0xACA50`. Log ends after successful
   `Direct3DCreate9Ex`, before device/OpenXR initialization.
2. UI control session `20261026-125629`, PID 31072: identical runtime and
   remaining 129 assets, original `UI_SinglePlayer_SF.xxx` retained. Same AV,
   faulting instruction, global values and startup-log boundary.

The control excludes the new menu package as a necessary condition. It does not
identify another package or DLL as the cause. No third launch was attempted.

Private evidence directories, relative to the worktree:

- `build/next-weapon-reload/final-complete-v3-20261026/sessions/20261026-124237/`
- `build/next-weapon-reload/final-complete-v3-ui-control-20261026/sessions/20261026-125629/`

Each retains the dump, startup logs, `allocator-witness.json`, live-session receipt
and guard output. Original WER dumps are `Singularity.exe.13480.dmp` and
`Singularity.exe.31072.dmp` under the user's real LocalAppData/CrashDumps.
Private game binaries/content are not committed.

## Machine-code evidence

The captured executable base is `0x00400000`. The disk executable is packed;
the following code was read from unpacked memory captured in both dumps:

```text
004ACA40  mov eax,[01CD4A5C]        ; available-slot Num
004ACA45  test eax,eax
004ACA47  je 004ACA67
004ACA49  mov ecx,[01CD4A58]        ; available-slot Data
004ACA4F  push edi
004ACA50  mov edi,[ecx+eax*4-4]     ; bytes 8B 7C 81 FC, read AV
004ACA54  push 1
004ACA56  dec eax
004ACA57  push eax
004ACA58  mov ecx,01CD4A58
004ACA5D  call 004D4DC0
```

Both dumps have Data=0, Num=-54 (`FFFFFFCA`), Capacity=0 at `0x01CD4A58`.
The effective fault address is `0 + (-54)*4 - 4 = 0xFFFFFF24` on x86.
The allocator takes its nonempty branch because it checks zero, not the sign.

PROVEN: this native array invariant is already violated when the allocator reads
it. This is the first *observed* invalid contract, not proof of the first corrupting
operation. Do not patch this read to conceal the earlier corruption.

UNPROVEN: initial writer, corrupting caller, responsible asset/runtime component,
or whether one fault causes both sessions through the same original path. The
exception context is already in the error handler; raw stack scanning did not
establish a reliable original caller stack. DLL addresses found on that stack
must not be reported as causal return addresses.

Older project incidents at the same allocator RVA involved a modified
`RvGame.xxx`. That package was unchanged in this candidate; history is a useful
search lead, not evidence of the current cause.

## Restoration and acceptance

First guard restored all 130 exact asset states; control guard restored all 129.
Both then restored the four runtime files and passed full accepted-profile
postflight (159 file pins, plus semantic INI). An independent final hash read
confirmed the installed renderer/INI/bridge/texture INI/UI package:

- `AB348E06080E479988E810375CC50D92EEA1936D94313A02360517269DC4F964`
- `9E68D94401ADD0986CF7ACB8E922F805BD3DCFBCAD967BE59388FAA13DF8831C`
- `99FF4BF5C49B32ECFE3E7F921F4B17AFD28E82390FAC8B34B9F5308E46100A55`
- `2492C8D08A067C9097AD24ACA5D8AA77EB56B883914369087C7FB6471F2D1BF7`
- `51CDAB5C892A7B682D89319721330AA3E78C7013D1C60E1476A0115431F01A1A`

Offline build/asset regressions remain PASS at their original evidence level.
They do not override startup FAIL. Age, menus, materials, new audio, Seeker and
frame times received no new in-game/physical acceptance. The 551 pending SIDE
takes remain unchanged and are not newly deployed XTTS recordings.

## Next minimal isolation, before requesting headset time

NO-GO for either failed payload. Reuse the captured exact binaries and dumps;
do not rebuild or change arbitrary rendering constants to chase this startup AV.
Inspect startup initialization and modified native-bank metadata first. The next
controlled comparison should separate the V3 runtime pair from all V3 asset
replacements, using accepted AB assets and its verified original configuration.
If that boots, isolate the asset groups; if it reproduces this AV, isolate renderer
versus bridge with their documented compatibility contracts. Any comparison
needs a fresh scoped profile/backup/guard, recorded hashes and identical fault
witness. A request to deploy the combined candidate is not physical acceptance
of these components. Do not repeat PSVR2 tests until startup is resolved.
