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
