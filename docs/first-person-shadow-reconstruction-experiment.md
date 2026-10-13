# First-person shadow receiver experiment (default OFF)

Preserve physically verified PID17212 world inverse + ClearGate + input cache.
User confirms textured/tracked gun, background-negative gone, enemy shadows
retained; only gun/glove shadow differs per eye.

Observed native near1 receiver291/292 uses exact audited VSd09ad82c/PS28e01133,
8 vertices/12 triangles, HDR scene+512R32F, per-half-eye scissors. PS c2 is
[.999000013,.000999987125,1.001001,.00100098806]. Both PS ScreenToWorld/Shadow
matrices are identical while actual VS eye matrices differ. Private shader
instructions prove reconstruction from clipXY/clipW and alpha-derived linearW;
only XYZ of ScreenToWorld is read (unused last W=10). Alpha blending preserves
destination depth. Gun base pass writes alpha; its depth/color now both moved1.

Falsifiable hypothesis: near1 shadow receiver still consumes SBS/eye-specific
clip coordinates with mono inverse, causing eye-dependent sampling on gun/glove.
At native arm-caster285 LocalToWorld origin, mathematical reconstruction errors
are61.053/97.475UU; native shadowUV(-.01382,.55416)/(.10989,-.22569). Actual-eye
inverse + preserved native world-to-light gives bothUV(.117733,.261451). This
is an algebraic witness, NOT a raster pixel; scene-alpha/W and moved caster
ownership remain limitations. Native producer/receiver chain has residual~.177
(unlike world); do NOT fit/replace native light projection based on that residual.

Smallest experiment: separate owned 20-minute default-OFF flag, require verified
world correction and ClearGate already ON, exact shader identities/resources,
non-controller-transformed volume and observed near1 depth family only. Derive
per-eye inverse and rebase c3..10 using the existing tested math; preserve native
world-to-light, all other constants, COM refs and eight-register restoration.
World near10 remains excluded from this new scope. Old rejected mixed receiver
stays OFF. No bone palette, gun transform, glove selection, texture or UI edits.

Disproof: final constants confirmed corrected but eyes still disagree, shadow
becomes detached, or gun/world/stereo regress. Disable only the new flag; keep
the working combination. Next then inspect depth alpha/receiver ownership rather
than guessing constants or suppressing normal shadows. Synthetic tests and x86
PE32 /O2 provenance required before MAIN Steam/physical test.

Implemented separate fpshadowfix scope and owned FirstPerson flag; existing
world shader cache/resources/math reused without changing world correction.
Requires WorldReceiver and ClearGate ON; fails closed without either. MAIN
rollback guard now removes this separate flag. x86 /O2 unit checks cover OFF
zero queries, prerequisite/depth/moved gates, captured near1 eye round trips,
balanced refs, scoped restoration and failed upload/restore fault latch. Existing
world scope regression, runtime policy and PowerShell parsers pass. No runtime
or visual pass for this new scope yet.

L2_PASS: source3488084d46d7a8ad5d8a2a7310a577e92630c461, x86 PE32 /O2
Performance SHA5D49C7A51EAAD4BC1F22F489D958DFE669A4AEE7B7C28936E053D34ED13F1A2F.
Manifest matches actual artifact, no tracked edits during build. Added tests
also cover nested world/near1 scopes and agreement of both-eye rebased light
coordinates on captured eye matrices. Ready for controlled MAIN runtime gate,
not physically accepted yet.

## Physical acceptance PID39308

MAIN Steam/OpenXR on verified DLL5D49C7A5/source3488084, previous world/ClearGate/
input cache plus new near1 scope ON. Pause-return1196draws/304groups/overflow0;
arm caster289 followed by near1 receivers295/296. Final GPU constants reconstruct
caster origin with5.45e-6/3.94e-6UU error and both UV~(.827322,.463601). Runtime
reports first-person application in both eyes and no restore failures.
User explicitly reports everything good, considers shadows finished, requests
main PR for approval/merge. PHYSICALLY_VERIFIED in this tested PSVR2 scene, not
every weapon/level/headset. After exit, guard removed all four owned flags;
MAIN verified actual DLL9FBA308C/INI C59B1CF6/UI5B2EA469 restored/retained.
Verified snapshot115652-403/runner backup115652. Raw assets/captures stay private.

Production promotion keeps the same math/scopes/cache and adds persistent INI
defaults; its independent L2 build is recorded in accepted-fixes note. No claim
that the new default-configuration artifact itself ran in Steam before PR.
