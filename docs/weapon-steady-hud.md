# Controller steady aim and contextual HUD — 2026-10-20

## Physical1938 result — scope reopened, NOT accepted

DLL0DF06FD1...5BB4AB ran through Steam PID4940. User confirms health hiding
with support and ammo hiding at lens, but reports FPS loss and a flat scope
image with no perceptible magnification; L3 does not slow time. R1 steadiness
itself was not explicitly accepted. Preserve the full private log at
`build/combined-followups/steady-hud-6815d62/physical-feedback-1938.log`.

Observed native scope callback CPU means include2.189–3.925ms, peaks up to
5.409ms in the targeted excerpt; these exclude GPU execution. This proves
extra capture work, NOT sole responsibility for every FPS loss or a measured
GPU bottleneck. WeaponSteady routing logged held0/1 with twoHand1/sniper1.

Source facts: own capture uses a fixed20-degree FOV,512x512, and the whole
texture maps to the lens aperture. This is not calibrated magnification;
its apparent enlargement depends on the displayed lens angular size. No
runtime projection receipt yet proves the requested20deg reaches the final
capture projection. Do not just reduce resolution or guess a new FOV.
PiP explicitly writes eligible Kasimov LT=0, bypassing native ADS. The native
description requires scope-up for [dash]/L3 slow time; ADS-dependent gating is
a supported hypothesis, NOT a proven slow-time execution-path diagnosis.

Next: audit capture projection producer/consumer and native scoped slow-time
gate, then isolate optical magnification/capture cost from controller filtering.
Falsifiers: actual rendered FOV differs from component FOV; or slow-time ignores
ADS state and fails earlier in input routing. Do not turn on fullscreen ADS
blindly: it can reintroduce fullscreen zoom/camera/weapon animation conflicts.

Guard193753 reports restored. Independent installed hashes confirmed
DLL7A1DE5854D1EC8635EE77200E3A39F0D4256C3A73ED6A69E99D3C26417B69495 /
INI60224CA08A8E9B01E299AD4B14A069A1E854F2B5E46ADFBB0B01A86973A74ADC;
no Singularity process remained. No new candidate installed in response.

## Evidence / hypothesis / falsifier

User accepted the native-aperture scope round broadly ("jest okej"), then
requested reduced controller tremor, health hidden during two-hand support,
ammo hidden at the optical lens, and R1 steady aim for two-hand Kasimov.
This is NOT quantitative 90-FPS acceptance. Its completed log is preserved at
`build/combined-followups/native-aperture-137143d/physical-feedback-1919.log`.
Game exit and restored accepted DLL7A1DE585...9495 / INI60224CA0...4ADC were
independently verified. No new Steam writes or launch in this implementation.

Observed source: DrawHealthPlate/DrawAmmoPlate have no two-hand/optical
exclusions. `g_twoHandActive` is the input latch's atomic result. The optical
near-eye policy is input-thread-owned: publish only its boolean for rendering,
clear before sync/focus/menu early returns, never read its mutable state on
the render thread. Health hiding also resets face-raise hysteresis.

Controller aim arrives directly from xrLocateSpace, with no tremor filter.
Hypothesis: small high-frequency pose changes can be attenuated without
altering HMD tracking. Falsifier: deterministic alternating noise is not
reduced, deliberate movement sticks, or optics/ballistics visibly diverge.
No claim that all shake originates in controller sensor noise.

## Implementation / boundaries

`[Input] WeaponSteady=0` is the default. The candidate enables it explicitly.
Controller-only adaptive position/quaternion filter: 18ms base time constant;
65ms while the named primary Use/grip action is held with two-hand Kasimov
(11389 vertices), right-handed mode, valid tracking and gameplay. No new XR
action polling, allocations, locks or engine offsets. The same filtered
controller pose feeds gun/arm transforms, ballistic direction, native FX and
scope capture. Gesture/reload/holster/UI processing precedes the filter;
knife and busy reload transactions bypass it. Raw motion-speed diagnostics
use a separate prior raw sample, not yesterday's filtered position.

Primary Use is the default right squeeze (Sense R1); consume its native
Use/reload output ONLY in this held two-hand sniper context. Other weapons,
one-hand use, left-handed mode and menus retain existing input routing.
Changing strong/mild mode, weapon/pawn, invalid tracking or >100ms gap reseeds
the filter. >4cm or >0.20rad changes snap immediately; speed-adaptive response
reduces lag on deliberate motion. No physical comfort/accuracy claim yet.

HMD/eye pose and compositor submission are NOT filtered. Existing engine POV
rotation-shake suppression remains unchanged; no experimental camera matrix
rewrites or translation-stabilization flags are enabled here.

## "Pad 11" investigation (not a global replacement)

Installed localization data `RvGame.int / RvUI / ARSniperRifleDesc` says:
"Press [dash] when scope is up to slow time." The input table binds `dash`
to left thumbstick click / XI_LTHUMB (Sense L3), not R1 steady aim. This is
evidence of a separate native slow-time instruction, NOT proof that the
specific observed Pad11 glyph came from that field. The completed session
contains no Pad11 producer receipt. Leave numeric pad aliases unchanged;
capture the actual localization section/key before replacing a native action.
Do not claim native slow-time under the custom PiP has been verified.

## Headset route after a fresh start

Load the sniper save. Raise the health wrist, then acquire two-hand support:
health must hide and return after release; moving into the optical eye box
must hide ammo, moving away must restore it. Compare gentle tremor with R1
released/held while supporting Kasimov, then make a deliberate large aim turn;
check target, scope, hit and VFX agree. Check Circle manual reload, one-hand
Use, pause/resume and return to menu. Stay in the headset until normal exit.
Report any lag/discomfort; do not smooth head motion to mask it.

## Offline delivery (L1_PASS / L2_PASS, not L3/L4)

Code `6815d6247f861e01b0360393f48b69580a25552b`, clean x86 PE32 `/O2`, no
PERFORMANCE_BUILD instrumentation. Candidate `build/combined-followups/steady-hud-6815d62`:

- DLL `0DF06FD1EF5486B6766CD1570CDA0F32AE1E8B94EE65962D9604446C825BB4AB`
- INI `E498F125D29C02DC2F9C3018309CB1FF9D05E5FFB53F58B69FC3DA2767BC2D92`
- plan `3EFA534B1ECEAF87BACE44FE1E4AC94A3BFDAE49068D49BED116E202E9B01FD9`

Ten optimized native FX/scope/steady fixtures + Python suites PASS; full
`test-centurion.ps1 -PreparedRoot build/next-weapon-reload/prepared` PASS,
including four-weapon refill/first-shot, original animation/math, grip/knife,
native own-process x86 ABI and both-eye shared-depth GPU replay/Reset.
42 private preparation checks and six fake-install lease scenarios PASS.
145-pin accepted-profile Install -WhatIf PASS, no writes. Build retains
pre-existing analysis/deprecation warnings; no compilation error.

Eight scoped INI keys (previous optical seven plus WeaponSteady) and DLL only;
assets/localization/native UI untouched. Fresh start required before installing.
Use fresh backup/guard, never reuse the prior session's lease or backup blindly.
