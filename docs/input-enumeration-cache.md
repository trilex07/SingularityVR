# Event-invalidated native controller discovery cache

Observed Steam/PSVR2 evidence: native discovery averages75–76ms; throttling
500ms then2000ms improved held-button drops to75–80 then85FPS, retaining tested
actions/stereo/tracking. This proves discovery frequency matters, not that a
timer throttle is a complete fix. Private logs and earlier timing caveats are
in shadow-clear-gate-experiment.md.

## Native callback audit

Owned complete live .text snapshot PID33004, no failed pages. Callback011D2180
compares instance GUIDs against the native controller list, calls the existing
product classifier, may create/configure/release a DirectInput device, and
stores instance GUID/device state. It is not an input-state poll. Native callers
request discovery for a missing GUID slot. Skipping all callback side effects
forever would be unjustified. COFF listing addresses are .text offsets: add
00402000 to get VAs; do not mistake listing00DD0180 for the runtime address.
Full0x313-byte callback fingerprint FNV1a8735998E; helper's earlier29-byte
signature independently verified. No game-derived source/bytecode is published.

## Hypothesis and smallest implementation

Windows discovery is redundant for an unchanged device topology. Cache at most
64 full DIDEVICEINSTANCEW records from an actual successful original scan.
On a hit replay the original callback, with original instance data and null user,
on the native input owner thread. Thus native device setup/recovery paths remain
executed, button reads remain unchanged, and no COM work moves to another thread.
The cache does not hold COM pointers or AddRef/Release devices.

Exact default-OFF owned v3 flag,20min lease. Failed scan HRESULT, invalid record
size, overflow, early callback stop or notification during scan cannot produce
a cache hit. Context transitions and other native callback activity invalidate.
DirectInput interface identity changes force a new scan. CM device-interface
notifications invalidate via atomic epoch only; registration failure means
native passthrough. Signature mismatch means no cache hook. Reentrant calls
abort capture and pass through; no lock held across native callbacks.

Device read/acquire errors are still handled by native game code; replay does
not intercept or invent their HRESULTs. Their wider recovery behavior must be
physically tested; do not claim general device-error or hotplug acceptance from
offline tests. A successful EnumDevices HRESULT can coexist with an internal
callback CreateDevice failure; callbacks remain replayed specifically to keep
those native attempts rather than caching callback side effects as successful.

Falsifier: verified snapshot hits/callback replay with continued held-input FPS
collapse; action/menu/device reconnect regression; capture rejected continually.
Runtime counts distinguish real scans, replayed requests, captured/replayed
callbacks and rejected snapshots. One completed counter is not acceptance.

## Verification gate

Synthetic x86/O2 checks cover success/failure, empty-list success, copied GUID
content/replay, exact interface identity, topology invalidation, in-flight
invalidation, capacity/record-size failure, reentrant fallback, unsupported
signature and unavailable notification passthrough. Runtime/PSVR2 pending.
Test same save/HQDecalsON: L2/R2/other buttons, pause/options/resume, controller
off/on if user permits. Compare exact scans/callbacks with FPS; guard restores
accepted DLL after exit and preserves user INI/UI.

## Steam/PSVR2 result PID19520, 2026-10-13

ExactDLLF294D272 source130dfdb, hookcreate/enable0/0 and OpenXRready. Actual
snapshot content:2 captured callbacks; subsequent requests replay2 callbacks
each, zero rejected snapshots in shown gameplay intervals. Both-trigger endpoint
LT255/RT255 interval11.131ms/frame, then90FPS gameplay with zero full scans.
Context transition executes another real scan, as designed.
User confirms all tested controls, stereo, tracking and pause/options/resume
remain intact and FPS no longer drop. This is physical acceptance for the tested
symptom/lifecycle, NOT controller reconnect or arbitrary device-error acceptance.
Bounded profiler stopped before final3600 report; last3360 partial:2 scans,
all timed, mean75942.450us. Do not report the profile as complete.
Private final log runtime-19520-input-cache-final.log. After exit guard restored
DLL9FBA308C; actual UI5B2EA469 and currentINI61AEC56C verified. User changed
LaserSight back1 during session; retained rather than restoring pre-testINI568E.
