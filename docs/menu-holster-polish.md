# Frontend comfort and holster marker artwork

SIDE, 2026-10-16. User: main menu before gameplay feels too close; MAIN's
thick white holster rings need nicer VR-shooter-style presentation (Arizona
Sunshine as inspiration). Original procedural art only, no copied assets.
Started origin/main fb73156, fast-forwarded to MAIN's committed 17bf1ed so
the existing ring and menu paths are present; NO uncommitted MAIN edits copied.
MAIN integration has newer work: cherry-pick our follow-up, don't replace it
with this standalone older integration base. Zero Steam access/deployment.

## Evidence and hypothesis

`HeadPlane` multiplies panel half extents by distance: moving it alone changes
vergence, NOT angular size. Menu render and right-controller ray share that
plane; changing the picture without its hit geometry would introduce drift.
Class 2 owns the main frontend; class 1 is boot, 3 pause, 4 other/options.
Native RT producer already has its own inset. Shrinking it as well as the
provider would apply tuning twice, so producer/crop/UV are unchanged.

Presentation uses shared helpers: `FrontMenuScalePct=88` multiplies only
class-2 provider/pointer inset (0.84 -> 0.7392); `FrontMenuDistanceMm=1400`
sets its depth. The native backbuffer fallback has the same clip scale and
existing -0.16 clip-Y offset. Pause/boot/mod VR Settings panel are unchanged;
other/options class remains on its existing layout. Missing keys = 100/0,
exact old presentation. No eye projection, native menu canvas or gameplay edits.
Falsifiers: menu/laser click disagree, double scale, cropped labels or changed
pause/boot. MAIN checks real menu producer/provider before extending scope.

Holster producers are `DrawHolsterMarker` / `DrawKnifeBeltMarker`, sharing
horizontal XR-plane geometry at actual existing hip/belt anchors. `Style=1`
replaces only artwork: thin broken ivory arcs, dark keyline, steel/amber
details, gun/knife pictogram and proximity highlight. Outer footprint remains
22.54 cm; no pulsing/time animation. Highlight uses valid actual hand pose,
never changes radii, input, holster calibration, grab/dock events or haptics.
Missing/0 `HolsterMarkerStyle` keeps the old white ring. Visuals are an original
interpretation, not a verified pixel replica of Arizona Sunshine.
New style hides outside focused gameplay (including menu, mod panel and
cinematic); legacy style keeps its old gates.

One batched triangle-list draw per eye and marker, fixed-capacity stack arrays,
no texture/asset loads or per-frame heap allocations. Same physical geometry
is independently projected through each XR eye; near-plane triangles are
skipped, never collapsed to a centre. Eye scissors prevent seam bleed.
New path requires actual backbuffer RT and captures ALL state before setup;
restores textures/shaders/stream0/declaration/FVF/viewport/scissor/render states
on draw failure or skipped geometry too. It never changes RT/DS or writes
depth/stencil/scene alpha. Existing cached state block is released on Reset.
Falsifiers: missing cue, wrong anchor, eye disagreement, state leak or hitch.

## Offline evidence and MAIN test

Run `tools/tests/test-menu-holster-polish.ps1`: menu size/depth isolation,
exact opt-out, projected corner/UV agreement from controller origin, cue
topology/finite/footprint/proximity/determinism, stereo projection, transaction
adapter failures, and existing holster/pause/knife regressions.
Generated `build/menu-holster-polish/holster-preview.svg` is CPU geometry,
NOT a game screenshot or proof of D3D/PSVR2 appearance.
L1_PASS: six x86 /O2 /W4 /WX test executables, including 64 visual-state
combinations and 202 cue geometry/proximity cases. Gun has 1122 vertices,
knife 1116 (374/372 triangles), at most four batched UP draws total for both
markers/both eyes. CPU preview rendered and inspected separately. This is
not a measurement of native GPU draw calls or headset performance.
L2_PASS: clean source `44b4bf7e03eaeab4260c898ad3fd91bcfde9d8cc`, /O2 x86
PE32 (014C/010B), instrumentation OFF, manifest working-tree status empty.
DLL SHA-256 `04C5B55AA6400F4F68FFABE246DF9120E967CF497D60B55A6A527F8E198BDDF8`.
Existing unrelated compiler warnings remain. Initial build caught the Windows
legacy `near` macro; fixed by descriptive variable names, with a macro-context
regression check, then re-tested/rebuilt clean. L3/D3D and L4 PSVR2 UNKNOWN.
Private compile artifact/provenance/logs/CPU preview:
`build/menu-holster-polish/candidate`. It contains no wholesale user INI.

MAIN applies only three [Render] keys (no wholesale calibration INI):
FrontMenuScalePct=88, FrontMenuDistanceMm=1400, HolsterMarkerStyle=1.
Test menu and artwork in separate sessions if isolating any regression.
Stay in headset: inspect main menu/readability and laser selections, then
inspect both hip cues while moving the respective hand near/far; dock/draw
gun and knife at the unchanged locations, pause/Options, return to main and
exit normally. MAIN backs up/verifies hashes/deploys/restores its accepted
installation; SIDE never launches or claims physical acceptance.
