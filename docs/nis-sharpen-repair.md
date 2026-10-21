# NIS sharpening — SIDE candidate, 2026-10-21

Branch: `review/tmd-vfx-side-safety`. Independent of default-OFF TMD aim and
effects-remaster changes on this branch. No Steam access or physical claim.

## Observation, mechanism, falsifier

User reports NIS ineffective and requests a live VR Settings control.
The source inherited at `55c8aa3` gates NIS on `cropEyes`, now defined as
`dup && !stereo && menuFlat`. This excludes gameplay SBS and includes flat
menus, contrary to the older NIS integration's intent. The 2026-10-09 smoke
receipt is historical, not evidence for today's menu/gameplay classification.

Two additional defects found by reading the actual host/shader contract:

- `NisDispatchEye` chooses scaler constants for differing dimensions, but
  still selects the sharpen shader and 32x32 dispatch. These must instead
  agree on scaler + 32x24. A same-resolution sharpen menu option should not
  resize to the recommended HMD resolution at all.
- NVIDIA's filter support samples outside the input viewport. A clamp sampler
  clamps to the texture boundary, **not** the SBS half boundary. The previously
  claimed half-viewport seam isolation was not guaranteed by those bounds.

Hypothesis: gate on captured gameplay SBS identity, isolate each input eye,
and align shader/config/dispatch selection. The falsifier is unchanged pixels
at different strengths, opposite-eye contamination, or no native gameplay
dispatch despite true SBS receipts. In-game visual quality remains MAIN L3/L4.

## Candidate

- Off remains the shipped default. `NISSharpen` uses the actual captured eye
  dimensions, with no resolution reduction/resampling. `NIS` retains the
  existing explicit INI upscale mode and recommended output sizing.
- NIS runs only on simultaneous SBS gameplay (`dup`, not alternate-eye,
  not flat menu/cinema). Both eyes use separate persistent input SRVs, native
  NVIDIA embedded shaders and persistent BGRA UAV outputs. NVIDIA source,
  bytecode, coefficients and licensing are unchanged.
- Submission uses one processed image per eye, full image rects and the
  captured per-eye poses/FOVs. Eye swap is applied once at final copy.
- Dispatch/config failure is resolved before choosing the XR swapchain size;
  fallback submits the original layout/dimensions and disables NIS for the
  session. GPU allocation failure is also a logged native fallback.
- Timestamp queries are not overwritten while pending; non-flushing polls
  preserve non-blocking GPU timing. Strength crosses UI/render threads as an
  atomic percentage. No per-frame resource allocation at fixed resolution.

## VR Settings / MAIN handoff

Both minimal and full settings have **NIS SHARPENING / WYOSTRZANIE NIS**.
Inside: ON/OFF and strength 0–100%, 10% steps. Start at 20–40%; 100% can halo.
Use the existing laser/R2 or stick navigation. Strength R2 clicks increase;
stick left/right decreases/increases. Closing saves only `Upscaling/Mode`
and `Upscaling/Sharpness`, respecting existing session-only mode. The menu
chooses Off or NISSharpen, not a hidden render-scale change.

Existing INI remains supported:

```ini
[Upscaling]
Mode=NISSharpen
Sharpness=0.30
RenderScale=1.00
OutputScale=1.00
```

No installed INI edited. TMD need not be enabled to test this feature.
Menus/cinema intentionally bypass NIS, so compare gameplay after closing the
panel. The desktop/Steam screenshot source is not changed to the NIS output;
evaluate the headset, not a raw-source screenshot.

## Autonomous evidence

`tools/fx/test-nis.ps1`: x86 `/O2` CPU math/gates, embedded bytecode validation,
own-process D3D11 WARP pixel readback and menu/submission wiring checks.
WARP is a synthetic L2 graphics fixture, **not** the game, OpenXR or PSVR2.

Pixel fixture uses non-group-aligned 66x35 eyes, distinct RGB colours for
left/right and checks every output pixel, including seam/borders, in scaler,
sharpen and requested-sharpen/resized-output fallback. It checks persistent
resources and measures a real strength difference on a soft 2D pattern
(2070 changed pixels at 0% vs 100% in the reference run). A hard square-wave
fixture originally produced no change because the SDK suppresses ringing at
that edge; it was replaced with a soft pattern, not treated as proof of a
failed shader. CPU gates reject mono/menu/cinema/alternate/odd/empty inputs.

L2 production artifact and exact SHA come from `d3d9.build.json` beside DLL.
MAIN must integrate with its latest baseline rather than blindly replace
accepted weapons/localization/config with this older-base SIDE checkout.

## One physical test, no chat while in headset

MAIN deploys a reviewed candidate with Mode=Off, preserving accepted INI and
assets, and verifies exact candidate hash before a real Steam/OpenXR session.
In one gameplay scene compare Off, On 30%, On 70%, Off by looking at a fixed
sign/diagonal edge; check both eyes for seam colour/geometry and FPS, then
pause/resume, view a cinema/menu and return to gameplay, finally exit normally.
MAIN checks bounded `nis: stereo submit` and measured GPU receipts; unchanged
pixels, one-eye artefacts or persistent GPU errors fail acceptance.
