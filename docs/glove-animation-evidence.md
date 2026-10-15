# Glove renderer — SIDE L1/L2 evidence (2026-10-15)

**Superseded candidate:** the receipts below describe the first implementation.
User later reported working gloves but rejected art/size. Current native-cut,
original-albedo and shared-size candidate: [glove-native-match.md](glove-native-match.md).
It needs SVRG v2 mesh caches; the v1 files below are not the new candidate.
Full native UE3 material/lighting equality is still OPEN.

Branch: `feature/glove-animation`, based on `origin/main` `72ad59d`.
Runtime source commit: `94aab90112e392d4a47ab142ed3aa38506461c8c`.
The evidence/status follow-up changes documentation only.

L1_PASS: five standalone x86 `/O2` CPU executables, including 261 new
math/cache/input assertions and fake-device guard tests; seven Python
geometry/format tests including both private models, all PASS.
Final real-cache sweep: 0 allocations, 3600 frames × 2 eyes per hand;
0.0367 ms left / 0.0392 ms right per two-eye CPU transform.
Standalone eager CPU pair+texture load: 2.369 ms.
These measurements exclude real GPU calls and do not establish no-hitch L3.

L2_PASS: production build finished from a **clean** source tree at the commit
above, `/O2`, x86 PE32 (machine `0x014C`, optional header `0x010B`).
`d3d9.dll` size: 2,886,656 bytes.
SHA-256: `27D5DA96003923D55B66F80E2601AE65D2211C4B92BD1132BC863839A82568A4`.
Build provenance is generated in `spikes/view_matrix/d3d9.build.json`;
local logs are `build/gloves/final-build-output.log` and `final-tests.log`.
The new headers produced no static-analysis warnings; existing project
warnings remain and were not broadened into this task.

No Steam installation was read, launched, deployed or restored. No HMD/GPU
render validation was performed. L3/L4 remain MAIN-owned and pending.

## Local candidate, not an installed build

`build/gloves/candidate/` contains the verified DLL, build manifest, CPU test
log, private asset report and exactly three runtime caches at
`vr-assets/hud/katorga/glove-animation/`. `hashes.json` records each file.
This is a local, private candidate; game-derived models/textures are not in Git.

- `glove_left.svrg`, 72,928 bytes:
  `58A267AD3C59CA42FC1FB8D7DB43BDF694FD0CFB4BBF91D433B2D230B9ED336B`
- `glove_right.svrg`, 73,672 bytes:
  `5683181F87EEB2E172D3F295B3B9E665588B327BBBA4B66FDABA8333059BFCF5`
- `glove.svrt`, 4,194,320 bytes (1024² BGRA8):
  `0F6A90BBEBEDE8CB80672F13F468DEFB0C3A1C5E758AA1459CCCBCD5A1DE442C`

Reference GLBs and CPU-rendered before/after are in `build/gloves/prepared/`:

- `glove_left.glb`:
  `EDCE3E3FE6D6D2F921C0C4BF6A5332808B4001FAC17EF8DEAA5C1236EC11C368`
- `glove_right.glb`:
  `FE5F54D0DA8EA10F9FE43EAEEDA3AB05CF70A7C7730448068C0F076D15827EE8`
- `geometry-preview.png`, `axial-rays.json`, `report.json` (all local).

## MAIN handoff boundaries

The candidate is based on the requested main commit, **not MAIN's possibly
newer/unmerged HUD/renderer work**. Integrate/rebuild on MAIN's current
verified source before testing if it has additional work; do not replace
that work with this older-base DLL. Reverify the resulting SHA/PE32 and all
leased files through MAIN's deployment workflow, with game closed and
verified backups. Add only `[Render] GloveRendererEnabled=1` for this test,
never replace the whole installed INI. Default OFF remains the rollback.

Use a noTMD/free-hands save. Confirm `glove-renderer: startup ready` and then
`draw=1 calls=4` before interpreting the physical result; initialization
alone is not proof of visible content. If these are absent, diagnose the
exact section/pose eligibility rather than asking the user to repeat an
unchanged baseline. Do not enable for held-weapon/TMD hands, which are
intentionally outside this vertical slice. Physical script and limits:
[glove-animation.md](glove-animation.md).
