# Pause-menu deep RE — bounded experiments, 2026-10-12

**Result:** the asset extraction blocker is removed. Prefer investigating the
engine's existing `RenderTexture` property on `SPPause`, rather than another
D3D batch mirror. This is an offline structural proof, **not a working VR fix**.
Branch: `research/pause-menu-deep-re`, based on research `1ff7a86`.

## New facts

The test asset matches the installed Steam asset byte-for-byte:
`UI_SinglePlayer_SF.xxx` SHA-256
`C8CDB2E193B53781D7634EEE88F2DC90FD168B5DCC639098441A2B489386984F`.
Only that installed asset was read for the hash comparison; no installation
files were changed. Extracted assets and downloaded references remain outside
Git in `C:/Users/Trilex/vr-mods/pause-re-lab`.

1. **Compression:** the file is a normal UE3 v584/licensee126 package with a
   compression array at `0x61`, not a fully compressed file. Its one chunk maps
   physical `0x81` to logical `0x71`, contains six LZO blocks, and produces
   exactly 764,132 bytes. Removing the physically inserted chunk record yields
   a 764,245-byte logical package. Earlier `uedecompress` failures were a
   container-selection error, not evidence that the SWF was inaccessible.
2. **Movie:** export 2 `SPMenus` contains a valid `GFX` movie at logical
   `0xAF67`, length 231,953, stage **1280×720**, 30 fps, two root frames.
   Movie SHA `0B28EA7B9FB3E32C678BAF83F12B0080AA8AC71FC852852F07F18F67AE21AE2F`.
   Both `SPPause` and `SPMain` reference this **same movie definition**.
3. **Configuration:** `SPMain` explicitly references `RTT_UI1` and
   `RTM_AlphaComposite`. `SPPause` serializes `MovieInfo=SPMenus`,
   `PauseGameWhileActive=true`, and `TimingMode=TM_Real`, with **no RenderTexture
   override**. Absence is not proof that the inherited/live value is null.
   `RTT_UI2` is a local `TextureRenderTarget2D`, 1280×720, clamp addressing,
   `bScaleformTargetOnly=true`, `bNeedsTwoCopies=false`. LoadGame, Options,
   Review and Slots already serialize references to it.
4. **Layout:** AVM1 root code at movie `0x16679..0x16688` assigns
   `Stage.scaleMode = "noscale"`. Thus the authored RECT does not guarantee a
   1280×720 runtime layout. `CenterMenu` uses `Stage.safeRect`; `SetupPauseMenu`
   positions Objectives using `Stage.safeRect.width - objectives._width`.
   This is concrete evidence of resolution-dependent layout before D3D calls.
5. **Content:** root `SetupPauseMenu` (`0x15FB7`, 224-byte body) creates
   ReturnToGame, ReloadCheckpoint, Options and QuitToMainMenu, then attaches
   `PauseMenuObjectives` under `Content`. Objectives is therefore part of the
   original pause setup; it need not be a separate focused movie or a tab.
   Sprite 288's `GetObjectives` callback (`0x128BB..0x128C8`) assigns its
   argument to `Objectives.htmlText`; its onLoad calls
   `gfx.io.GameDelegate.call` with the `GetObjectives` identifier. This is an
   ActionScript callback, **not a validated UE3 UFunction**.

## Experiment ledger

| Approach | Smallest test | Result / stop condition |
|---|---|---|
| Engine-native RTT configuration | Decode name/import/export tables and tagged properties; insert two engine-authored tags into SPPause **in memory**; update serial offsets; parse again | PASS: +60 bytes, all 44 export ranges valid, no property parse failures, other movie properties and SPMenus unchanged. Stop before engine use until live fields/target ownership are checked. |
| Asset layout / viewport virtualization | Decode 359 AVM1 action blocks; inspect SetupPauseMenu, CenterMenu, Stage and Objectives callback | Confirmed noScale + safeRect placement. A global showAll edit would affect other screens sharing SPMenus; no asset modification emitted. Pause-specific layout change remains possible but requires lifecycle/input proof. |
| Native-backed overlay readback | Recover the actual Objectives callback and attachment names from SWF | Static readback candidate `Content.PauseMenuObjectives.Objectives.htmlText` is now evidence-derived; `_root` qualification and native this/UFunction remain unverified. Selection synchronization still missing; stop before native calls. |
| Gate-B state safety | Inspect `7c25e04:pause_canvas_capture.h` and integration; exercise D3D contract/alpha counterexamples | Concrete defects below. No repeat runtime mirror attempt; no hands-causation claim. |

Tools: `tools/pause-re/unpack_ui_chunks.cpp`, `inspect_package.py`,
`inspect_movie.py`, `inspect_actions.py`, and `offline_checks.py`.
The unpacker includes the repository's **GPL-2.0-or-later** LZO decoder and
remains an offline executable, never part of `d3d9.dll`. Python utilities are
original analysis code. No recovered ActionScript or assets are committed.

Reproduce from repository root (build outputs and extracted assets outside Git):

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 && cl /nologo /EHsc /W4 /O2 /MD /std:c++17 tools\pause-re\unpack_ui_chunks.cpp /Fe:C:\Users\Trilex\vr-mods\pause-re-lab\unpack_ui_chunks.exe /Fo:C:\Users\Trilex\vr-mods\pause-re-lab\unpack_ui_chunks.obj'
& C:\Users\Trilex\vr-mods\pause-re-lab\unpack_ui_chunks.exe <owned-asset.xxx> C:\Users\Trilex\vr-mods\pause-re-lab\UI_SinglePlayer_SF.u
python tools/pause-re/inspect_movie.py C:\Users\Trilex\vr-mods\pause-re-lab\UI_SinglePlayer_SF.u --extract-to C:\Users\Trilex\vr-mods\pause-re-lab\movies
python tools/pause-re/offline_checks.py C:\Users\Trilex\vr-mods\pause-re-lab\UI_SinglePlayer_SF.u C:\Users\Trilex\vr-mods\pause-re-lab\movies\movie-0000af67.swf
```

All four checks PASS: real package/movie invariants, property insertion and
reparse, AVM1 content/layout evidence, synthetic RT/alpha counterexamples.
Logical package SHA after extraction:
`90A5CA7EFFFCCE7BEA8A201E99B8313F4A276C270657E5B521DC9EDE8F4101C6`.
The normalized extraction is a research input, not a game-loadable patch;
loader flags, recompression and cook compatibility are not validated.

## Gate B: defects explaining an invisible replacement

`MirrorBatch()` changes RT0 twice and calls `g_state->Apply()` afterwards,
but never calls `SetRenderTarget` with the saved original target. RT0 and the
depth-stencil binding are outside the D3DSBT_ALL restoration contract. See
[Microsoft's state list](https://learn.microsoft.com/en-us/windows/win32/direct3d9/saving-all-device-states-with-a-stateblock).
Therefore successful Apply can leave native subsequent draws aimed at the pool.
This is a concrete state leak; it does not establish that the observed hands
stretching was caused by it.

Additional defects in that exact experiment:

- On first allocation, `BeginFrame` precedes `EnsurePool`; `EnsurePool` invokes
  `ReleasePool`, which resets the ledger including its frame key. First-frame
  accounting is lost.
- RT/viewport/clear/constant/Apply HRESULTs are ignored; band counters increment
  even on failed draws, so “settled” does not prove usable pixels.
- Band-presence is not semantic coverage; scale bands alone do not prove that
  every menu/cursor/stencil draw was included.
- Native suppression is decided from the previous ready flag before the new
  mirror is validated; no provider RGB/alpha success is required.
- With a transparent clear, ordinary SRCALPHA/INVSRCALPHA blending can store
  alpha squared unless separate-alpha handling is correct. Alpha=.25 yields
  .0625 stored alpha; another straight-alpha pass attenuates RGB again. This
  is a mathematical hazard, **not measured live blend state**.

Do not redeploy a repaired MirrorBatch as a menu fix. These findings justify
an isolated D3D device-state/pixel test if MAIN returns to capture later.

## Reference projects: inspected code, not copied offsets

- **Dishonored**, pinned `b15e4d3`: [hud_owner.cpp](https://github.com/VR-Stereo-Hub/Dishonored-VR/blob/b15e4d32a720bd4cfdb5ce4d600f3f8fba8d2cec/src/game/dishonored/hud_owner.cpp)
  `Display/Publish/Execute` carries owner metadata across native command
  publication/replay. A thread-local movie flag alone would lose ownership at
  a render-thread boundary. [hud_capture.cpp](https://github.com/VR-Stereo-Hub/Dishonored-VR/blob/b15e4d32a720bd4cfdb5ce4d600f3f8fba8d2cec/src/core/gfx/hud_capture.cpp)
  `begin/end` (~379–439) explicitly restores RT and depth and disarms on restore
  failure; sinks match multisample or provide compatible stencil. Source is
  zlib/libpng licensed; reviewed only. `ui_state`, `hud_route`, `mono_screen`
  and `stereo_state` were also read.
- **F.E.A.R.**, pinned `85b3af1`: [stereo_hook.cpp](https://github.com/DR-89/fear-vr/blob/85b3af161f7f33e4d71fa4881e99699fa7ce9ecb/src/gameclient_loader/stereo_hook.cpp)
  has native menu focus/navigation hooks; [openxr_host.cpp](https://github.com/DR-89/fear-vr/blob/85b3af161f7f33e4d71fa4881e99699fa7ce9ecb/src/host64/openxr_host.cpp)
  (~1451) submits a menu quad with BOTH eye visibility. This transports a
  valid image; it cannot reconstruct Singularity's broken source.
- **RE5**, pinned `0c621b5`: [d3d9_hooks.cpp](https://github.com/SpreadTheBirb/ResidentEvil5VR/blob/0c621b52a8b77e4ef6a272e837b65c8dae6a9192/RE5VR/src/hooks/d3d9_hooks.cpp)
  Present (~264) runs `StereoTest_ComposeTheatre` before its own menu. Its
  premise is an already-flat frame. That premise remains unproven for
  Singularity. No license appeared in API metadata; no code copied.
- **Starfield**, pinned `c8a2b20`: inspected `CreationEngineRendererModule.cpp`
  and `experimental/hooks/CreationEngineHook.cpp`; the requested
  `onScaleformSetViewPortInternal` / `GetFileURL` implementation was not found
  in those files. Stop this path lookup rather than invent an exact reference.
  License MIT; no source copied. A per-movie viewport remains a conceptual
  option, not a verified Singularity hook.

## Corrections to inherited research

Gate A at `7c25e04` supplies evidence that bands A+B share the **backbuffer**;
our earlier “only offscreen scene” account is incomplete. Translations alone
do not prove pixel bounds or semantic content. The recovered movie now proves
an authored single 16:9 stage and resolution-dependent noScale placement.
The earlier assertion that a whole-movie hook was the *only* route was too
strong: serialized engine RTT configuration is another concrete route.

Reviewed the supplied ownership/readback reports at `8ae0502`/`8b5643a` and
the rootcause/design at `1ff7a86`. `pause-mono-capture-blocker-20261012.md`
was absent at checked research/Gate-A revisions. The two named external
research reports were not located in the scoped documents/attachments; their
contents are not assumed. No GOG/native binary disassembly was performed.

## Ranked candidates and stop rules

1. **Native RenderTexture configuration for SPPause — preferred.** Direct
   structural evidence; original renderer owns batching/stencil/Objective
   callbacks. Small property-level prototype, but startup, input scaling,
   reference lifetime and engine consumption remain runtime gates.
2. **Pause-specific Stage/viewport initialization.** Strong safeRect/noScale
   evidence; needs a proven movie-local lifecycle and matching input. Shared
   SPMenus must not be globally patched for all screens.
3. **Native state overlay.** Objectives text path now has static evidence,
   but selection/focus/Options state and native read interface remain missing.
4. **D3D mirror repair / late frame theatre.** High regression risk; no new
   capture runtime experiment is recommended without the producer/source gates.

**MAIN next:** read the small proposal in `pause-native-rtt-proposal.md`.
Observe the live RenderTexture values and ownership before allowing any write.
Keep the accepted installation under MAIN's control; this branch made no
renderer, hand, localization, texture, deployment or installed-settings changes.
