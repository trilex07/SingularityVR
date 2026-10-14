# Wire-up: `DesktopMirrorEye` (3 edits in `d3d9.cpp`)

Logic lives in `desktop_mirror.h` (or inline in cpp — see handoff).

## 1. Globals (near `g_startInVr`)

```cpp
volatile LONG g_desktopMirrorEye = 0; // ini [Render] DesktopMirrorEye
```

## 2. INI read (after `StartInVr` log)

```cpp
{
    LONG mirror = GetPrivateProfileIntA("Render", "DesktopMirrorEye", 0, path);
    if (mirror < 0 || mirror > 2) mirror = 0;
    InterlockedExchange(&g_desktopMirrorEye, mirror);
    Log("ini: DesktopMirrorEye=%ld", mirror);
}
```

## 3. Present (after `UpdateFromHeadset(s);`, before `g_origPresent`)

```cpp
#include "desktop_mirror.h"
// ...
desktop_mirror::Apply(s);
const HRESULT presentResult = g_origPresent(s, a, b, c, d);
```

## User INI (TV)

```ini
[Render]
DesktopMirrorEye=2
```

Full notes: `docs/handoffs/desktop-mirror-tv-handoff.md`.
