# Pause GFx head overlay — frozen

**Status: `BLOCKED` for visual acceptance.** Branch `fix/pause-gfx-head-overlay` is an experiment, frozen at `1c370f8`. Do not deploy it, do not merge it, and do not run another Steam or headset test of it.

Further local fixes to the DIPUP subset will not prove that the subset is the native pause list. No production change until a later approach can capture the **complete** native GFx menu, preferably on the composition path that already works for the main menu. Do not add broader draw-hook heuristics, and do not keep iterating the synthetic 1280×720 pool.

## Confirmed GFx paths

Physical pause retest, 2026-10-11 (`deployment/physical-20261011-233500-pause-retest/view_matrix.log` on the machine that ran it; not a headset pass of this branch).

| Menu | Where GFx is composed | What the trace shows |
|---|---|---|
| Main (`class=2`) | Engine-private colour target `1280×720` | `VR menu sink` registers that target. Head-relative provider quad and diegetic suppress use that texture identity. |
| Pause (`class=3`) | No engine `1280×720` row while pause is open | Flash batches land on the scene HDR colour target, `5264×2684`, viewport `0,0,5264,2684`. `menu mono: class=3` is a one-time log, not a draw count. |

Main-menu capture works because GFx is already a private render target. Pause GFx is drawn into the scene target. Capturing that target would capture the world, which is why scene-sized pause capture stays rejected.

## MDG signatures that were identified

These are process-local shader and declaration pointers from that one session. They identify batches. They do not identify the whole menu.

| MDG | Hook | Stride | Blend / depth | PS / VS / decl | Target while pause is open | What it is |
|---|---|---|---|---|---|---|
| 74 | `DrawIndexedPrimUP` (`kind=3`) | 12 | blend on, `SRCALPHA` / `INVSRCALPHA`, Z off | `34275420` / `4DAC5B00` / `2F515260` | main menu only: private `1280×720` | Wide flash batch, ~54 prims. Same shader identity as pause MDG 137. |
| 140 | `DrawIndexedPrimUP` | 8 | same | `34275F60` / `34C76840` / `2F514A00` | main menu only: private `1280×720` | Narrow flash batch. |
| 137 | `DrawIndexedPrimUP` | 12 | same | `34275420` / `4DAC5B00` / `2F515260` | scene `5264×2684` | Pause flash fragment. 42 prims, 32 verts. `c5` translation moves between samples (`c13` 0.555 then 0.3083) while the scale stays ~`1.335e-5`. |
| 263 | `DrawIndexedPrimUP` | 8 | same | `34275F60` / `34C76840` / `2F514A00` | scene `5264×2684` | Different pause flash fragment. 10 prims. Scale ~`6.152e-6`, clip translation not the same as MDG 137. |

`c5` for these batches is a row matrix: `clipX = vx*c[0] + c[12]`, `clipY = vy*c[5] + c[13]`, with `c[15] = 1`. The first 8 bytes of the vertex are an int16 pair, not clip-space floats. MDG `xyFirst` prints that bit pattern with `%.4g`. For MDG 137, `y = -1755` and the two logged x values fall in int16 ranges `[18062, 20081]` and `[-19201, -17182]`. Those two points sit on one horizontal row of the `5264×2684` target (about y=541 and a span of about 1310 px), inside the left half of the stereo buffer.

The same pause frames contain many `kind=1` indexed draws (stride 12 and others) on the scene target. They are not in this filter. Large stride-32 scene batches are not in it either.

## What `1c370f8` actually does

`PauseHeadMenu` defaults to 0. When it is 1, only a DIPUP batch that passes every gate is redirected:

1. Stride 8 or 12, GFx blend, primitive cap.
2. `c5` has the flash diagonal shape.
3. `|c12|` and `|c13|` are within 0.88.
4. Pause redirect context is active (class 3, paused, scene target, not the backbuffer).
5. Only then may a PS+VS+decl triplet be learned, per stride.

The pool remap is a uniform contain of the source viewport into `1280×720`. For `5264×2684` that letterboxes vertically (top of the scene maps to about y=33.7 on the pool, not y=0). It is not an independent scale by `(1280/5264, 720/2684)`. A failed pooled draw or a failed D3D restore returns that HRESULT and does not draw the batch a second time on the scene.

L1 covers the contain math, the learn-order predicate, and the HRESULT policy. L2 for that commit is x86 PE32 `/O2`, SHA-256 `7A57B89695246F132282052FFC7B5D69DEC85A03538CFC846F3F095D1F8B0EFF`. That artifact is not for Steam.

## Why this cannot be accepted

The filter was built from the flash DIPUP rows above. Those rows are fragments: different primitive counts, different `c5` scales, and different places on the scene target. No measurement shows they are the rest of the pause list (rows, highlight, background, cursor). A pool that only receives this subset cannot become the native pause menu by tightening the same signature.

Headset result already recorded for the earlier build of this idea: with `PauseHeadMenu=1` the pause list stayed on the diegetic monitor. That run was rolled back. It is not a pass, and this later commit was not retested.

## Remaining fail-closed hole

`PauseHeadExecutePooledDraw` still draws into the pool when `c5`–`c8` cannot be read or do not match `FlashMatrixShape`. `haveOrigC` is then false, the remap is skipped, and the original draw runs on the pool with whatever constants are already in the device. That is fail-open.

The path that **is** fail-closed: if the remapped constants were computed and `SetVertexShaderConstantF` fails, the function restores and does not draw. A future implementation must treat “could not read” and “shape rejected” the same way: do not issue the pooled draw on untransformed constants. This note is the record of that hole. This branch will not grow another heuristic to paper over it.

## Required next approach

Capture the complete native GFx pause menu at composition time, the way class 2 already composes into a private `1280×720` target. Do not start from another draw-call filter, and do not extend this pool. Production stays on the current shipped menu path until that composition capture is demonstrated.
