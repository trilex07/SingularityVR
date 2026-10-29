# Valkyrie AR9 MAIN native HD integration � 2026-10-29

Status: offline geometry/GPU/mechanics checks PASS; Steam/PSVR2 PENDING.
Base: accepted main a1b506b9 (Volk), source asset commits 8ea45224/75b47af3.
Branch: integration/valkyrie-ar9-hd-physical-reload. No changes to aim,
input, sockets, grip thresholds, native ammo acknowledgment, TMD or HUD.

## Actual native producer and conversion

The named SkeletalMesh is in the original RvGame.xxx, NOT the similarly named
weapon audio package. Read-only whole-package decompression yields core.u SHA
09de450d9f620952c3701d874beb90a8bd9235263975157a641b9e3441336c92.
Native source VB is 11387 vertices, stride 32, FNV3607008755; all positions,
UVs and skin weights match the original PSK after exactly one Y reflection.
Body palette: cog_machinegun, Trigger, clip_release, clip, fan, Slide.
Body IB:12834 triangles FNV615094726. Full IB:12842 FNV941526542.
The final 8 triangles have a separate one-bone palette and are NOT replaced.
This also avoids the historical final-attachment/wrong-palette witness bug.

All 37 private handoff hashes are verified before conversion. The three pinned
HD detail groups are transformed from their bone-local GLB to original native
bind space, with original body vertices/indices byte exact. Final body:
13965 vertices / 14116 triangles. Details use existing slots (clip=3, fan=4,
root=0), so native magazine collapse/carry/drop replay includes the HD ribs.
No GLB loader, new shader, extra material, draw-time I/O or extra body draw.

4K maps are 4096x2048, retaining original UV aspect. DDS: BC3 diffuse with
original material alpha, BC1 native normal, complete mip chain. glTF normal
conversion is checked against the pinned native map. Constant detail color is
baked into a proven unused rectangular UV island; only new details use it.
PBR roughness/metallic are NOT native shader inputs and are not promised as
preview-equivalent. Original shadow geometry is retained; details are not
added to native shadow draws.

## Runtime and reload ownership

Render/ValkyrieHdNativeModel defaults OFF. Exact original full-body draw,
verified original IB, live weapon, foreground/context/layout and stereo gates
are required. Existing native adapter is profile-parameterized; accepted Volk
profile values/API remain identical. Missing/corrupt cache, incompatible stream
or foreign IB retain the original body. VB/IB binding restores on failure and
every eye. Reset releases/recreates GPU buffers. Receipts are capped at 8.

Existing Circle/left-shoulder/physical-left Grip/wrist carry/insertion/native
ack FSM is unchanged. Original reload SVWA is hash pinned,52 frames at24fps.
The measured clip departure/return frames are15/29. Existing native palette
animates source parts during eject/carry, collapsing only the absent clip.
During the existing180ms refill seating window, Slide/fan use the source
return segment, endpoint-normalized as D(f)*inverse(D(51)) before the SAME
controller root. Root/Trigger/release/clip palettes are preserved byte exact
by this additional helper. No mandatory bolt and no camera/root animation.
Outside that window native first-shot animation remains untouched.

## Reproduction (private inputs; no proprietary content committed)

Read-only decompress owned RvGame.xxx with tools/uedecompress/uedecompress.exe
into build/valkyrie-hd/core.u. Then:

    python tools/assets/prepare_valkyrie_runtime.py --package ../valkyrie_ar9/final_handoff --cooked build/valkyrie-hd/core.u --output build/valkyrie-hd/runtime
    ./tools/tests/test-valkyrie-hd-runtime.ps1
    ./tools/tests/test-volk-hd-runtime.ps1
    ./tools/tests/test-dual-weapon-tmd.ps1
    ./tools/tests/test-accepted-profile-guard.ps1
    ./spikes/view_matrix/build.ps1

Original three Valk reload caches must also exist (SVWA/SVRK/SVRT); restore only
verified accepted originals if the clean installer omitted them. Do not replace
RvGame.xxx or any unrelated package. Asset changes use the exact six-file
session lease and production deploy tool with accepted-Volk preservation pins.

## Evidence and physical gate

All1282 new triangles vs three hash-pinned OBJ sources: maximum error
4.17960938e-6 UU; rigid slot ownership/winding/body-attachment separation PASS.
Original52f data:1503 timeline/controller samples, endpoint error7.12220033e-19;
root/Trigger/release/clip preservation PASS. D3D9 and D3D9Ex actual HD pixels:
4502/4502 per view; state/failure/fallback/recreation PASS. Flat offline pixel
fixture proves submission/state, NOT native materials or headset appearance.
Accepted Volk regression:1892 triangles,3002 mechanics samples, GPU5401/5401
on both APIs PASS. Remaining suite/build/deployment identities recorded in the
release manifest after completion. No measured frame-time claim.

One grouped PSVR2 check: inspect Valk HD and both hands; turn/roll gun and head;
fire and grip with two hands; Circle eject, acquire behind left shoulder, carry,
insert, confirm exactly one native refill and fire again; cancel once. Check
Volk/TMD briefly for regression. Physical visual/performance acceptance PENDING.

## First real Steam session — E9133DBA

Trilex reported "chyba okej działa". General reload/handling appears acceptable;
not a per-feature acceptance statement. OpenXR rendering/submission was active.
Actual bounded receipts show BOTH Volk and Valkyrie original-body fallback:
`replaced=0 gate=source-layout-or-IB`. Thus HD geometry runtime acceptance FAILED,
even though CPU cache READY and offline GPU tests passed. MAIN restored exact
accepted Volk DLL485FE0AF, INI5D1C4A1A and all six asset preimages, verified.
Do not call geometry integrated/physically verified from this session.

Next bounded observation expands this exact refusal into actual offset, stride,
frequency, IB identity and full vertex declaration (first two attempts only).
Additional streams include size/stride/frequency, without data readback/GPU sync.
This distinguishes declaration layout from unrelated IB/stream causes. No
visibility, tracking or renderer eligibility guard has been relaxed.

## Native declaration correction — observed second Steam session

Both eyes reported stream0 offset0/stride32/frequency1 and the exact expected
source IB. The actual declaration is POSITION FLOAT3@0, TANGENT UBYTE4@12,
NORMAL UBYTE4@16, BLENDINDICES UBYTE4@20, BLENDWEIGHT UBYTE4N@24, and four
TEXCOORD FLOAT16_2 aliases@28 with semantic indices3,2,0,1. The first proven
violation was the adapter expecting normalized tangent/normal elements.

Valkyrie now validates that actual declaration, including semantic identities
and all four UV aliases. Native shader, vertex bytes, materials and transforms
are unchanged. Volk declaration policy is unchanged. The GPU fixture now uses
the captured declaration, rejects the old normalized fixture and invalid UV
semantics, and submits4502/4502 pixels in D3D9 and D3D9Ex. Volk regression
submits5401/5401; both preserve bound state and fallback/reset behavior.
Native replacement receipt and final HD headset acceptance remain PENDING.

Queued next integrations: Kasimov802f099a then Centurion218e7652. Each follows
this integration's actual rendering acceptance gate; private source assets are
not redistributed or merged wholesale from asset branches.

## Corrected real Steam submission — F043D4F5

Source c9d67044; optimized x86 PE32 build; exact DLL SHA-256
F043D4F57E78B2B4EDC13613FA953C73D94E486135BA4630B7BEBF0DB73804E1.
Real Steam session20261029-231800 confirms eye0 AND eye1 replaced=1,
vertices13965 triangles14116 hr00000000, native-material1/controller root.
OpenXR shouldRender1/haveEyes1 with successful capture/submission. This proves
actual HD native draw submission; final headset appearance and performance
remain separate gates. Full evidence archived by the guard after normal exit.

Next packages inventoried: Kasimov18 SHA-256 entries and Centurion41 entries
match their local manifests. Neither package has yet been integrated into this
candidate. Preserve weighted Kasimov wire chain and separate168-face lens path;
preserve native Centurion cylinder/socket and79-bone hierarchy.

Final physical result: Trilex confirmed all four requested points correct:
HD appearance, magazine, firing and two-hand grip. Current corrected session
also records eject -> grab-fresh -> inserted/request -> native-ammo-ack,
0/72 ->30/42, consumed1/nativePermits1. Guard restored accepted485FE0AF DLL,
5D1C4A1A INI and six asset preimages with all profile pins verified. No measured
frame-time comparison; unvisited features are not asserted PASS.
