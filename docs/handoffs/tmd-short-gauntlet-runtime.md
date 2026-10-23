# TMD short-gauntlet runtime repair — SIDE

Base: `332cc98ecb3fc16ab72aabc6f8b00248a8d05e33`.
Branch: `fix/tmd-short-gauntlet-runtime`. No Steam writes, game launch, merge,
geometry edit, dual-weapon feature or haptics integration.

## Proven defects and evidence boundaries

1. Both input producers treated native RB/Impulse as melee even with acquired
   TMD: the rising `g_impulseBtnIdx` edge and the held `XI_RSHOULDER` path.
   These set the cached animation owner consumed by **both** FgHidden and
   PrepareIndependentLeft. The current archived game log read on 2026-10-23
   (`view_matrix.log`, 1,904,096 bytes at inspection) has TMD acquired at line
   6535; TMD raised at 14731; false melee windows at 15618/15960 and later.
   This confirms the problematic producer executes; it does NOT establish
   that every previous `animated` rejection was false melee. In particular,
   the first rejection at 13203 occurs next to cinema/menu transition logs.
   The old aggregate boolean cannot separate its three owners retrospectively.
2. Both the early entry and final replacement required `minIndex==0`, rejecting
   the actual `min=2626, verts=4818, first=13206, tris=7704` draw (13202/13217).
   D3D9 MinVertexIndex is a range bound, not an index rebase. See the
   [Microsoft DIP contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-drawindexedprimitive).
3. Old success text ran on the first handled dispatch, before checking either
   draw status or restoration. It was not a valid two-eye receipt.

## Corrections

- `TmdShortGauntlet` remains default OFF. With ON, acquired TMD and no held
  knife, neither shared-input producer may create a melee window. Native
  actions still receive their original input. All direct knife producers,
  heal deadlines, cinematic and restraint ownership remain unchanged.
- No bypass of NativeAnimatedHandsNow in either visibility or palette gates.
  A bounded owner-mask diagnostic distinguishes arms/cinema/restraint, with
  heal/melee/knife context. Maximum seven owner combinations; no per-frame logs.
- Early admission accepts the known original whole-buffer and subrange draw
  declarations. Final SVTG selection checks decoded min/max against the full
  incoming range, with overflow-safe arithmetic and a 7444-vertex source limit.
  No baseVertex, VB, vertex declaration, material, UV or index rebasing changes.
- Added actual native-glove CPU readiness alongside existing cuff readiness.
  Missing cache/cuff/buffer or failed binding retains the audited **49-run,
  6527-face hardware fallback**, not the unfiltered long arm. Palette failure
  suppresses an admitted mixed section before submission.
- Exactly two successful 7110-face draws, successful IB restore and successful
  palette restore are required for the receipt and same-frame tail suppression.
  Failure/stale frame/Reset cannot authorize omission of the separate tail.
- Anatomical-left tracking, OffHand-dependent delta selection, original moving
  hardware matrices and shadow exclusions are unchanged.

## Geometry identity and draw arithmetic

Private asset: `research/tmd-short-gauntlet-private/tmd_short_gauntlet.svtg`.
SHA256: `452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192`.
21,330 indices / 7110 triangles; actual extrema **2743..7322**.
Native allowed range: **2626..7443**, because 2626 + 4818 = 7444.
Every replacement index is in that range and addresses the same original
vertex as before; the replacement IB starts at index 0, but its VALUES stay
absolute. Original cooked index round-trip, hardware order, short cuff and
23-face cap all reproduce the exact existing asset checksum.

## Verification

- L1 short-gauntlet C++: measured range, invalid/overflow ranges, input ownership
  truth table, genuine animated admission, two-eye dispatch, failure injection,
  original 49-run fallback, IB refcounts/restoration, palette-failure receipt,
  stale frame/Reset refusal: PASS.
- Private source test: full cooked/PSK identity, exact asset SHA, range,
  hardware weights, no long island, five wrist rotations: PASS.
- Standalone **D3D9 AND D3D9Ex**: real SVTG IB + synthetic 7444-vertex buffer;
  unchanged base=0, min=2626, vertices=4818; two successful 7110-face draws;
  original IB restored; render-target readback **3304 / 3304 nonblack pixels**.
  This proves API/buffer/pixel behavior, NOT original game materials or PSVR2.
  Sandbox could not create the D3D9 HAL device; the same test passed outside
  sandbox in its own hidden window, without loading the game.
- 12 existing CPU executables: TMD aim and native adapter, holster, cuffs,
  hands-only (both handedness modes/hardware animation classification), VR
  transition, Centurion reload, next-weapon reload, measured aim, WeaponLocker
  preview/policy/layout: PASS.
- TMD wiring (3 tests), WeaponLocker preview paths, compiled x86 factory
  `[eax+20Ch]`/FName stack regression: PASS.
- OLED CPU/lifetime and D3D11 pixel comparisons: PASS.
- NIS math: 178 PASS; embedded bytecode and isolated-eye pixel tests: PASS.
  **Known baseline failure:** old `test_nis_wiring.py` expects the removed text
  `if (nisStereoSbs && g_frameSrc11`. Same 1/3 failure reproduced on 332cc98 and
  this branch; no NIS code changed. Do not report this source-text test PASS.
- L2 x86 PE32 `/O2` build: PASS; exact DLL identity in build manifest/report.
- L3/L4: NOT RUN. No physical visual claim.

Reproduce: existing `tools/gloves/test-tmd-short-gauntlet.ps1` with private
`-Tmd`, `-Cooked`, `-Asset`; `tools/gloves/test-tmd-runtime-regressions.ps1 -Asset`
for CPU/GPU/OLED; existing `tools/fx/test-nis.ps1` (baseline textual failure
above); `tools/reload/test_locker_compiled_abi.py spikes/view_matrix/d3d9.dll`.

## Required MAIN runtime receipts

Keep original feature prerequisites, including IndependentHands, hands-only
GloveIsolate=2, GloveNativeEnabled and NativeGloveCuffClosure. No new INI key.

1. `CPU ready enabled=1 tris=7110` only proves loading.
2. `early exact mixed section admitted to independent palette` proves entry.
3. **`stereo receipt eyes=2 tris=7110 indices=2743..7322 nativeRange=2626+4818
   base=... indexRestore=OK paletteRestore=OK`** is the draw/restoration gate.
4. `native-animation owner=...` identifies genuine animation ownership; an
   Impulse without a held knife must no longer arm a new `melee window`.
   Any `first failed gate=...` after admission identifies the next refusal,
   not a reason to remove safety checks.

MAIN can then validate empty-left-hand wrist rotation, Impulse and Age,
knife/heal/cinema/restraint transitions in one controlled session. Confirm
short sleeve/hardware in both eyes and no long forearm; check normal Centurion
aim afterwards. No request to run that session is made by this SIDE task.

Recommendation: GO for MAIN review/controlled validation; NOT physical PASS.
