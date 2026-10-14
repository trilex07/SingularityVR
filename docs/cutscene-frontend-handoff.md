# PID16456 frontend/cinema handoff experiment

Physical: Polish helicopter voices now play, but too quiet; noTMD deformation
mostly resolved, controller alignment unaccepted (user seated on floor).
Button-related FPS difference not observed, residual dips remain. Returning
to the menu produces two flat images. Accepted installation restored/hash verified.

Evidence: normal main-menu submissions use class2 half=90/crop=0. After return
during this session class2 reports crop=89/half=0 while the cutscene composer
reports mode=CutsceneFlat, native cine=1, menuClass=2. The composer excludes only
MenuSubmitForcesFlat (boot class1), not frontend class2. UE3 can legitimately use
a cinematic camera for the frontend backdrop: raw cine alone does not identify
a playable Matinee. Applying full-screen cinematic composition to a frontend
that already owns per-eye UI captures is a concrete conflicting producer.

Hypothesis: giving the identified focused frontend its normal MenuFlat
presentation stops full-SBS cinema fitting/cropping and restores the existing
accepted frontend panel path. Raw UE3 cinematic state is NOT overwritten.
Falsifier: class2 does not own the failing frame, class2 crop remains despite
MenuFlat, or duplicate content persists after the cinematic consumer is excluded.

Small experiment: [Render] CutsceneMenuHandoffFix=0 default OFF. When ON only
focused native class2 overrides cinematic presentation, and confirmed Bink
video retains priority. Classes0/1/3/4, cinematic dialogues, gameplay, layout
insets, assets, weapon/hands/shadows, input and engine cinematic flags unchanged.
One shared scene controller governs camera/projection/composer/capture decisions.
Bounded transition logging records frontend predicate and raw cine separately.
L1 matrix tests and x86/O2 build before any new physical trial; no runtime PASS
inferred from the tests. No new Steam launch before fresh user start.

FPS profile3600frames/43687ms contains menu1080/cine309 (not pure gameplay),
idle/LT/RT3548/52/0. Sparse native Draw forwards19254samples/~0.333us/call;
whole-window estimate0.9142ms/frame, inclusive draw1.8747ms/frame. Different
sampling domains/rates: do not subtract them to claim exact hook-exclusive cost.
No discovery scan in this window, callback replay28/~3.41ms per replay. Residual
FPS cause still open; sampler intentionally supports one capture per process,
so no second capture was armed/reset in this session. Later gameplay near90FPS
and a ~4FPS VideoFlat interval are not an isolated gameplay A/B.

## Controlled Steam trial PID34980 (2026-10-13)

User explicitly authorized start. Four-file candidate installed after closed-game
preflight and verified backup: VR85CCCB60, injectorD6EFDDAE, INIFB9779DA,
native audio84B9A419. Loaded DLL paths and OpenXR ready confirmed. Independent
read-only observer and hidden restore guardian ran; no game-window automation
or mid-headset chat instruction. Evidence directory (private, not committed):
deployment/hands-dubbing/frontend-loud-live-20261013-223929.

L3 execution evidence: focused class2 with raw cine=1 is now MenuFlat; cinematic
composer reports gate-blocked. On final frontend return, class2 submit counters
are crop=0/half=90 (last89), versus the previous failing crop89/half0. Confirmed
video still takes VideoFlat priority, and CutsceneFlat returns to GameplayStereo
at frame12874. Exact noTMD35-slot palette gate executes. These are route/state
observations, not pixel or physical acceptance; duplicated-menu falsifier still
requires the user's result.

One bounded profile completed:3600frames/40313ms, cine0, menu165, input
idle/LT/RT/both=3600/0/0/0. It is predominantly gameplay, NOT a trigger A/B or
strictly menu-free profile. Sparse native draw forwarding16865samples/~0.238us;
estimate0.5737ms/frame, inclusive draw1.3266ms/frame. Two native discovery calls
~21.37ms each and16callback replays~2.55ms each. Do not sum inclusive families or
infer GPU time. Sampled gameplay perf lines near90FPS do not rule out short dips.
No second profile armed. Residual performance and hand alignment remain open.

Game exit observed; guardian restored all four originals. MAIN independently
verified VRD8928180/injectorAAA07005/INIB3FEED43/audio890C6172, frozenUI5B2EA469,
107DDS, texture config and original forwarder. No pending profile request.
Full session and post-exit injector logs retained. Physical feedback PENDING;
no acceptance or automatic next Steam launch inferred.
