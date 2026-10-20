# Calibrated optical scope and native L3 bridge

Status: implemented offline; runtime / physical acceptance NOT claimed.
Previous physical failure: scope looked unzoomed, FPS fell, L3 did nothing.
Accepted installation remains DLL7A1DE585...9495 / INI60224CA0...4ADC.

## Evidence before implementation

The complete private physical1938 log preserves native capture CPU callbacks:
typical means 2.189–3.925 ms, some windows peaking at 5.409 ms. Extra native
draws/uploads occur every optical render. These are CPU submission timings,
NOT GPU execution measurements and not proof of every frame-drop cause.

Offline archived executable audit proves SetCaptureParameters' 373-byte
rebuild computes tan(FOV/2)*aspect*0.75. A square capture therefore does not
have the nominal FOV. More importantly, fixed20deg cannot imply magnification:
the source must be narrower than the actual lens aperture's angular width.
The factory/constructor/render contracts copy component view A0->proxy50 and
projection E0->proxy90. Updating component FOV alone leaves the proxy stale.

Kasimov's 357-byte native tick calls its 512-byte energy update. That update
decrements native slow-time energy and checks the player's IsAiming, stopping
slow-time when false. PiP intentionally keeps native ADS false to avoid its
full-screen zoom, hand animation and postprocess. Native L3's original path
also requires aim. A separate 87-byte toggle delegates to native start/stop;
native start retains energy/cooldown/controller checks.

Reproduce against owned inputs with tools/fx/audit_scope_optics_slow.py.
It outputs identity/hash contracts, not game code/assets. Runtime resolves
registered wrapper / actual class vtables, then checks complete fingerprints.
No committed executable address is used. Reflected slow-time fields must match.

## Small controlled candidate

Both new flags default OFF:

- Render/ScopeOpticalCalibration=1: calculate 4x from the actual foreground
  aperture width, per-eye forced projection and native 0.75 normalization.
  Only the owned capture's projection changes, with component->proxy mailbox
  synchronization and a native matrix readback check (0.5% tolerance).
- Same flag bounds owned optical renders to at most45.5Hz, reusing the resolved
  texture between captures. Headset/main scene and lens placement remain at
  their normal refresh. This is an explicit optical temporal-quality tradeoff,
  not a guarantee of 90 FPS or a reduction of unrelated gameplay work.
- Input/SniperOpticalSlowTime=1: consume L3 only during fresh near-eye PiP;
  submit its rising edge to the original native toggle on the engine thread.
  An IsAiming detour substitutes true ONLY at the exact native energy-update
  return site, for the local pawn with fresh owned optical aim. Every other
  query delegates unchanged. Lowering the scope calls native stop; energy,
  exhaustion and regeneration remain native. R1 stays weapon steadiness.

No head smoothing, whole-frame ADS, global time-dilation write, replacement
scope scene, texture remaster, weapon/reload or accepted native VFX edits.

## Falsifiers / physical route

If native projection differs from the predicted matrix, refuse the capture;
do not guess another multiplier. If 4x geometry passes but perceptually there
is no zoom, compare actual source projection and aperture receipts, not another
constant FOV. If L3 submits but energy aim never matches, inspect the native
caller/context; do not force native ADS globally. FPS remains unaccepted until
measured on the same scene with scope lowered versus raised.

One uninterrupted PSVR2 route: load Kasimov, compare a fixed distant detail
with scope down/up, aim at a different target, hold R1 with two hands, click L3
with the scope up, then lower it. Check health/ammo hiding, Circle reload and
native muzzle FX. Stay in headset; close normally, then report magnification,
slow time and FPS. MAIN collects the bounded receipts without chat switching.

## Candidate receipt (L1_PASS / L2_PASS only)

Clean source 1e27b9424c1982018f3038d647e6d0d50c10f910, x86 PE32 /O2,
no PERFORMANCE_BUILD. Native FX/scope/steady fixtures, complete four-weapon
reload/first-shot/animation/grip/ABI/GPU regression,49 private prep checks and
six fake-install lease checks PASS. Private original executable audit PASS.
Existing unrelated static-analysis/deprecation warnings remain; build succeeds.

Candidate: build/combined-followups/calibrated-optics-1e27b94

- DLL: 380F15213259B036DAD8D9C4135C215F09661F912A11CD6D45A04B2078FA1EB9
- INI: 9CB5A44E8DE1462E7C97C277894186BF831E8B233643749F93AFAD66D724EBBE
- plan: 4954BD4B0E25A99B0F76422934440AFA868FE0E4DECEE2E256FE90CD3482C759

Accepted-profile Install -WhatIf PASS across145 pinned files and logical INI;
no Steam writes/launch. Ten scoped INI deltas retain the preceding optical/
rifle/steady candidate and add just the two flags above; DLL only, no assets.
READY_FOR_PHYSICAL_ACCEPTANCE; require fresh start and a fresh verified lease.
Installed DLL/INI independently still match7A1DE585...9495/60224CA0...4ADC.

## Physical2016 result (supersedes ready-only receipt)

Deployed380F1521...FA1EB9 with verified fresh backup20261020-201550 and guard;
Steam PID42312, OpenXR running/rendering confirmed. User: scope present but its
view does not match weapon aim; Valkyrie also inaccurate; rest OK. No measured
native-spread attribution, no claim of full scope acceptance or guaranteed FPS.
Private physical-feedback-2016.log records calibrated4x projection, changing
owned-proxy view, and native L3 toggle energy5.000/dilation0.100 plus native
energy aim gate. These prove execution, not camera/barrel/shot alignment.
Guard restored baseline; independently rehashed DLL7A1DE585...9495 and
INI60224CA0...4ADC. No new candidate deployed after feedback.
