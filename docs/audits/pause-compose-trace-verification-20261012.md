# PauseComposeTrace verification for MAIN

**Branch:** `research/menu-intro-rootcause` after `dad1d99`.
**Renderer:** not modified. **Steam:** not touched.
**Runtime log:** no `pause-compose` lines exist in the current `view_matrix.log`.

**Verdict: NO-GO.** Gate B must not start.

## Checklist

MAIN may treat a class-3 capture as complete only when every row is true for the same token.

1. `pause-compose txn=<id> ev=begin` and a matching `ev=end reason=complete` exist. A missing end, `reason=truncated`, or any other end reason fails.
2. `token` is the `g_vrMenuStateToken` published by `NativeVrUi::UpdateMenu` (`native_ui_input.h` lines 49–51). Every draw in the transaction carries that same token.
3. `panel=0` on begin. A trace taken while VR Settings is open is contaminated. HEAD already skips redirect in that state; do not use it as the movie boundary.
4. Three passing transactions, `state=root`, `state=selection`, and `state=objectives`. One pause frame is not enough.
5. Each of those transactions contains `group=main` and `group=objectives` draws. `group=unknown` fails. `group=world` or `group=overlay` inside the transaction fails.
6. Those menu draws name one `surf`. A second surface is an inconsistent render-target transition.
7. Main and Objectives viewports overlap the same full target. Viewports that sit in opposite horizontal halves are baked SBS and must not be redirected.
8. The trace records viewport, scissor enable, and scissor rect on each draw. The offline checker uses the viewport to detect halves; MAIN still needs the scissor fields in the log so a later pass can reject a scissor-only split.
9. No per-draw readback. Completeness is this ledger, not a pixel sample.

`spikes/view_matrix/pause_compose_trace.h` implements rows 1–7. `tests/test-pause-compose-trace.cpp` rejects a missing end, a missing Objectives group, an unknown group, a foreign token, two surfaces, a world draw, eye-half viewports, an open VR Settings panel, and an empty log.

## Log lines MAIN should emit

One line, no per-draw allocation. Stop with `reason=truncated` when the fixed buffer fills.

```
pause-compose txn=<id> ev=begin token=<u64> owner=<3> panel=<0|1> state=<root|selection|objectives> frame=<n>
pause-compose txn=<id> ev=rt token=<u64> epoch=<n> surf=<u64> w=<n> h=<n> bb=<0|1> scene=<0|1> vp=<x,y,w,h> scissor=<on,l,t,r,b>
pause-compose txn=<id> ev=draw token=<u64> epoch=<n> surf=<u64> group=<main|objectives|selection|cursor|panel|world|overlay|unknown> api=<dip|dipup|dp|dpup> vp=<x,y,w,h> rtw=<n> caller=<hex>
pause-compose txn=<id> ev=end token=<u64> reason=<complete|truncated|stale|fallback>
```

`group` has to be assigned by the state delta in `dad1d99` (what appears or moves between root, selection, and Objectives), not by stride or `c5` scale. The blocker log's two backbuffer scales (`sx=7.812e-05` and `sx=1.249e-05`) are not yet those groups.

## What the blocker log does not prove

`bc83af6` counted class-3 `HudStereoPair` batches: 5878 offscreen `sx=1`, 4594 backbuffer `sx=7.812e-05`, 1284 backbuffer `sx=1.249e-05`, and one submit of `5264x2684` with `menuFlat=0`. That shows where pixels landed. It does not open or close a GFx transaction, name an owner, or separate world draws from Objectives. Those counts stay NO-GO.

## Hazards in the current hooks

These are in this research tree, which is the code the design cites.

- **Token vs render thread.** `UpdateMenu` writes `g_vrMenuStateToken` on the input thread (`native_ui_input.h` lines 36–63). A render transaction must copy the token at begin and fail if it changes before end. Otherwise two movies share one capture.
- **Capture on bind is too early and too small.** `Hook_SetRenderTarget` calls `CaptureVrMenuTarget` when the new RT0 is a colour target at most 2048 wide (`d3d9.cpp` lines 11606–11612). That is before the movie draws, and a 5264 pause target never qualifies. Do not use this call as the transaction end.
- **Surface pointers are not owned.** `g_rtSeen` stores the raw `IDirect3DSurface9*` (`d3d9.cpp` lines 11580–11584). A trace may log the address as an id. It must not AddRef-cache it across frames without a release on reset.
- **Draw trace is a sample, not a transaction.** `MenuDrawTraceRecord` returns before any device query unless the token is 20–27 frames old (`menu_draw_trace.h` line 58). Inside that window it AddRefs the current targets and releases them before the hash insert (lines 63–77). A PauseComposeTrace that returns between those Gets and Releases will leak. Overflow increments `g_menuTraceDropped` and emits nothing (line 89), so a full buffer looks like a short movie.
- **State block lifetime.** `DrawVrMenuPointer` creates one `D3DSBT_ALL` block and reuses it (`vr_menu_overlay.h` lines 374–376). `ReleaseVrMenuResources` does not release that block (lines 346–354). A whole-transaction redirect needs its own block, captured around the single GFx call, and released on device reset. Restoring the shared wrist block would put back whatever overlay last captured.
- **Frame order.** Provider draw runs at Present (`DrawVrMenuPointer`). Suppression via `g_vrMenuSources` only matches a texture the scene samples. It does nothing for batches that were drawn straight into the backbuffer. Suppression stays off until `ev=end reason=complete` for the current token.

## If a later log is GO

Minimum seams, and only those:

1. Begin/end around the single caller Gate A names. Not `HudStereoPair`, and not `PauseMenuMode3Bake`.
2. Bind a new RT0 with the original width, height, and format for that one call. No c5, viewport, or scissor rewrite.
3. Register that texture with the current token only after `reason=complete`.
4. Present with the existing full-UV head-plane path. Suppress native sampling of that texture only. Leave classes 1, 2, and 4 on the native path.

Until the three-state log passes `EvaluateLog`, the recommendation remains: keep LEGACY, do not redirect.
