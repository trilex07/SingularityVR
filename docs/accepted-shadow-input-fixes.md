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
