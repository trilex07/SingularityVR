# MAIN: Seeker cooked-space correction, depth prime and wheel state

This follow-up supersedes the older test instructions in
`main-seeker-graphics-rootcause-20261025.md`. Worktree/branch:
`SingularityVR-main-seeker-graphics` / `fix/main-seeker-graphics-rootcause`.
No new Steam deployment, launch or physical PASS during this implementation.

## Actual failed physical session

Source `b3fa514ab3221c17c765bc7bf9fb08892c50c60b`, DLL
`B32E9A7EB91F87222253A86C7897410C0F0EFC7FEA18F56A0F385A5D27C18CF0`.
Evidence: `build/main-seeker-evidence/session-20261025-145755/view_matrix.log`.
User: beam probably okay; black silhouette remains; Seeker support hand falls
to the floor; shots miss; wheel black. The restore guard completed and MAIN
independently verified accepted DLL `610F4C21...` / INI `C5D24E16...`.

## 1. Seeker support and launch: PROVEN coordinate-contract defect

`prepare_gloves.read_psk()` returns native-space bone matrices, but raw ActorX
PNTS0000 positions. `audit_seeker_geometry.py` reflected the matrices a second
time and emitted incorrect positive-Y root/muzzle constants. This is Seeker-only.
PSA key conversion was correct; the measured `LeftGrip` remains unchanged.

Independent original cooked package witness: all **12444** packed GPU positions
at byte offset **1058626**, stride **32**, exactly match PSK positions with Y
reflected (maximum error **0 UU**). They do not match unconverted PSK positions.
Original chunk palette `[64,53,54,62,57,59,60,61,55]`, root slot 1 / bone 53;
original muzzle bone 56. IB identity/ranges are unchanged.

Corrected native measurements:

- Root translation Y: `+109.49099916` -> `-109.49099916` UU.
- Muzzle Y: `+112.88768866` -> `-112.88768866` UU.
- Related reflected rotation signs corrected; no guessed angles or offsets.
- Compiled measurements agree with independent PSK/PSA/package audit to
  **0.000003625 UU** maximum float error.

In the recorded body LocalToWorld, native Y maps to negative world Z. The wrong
root displaced the support by **218.982 UU** downward; wrong muzzle displaced
the spawn by **225.775 UU** downward. The regression replays this matrix:
correct support Z `-44.76530`, muzzle Z `-35.36231` (translated-world space,
before the final controller transform). It also checks 15 rotations/scales,
shader palette/local/controller composition, both eyes and native spread cone.

Runtime support evidence: actual final-bound SVNC 3994 triangles submitted in
both eyes; tracked hands valid, current frame/pawn/weapon witness, independent
palette 108 registers, nSeg=1. This was wrong placement, not a missing draw.
No TMD/support visibility or controller offset was changed.

Six original Spawn receipts exist: four returned null; two returned initialized
projectiles with location equal to supplied origin and velocity equal to the
supplied direction times 5000. Bad-origin collision explains the nulls plausibly,
but native Spawn failure reason is **not proven**. Native drunk/guided flight,
spread, recoil, ammunition and native damage remain authoritative. This patch
corrects the initial rendered-muzzle geometry, not subsequent steering.

Private reproduction (assets remain ignored, not distributed):

```powershell
tools/reload/test-seeker.ps1 -IndexAsset ../SingularityVR-sol-seeker/build/seeker-private/seeker.ib
python tools/reload/audit_seeker_geometry.py --psk ../SingularityVR-sol-seeker/build/seeker-private/wp_SP_ARSteerable_SF/SkeletalMesh3/ch_inview_e99_steerable_mesh.psk --psa ../SingularityVR-sol-seeker/build/seeker-private/wp_SP_ARSteerable_SF/AnimSet/e99_steerable.psa --package ../SingularityVR-sol-seeker/build/seeker-private/seeker.u --test-exe build/seeker-private/tests/test-seeker.exe --report build/main-seeker-evidence/seeker-corrected-geometry.json
```

## 2. Black silhouette: PROVEN filter refusal, pixel cause pending physical check

Latest log lines 4513/4516/4522, 5971, 6292/6294/6307, 13156 and 13466 show
exact retained VB/IB/stream/range siblings on the scene target, color-write=0,
depth test/write=1, stencil=0, non-shadow layout. All refuse ONLY because native
requested viewport is `0/.0001`, not the earlier assumed `0/0`. Driver actual
range is `0/.001`. This includes ordinary weapons, TMD and Seeker support.

`NativePrimeRange` now accepts exactly the two evidenced native requests
`0/0` and `0/.0001`. It still refuses a native request for `.001`, the adjacent
float to `.0001`, shadow/color/stencil draws and stale/unmatched resources.
No epsilon, matrix write or broad mesh suppression. Learning still requires
successful two-eye tracked color and full restoration; COM/lifetime gates stay.

Real D3D9 **and** D3D9Ex tests reproduce old-pose prime -> world -> new-pose
color with these requested ranges: OFF leaves black, ON restores world pixels
`204080`, while gun pixels stay `FFFFFF`. Reset, exact identity, changed owner,
failed viewport, stale witness and state-preservation cases pass.
This proves the omission mechanism offline; physical disappearance remains
PENDING. If the exact sibling is omitted and black pixels persist, investigate
their actual producer rather than broadening this filter.

## 3. Black wheel: PROVEN incomplete draw state; specific native leak unrecorded

Latest log has successful M02 atlas/both-eye submission. Installed neutral atlas
and repository source both hash to
`3EF1782696B4A5E1E9D93F409C5B0F6FF014B78CC42FAB06EC29110661981818`.
Missing files or a fallback wheel do not explain this session.

Production fixed-function wheel omitted blend operations, sRGB sampling/write,
stage-1 termination, alpha-test, color mask and other inherited material state.
`wheel_draw_state.h` establishes explicit legacy / atlas / composite contracts
inside the existing D3DSBT_ALL scope. Scratch remains premultiplied RGB with
straight alpha, composited with ONE/INVSRCALPHA. Legacy fallback is reinitialized
after failed staging. Atlas UVs, assets, geometry, ownership and input unchanged.

Actual GPU negative controls: old setup changes pixels with reverse-subtract,
stage-1 modulation, sRGB sampling, alpha-test or color-mask contamination. New
setup produces bit-identical baseline pixels in both eyes for all six poisoned
states on D3D9 and D3D9Ex. Fog poison alone has no visible effect on this host.
Transparent border, opaque glyph, failed staging, state/texture restoration and
Reset also pass. This does **not** identify which state the physical Seeker left
behind. `wheel incoming state:` records the first 8 weapon-state transitions
without GPU synchronization to close that remaining evidence gap.

## Verification / preserved systems

- PASS: x86 PE32 `/O2` build and static-analysis format checks.
- PASS: Seeker production adapters, original IB D3D9/Ex identity/reset, private
  geometry-to-compiled-constants oracle and recorded-frame support/muzzle test.
- PASS: depth-prime GPU test; 13 UI programs including poisoned wheel GPU test.
- PASS: dual-TMD suite; TMD runtime including real SVTG, native power gates,
  holster, reload, measured aim, WeaponLocker preview/layout/policy.
- PASS: TMD socket/beam actual callbacks; native muzzle/aim regressions.
- PASS: HUD 3D 120 actual-atlas cases per D3D9 / D3D9Ex plus pose replay.
- PASS: OLED CPU/device-lifetime and all three D3D11 presets (<=1 LSB).
- Final emitted ABI reports, clean source SHA and exact DLL SHA are in the
  candidate manifest created after commit/build.
- Steam/OpenXR/physical PSVR2 for this new build: **NOT RUN / PENDING**.

The initial sandbox HAL error `8876086A` was environmental. Rerunning only own
offline fixture executables with GPU access succeeded; no game was launched.
Accepted Valkyrie accuracy and 20 cm reload settings, HUD, Polish assets, native
TMD animation and right-weapon rendering remain untouched.

## Candidate and one PSVR2 session

Use the same controlled INI as B32E9A7E: SeekerSupportGrip/ProjectileAim=1,
SeekerBackSlot=0 (user chose native heavy lifecycle), TrackedDepthPrimeFix=1,
TrackedDepthPrimeProbe=1, TmdTrackedBeam=1. TmdTrackedMuzzle/NativeAim remain OFF.
No asset replacements or new layout. TMD beam code changes only bounded install
gate logging: its last session contained no armed/corrected receipt, so the
user's tentative visual improvement must not be attributed to that hook yet.

MAIN next deploys only after `start`, exact profile preflight and restore READY.
No active-game timeout or interruption. User stays in headset throughout:

1. Ordinary weapon over floor/wall; rotate it and use Square/L2. Look for the
   offset black silhouette, check TMD beam and ordinary firing.
2. Obtain Seeker natively. Grip/release with left hand repeatedly: full glove
   at support, returns to tracking after release, right gun remains stable.
3. Several single Seeker shots at a clear medium-distance wall/target, no nearby
   obstruction; check initial flight exits the muzzle and impacts approximately
   along it. Native drunk/guided behavior remains. MAIN compares post-Spawn logs.
4. Open R3 wheel with Seeker and an ordinary weapon: icons/background readable,
   transparent where intended, selection usable. Exit normally and report.

Do not promote this candidate as accepted until these physical checks pass.
