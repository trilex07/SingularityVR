# Production runtime integration checkpoint

Status: CONTINUE WITHOUT HEADSET. Not a completed menu/prompt/shadow fix.

Production branch is checked out in `SingularityVR-production`; the canonical
checkout remains on the other agent's NIS branch. No NIS integration was made.
Existing `grip_latch.h` and `fp_shadow.h` from the helper work are now consumed
by production rather than duplicating their implementation.

Implemented:

- Grip sampling moved outside the configurable XInput binding loop. Release,
  invalid gameplay/menu state, and weapon identity now invalidate support.
  Tracking grace uses the helper's clock, and applies only to an acquired grip.
- Provider vertices and pointer hit testing consume `VrMenuPanel`. The provider
  now projects tracking-space corners through the same XR projection as the laser.
- Removed absolute Y inversion. Native getter `0x011CFC40` calls GetCursorPos
  (IAT `0x017836B8`) and ScreenToClient (`0x01783710`), then returns POINT.y
  unchanged. MouseY delta sign is a separate convention.
- Removed calls rendering the permanent PS legend. A contextual replacement is
  NOT implemented: native prompt lifecycle remains unhooked.
- All draw entry points bypass VR transforms for classified shadow-depth shaders.
  Indexed shadow omission requires retained VB/IB/offset/stride/range identity
  from accepted foreground color geometry. UP draws are never omitted solely by
  vertex count. Cached COM resources are released on weapon change/device reset.
- Added transition-flushed counters. Cursor/prompt counters are reserved and
  explicitly marked unresolved; their zero values are not verification.

Validation:

- x86 `/O2` build and format-string analysis succeeded; PE machine 0x14C,
  optional header 0x10B.
- `test-production-runtime`: 7200 samples using production panel geometry,
  top/25/center/75/bottom, sizes/head rotations; no-grip/release/grace/switch.
- `test-vr-interaction`, `test-parallel-components`, and
  `test-reference-integration` rebuilt and passed. These do not prove live
  menu suppression, cursor suppression, or prompt replacement.
- Launched through Steam. Fresh log confirms missing-mouse guards, graphics,
  OpenXR, native viewport binding, IIS -> Main -> gameplay, and grip acquire/release.
  This is ONE successful launch, not three; the game remains running.
- Live Main counters: captured=0, suppressed=0, provided=846. The texture was
  retained from IIS. Thus original menu suppression remains unproven and the
  old generic capture rule is demonstrably insufficient. Do not claim one image.

Built and deployed DLL SHA-256:
`99FF6B97F72BCDE3EE3EA5D59045D0A480CAB88B69D8AE607013B9EE2E6098D2`.
Only DLL deployed; current INI/loader/assets preserved.
Rollback: `deployment/native-hands-next/rollback/20261007-production-runtime-integration`.
Saved log/build evidence: `deployment/native-hands-next/production-runtime-20261007`.

Next integration needs: per-draw identification of both Main images, authoritative
source capture and original suppression, native cursor draw identification, and
Agent 3's actual contextual prompt lifecycle hook. No further headset request yet.
