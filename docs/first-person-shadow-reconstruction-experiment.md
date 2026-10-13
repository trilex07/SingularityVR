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
