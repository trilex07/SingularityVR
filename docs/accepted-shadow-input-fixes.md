# Accepted shadow/input fixes

Physical PSVR2 acceptance2026-10-13: dynamic enemy shadows; textured/controller-
tracked gun; no controller-following background-negative; matching gun/glove
shadows between eyes. User explicitly considers shadows finished after PID39308.
Input cache was separately accepted with held controls and pause/options/resume.
Other headset/model/scene coverage and controller hotplug remain untested.

Production configuration (missing keys default ON, including old INIs):

```ini
[Render]
ShadowWorldReceiverFix=1
ShadowForegroundClearFix=1
ShadowFirstPersonReceiverFix=1
[Input]
InputEnumerationCache=1
```

Keep High Quality Decals ON. To disable an individual fix, set its key to0 and
restart while the game is closed; no installed file replacement is necessary.
First-person receiver requires both world receiver and foreground clear fix.
Its near1 scope and world near10 scope remain separate. Native Clear, shader
code, bone palettes, glove selection, textures and UI are not replaced.
New INI switches only persist the same logical state used in accepted tests;
the production-configured artifact needs its own recorded build provenance.

Owned 20-minute diagnostic ON flags still opt in when an INI key is0. Remove
those flags using checked-in tools before a disable/control test. Without flags,
INI state persists indefinitely; diagnostic capture/profiling stays default OFF.
Legacy rejected mixed receiver correction remains default OFF and is not part
of this accepted set. No proprietary game assets/captures are distributed.
UI package installation/installer work is separate from these DLL render fixes.

## Verification and release build

PSVR2 visual acceptance uses source3488084/DLL5D49C7A5, real SteamPID39308,
OpenXR confirmed,1196draws/304groups/overflow0. Separate earlier tests confirm
enemy shadows, gun material/tracking, background-negative removal and input FPS.
Physical acceptance is recorded from the user's results, not counters alone.

Persistent-default promotion:9 x86 /O2 C++ unit executables pass, including
captured eye matrices, scoped restoration/failure latch, native input callback
replay/invalidation, trace lifetimes and policy gates. Python trace/CPU-parser
regressions and source+shipping-INI default-policy checks pass.

Regular release build (no Performance switch), source
1b4e0b6a366098b071a9347ae53b54116e57351d, x86 PE32 /O2,
SHA256 E9DC831B599734CD3EC6CB8A16C907DE26F7D9EB8CEC29E95A5DD2C104D8116E.
Manifest and actual DLL hash match; no tracked source edits during build. This
final default-configured artifact is L2_PASS, not separately Steam/PSVR2-tested;
the accepted algorithms/scopes are unchanged. No CI workflow currently present.
MAIN preserved verified backups and left installed DLL9FBA308C/INI C59B1CF6/
UI5B2EA469 after test exit, rather than deploying the PR-preparation artifact.
