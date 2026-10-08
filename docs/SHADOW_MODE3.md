# ShadowMode=3 — protect the light-space matrix (VR shadow fix)

Branch `feature/shadow-mode3` (worktree `SingularityVR-shadow3`), based on the
shadow-policy work `a775c9b`. This adds a concrete, testable implementation of a
new hub option, **`ShadowMode=3`**.

## The bug (matches the reported symptoms)

Production `d3d9.cpp::StereoPairBody` does **not** translate geometry. It scans
the constant registers, collects every one that currently holds a camera matrix
(`g_camMats[]`), and re-uploads a **per-eye** version of each (`ApplyEyeRemap`:
clip-space half-width + eye offset). That is correct for the camera
view/projection.

It is catastrophic for a **light-space (shadow) projection matrix** that lands in
the same register set — the production comment already names the reuse
(“the same register is reused by shadow and post passes with different
[meaning]”). A per-eye remap of the light matrix shifts/squashes the shadow
lookup every eye, which is exactly:

* **blocky / square** — the light matrix is remapped like a camera, so shadow
  UVs land on wrong texels;
* **flicker** — the slot’s validity toggles within a frame, so the draw flips
  between remapped and native;
* **one eye only / darker-lighter** — the eye that got the stale/native matrix
  samples a different value than the remapped eye;
* **corrupt map** — the light-space *generation* pass being split into SBS
  halves leaves two squashed copies.

**Key invariant:** the shadow lookup is `f(worldPos, LightMatrix)`. `worldPos` is
identical for both eyes (stereo remaps the camera clip matrices, not geometry).
So with a **native** `LightMatrix`, both eyes sample the *same* shadow
coordinate. The fix is therefore not new shadow rendering — it is to stop
remapping the light-space register and stop splitting the shadow pass.

## `ShadowMode=3` semantics

`shadow_mode3.h` implements three rules:

1. **Shadow-map generation** (shader declares shadow, or an offscreen depth-only
   target): draw **once, native**, never split/remapped (`NativeShadowOnce`).
2. **Light-space register**: excluded from the per-eye camera remap
   (`ShouldRemapRegister(camera, shadow) = camera && !shadow`).
3. **Unknown** evidence: one unsplit draw (`MonoFallback`) — the existing
   anti-flicker fallback, never a split.

Plus a live **census** (`ShadowRegisterCensus`) of which constant slots hold a
light-space matrix, and an optional, tested **world-offset compensation**
(`CompensateWorldOffset`, `L' = L · T(-e)`) for the case a future path ever
offsets geometry in world space instead of remapping cameras. This mod does the
latter, so the compensation is belt-and-suspenders; it is proven by test.

## Integration (main agent) — small, explicit edits in `d3d9.cpp`

1. `#include "shadow_mode3.h"` (next to the existing `shadow_policy.h`).
2. Add `singularity::shadow::ShadowRegisterCensus g_shadowRegs;` and
   `singularity::shadow::DrawEvidence` fill-in at the draw seam.
3. **Mark the light-space slots.** Where the bound shader layout is shadow
   (`g_boundShaderLayout.shadow`, ~L4144) or a projected-shadow texture is bound,
   mark the projection-matrix slot(s) into `g_shadowRegs` from the *same* scan
   that fills `g_camMats`. Clear/refresh it when the layout changes.
4. **Skip them in `StereoPairBody`.** In the slot collection (~L10992), replace
   `if (g_camMats[s].valid) slots[nSlots++] = s;` with a guard that also requires
   `!g_shadowRegs.IsShadow(s)` (or `SlotMayRemap(...)`).
5. **Shadow-map generation draws** (layoutShadow / offscreen depth-only): draw
   once native via `DecideDraw(...) == NativeShadowOnce` — no split.
6. Gate it all behind `g_shadowMode == 3`; add the hub row
   `SHADOW 3 protect light-space` and extend the value log. Modes 0/1/2
   unchanged. Default remains whatever the hub sets.

## Register census (do not guess the slot)

`ShadowMode=3` is only *fully* correct once `g_shadowRegs` is filled from real
evidence. Until then, shadow-ambiguous draws take `MonoFallback` (safe: no split,
no corruption). Add a bounded census that logs, per draw: shader shadow flag,
bound projected-shadow texture, and every `g_camMats` slot. Set `g_shadowRegs`
from that evidence — **never invent the register**. This mirrors the rule from
the shadow-policy branch.

## Grounding in the reference repos

* **KHARVOX** `src/sfs/ShadowProjection.h`: unshifted, depth-writing, no-color
  shadow draws are distinct from shifted camera depth — classify, don’t shift.
  `NativeSharedShadowPolicy.h`: left/right may share only if they describe the
  *same world-space shadows*; the paired float4 is atlas scale/bias.
* **Dishonored-VR** (`stereo_state_policy.h`, `stereo_occlusion.cpp`, docs): a
  render-target/pass identity is stronger than a matrix-register hit; a shadow
  map must never be split into eye halves; the ghost-sibling rule keys an FP
  shadow caster to its this-frame color draw.
* **vrframework** `Fh5Adapter.cpp`: only the gameplay camera receives the per-eye
  transform; shadow/reflection/UI passes pass through untouched.

## Files

| file | contents |
| --- | --- |
| `spikes/view_matrix/shadow_mode3.h` | policy, register mask, compensation math |
| `tools/reference-tests/shadow/shadow_mode3_test.cpp` | decision, register, compensation invariants |
| `tools/reference-tests/shadow/run-mode3-tests.ps1` | x86 `/O2 /W4 /WX` build + run |

Test: `PASS: ShadowMode=3 policy + light-space compensation invariants`.

## Done criteria (headset)

Shadows present in **both** eyes, same intensity, no flicker, no squares, and
stereo geometry unchanged on receiver draws.
