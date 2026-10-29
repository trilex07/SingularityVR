# Kasimov SNV-E99 native HD integration — MAIN 2026-10-29

Branch: integration/kasimov-snv-e99-hd-physical-reload. Base: physically accepted Valkyrie3dc177f1, not a replacement for its accepted controls. Source asset802f099a imported only five reproducible tooling/documentation files. All18 private package hashes verified. No proprietary mesh/image bytes committed.

## Native geometry and ownership

Original ch_inview_sniper_mesh is in RvGameMPLoader_SF, an internally chunk-compressed package (not whole-file script compression). Existing offline unpack_chunks decoder verifies every block and leaves the installed file untouched. Logical package SHA6dcbda6047384c34724859787e9643278f414ed93191844c661718d128165994.

Body11389 source vertices,12942 faces; native body palette cog_sniper,wire_2,wire_1,wire_3,Trigger,clip. Complete original source VB FNV2571367080 and body IB FNV855862860 preserved byte-exact. All80 source bones and143 dual-weighted points preserved. Full original IB includes168 separate lens faces; those97 lens vertices remain in the original prefix but never enter the replacement body IB. Original lens shader, optics and draw remain untouched.

Thirteen source-derived machined details add780 faces/1248 vertices. Final12637 vertices13722 body faces; details bind to original clip/root/wire bones. Complete glTF node hierarchy/TRS converted once from glTF Y-up to native coordinates; source original bind comparison max6.198883e-6UU; emitted detail error3.808198e-6UU. No guessed offset or dominant-bone conversion. Original indices/weights/UVs remain exact.

Private runtime SVKS cache, guarded diffuse/normal DDS4K4096x2048 with13mips. Native normal source37ea2c1a3c368e1a2b7428490d5a36d9673ab0244dfc5af013ac604c17daed77 retained; native alpha preserved. Constant detail base color/neutral normal occupy a proven unused UV island384,64,64. Original native materials/lighting reused. PBR metallic/roughness and offline lens material are not inserted into UE3.

## Animation and physical reload

Existing Circle -> shoulder physical-left Grip -> carried clip -> socket insertion -> native ammo acknowledgment FSM and validity checks unchanged. Original239-frame60fps source cache and170vertex180face magazine cache remain pinned. HD clip details share original slot5 and the same existing attached/carry/drop replay. No second reload mechanism or mandatory bolt.

Existing source animation resolves wire_1 -> wire_2 -> wire_3. Only inside the existing180ms seat window, return deltas for slots1/2/3 use the source-complete endpoint normalization, applied to the controller-owned root. Root, Trigger and hand-owned clip are byte-exact; native animation resumes unchanged outside window. No authored root/camera motion.

## Safety and verification

[Render] KasimovHdNativeModel=0 default; independently switchable. Runtime reads cache once at input initialization, creates VB/IB once per device lifetime, restores binding on failure/reset; strict original VB/IB/profile/hash, shader/context/scene gates. Actual declaration must match native packed tangent/normal and UV aliases; bounded first-two refusal records distinguish incompatible declarations. Original body draws on refusal. Original lens/shadow paths remain unchanged; HD details use existing body draw count, not new shadow passes.

Offline actual geometry, weights,239frame mechanics1503pose/time samples, stereo GPU D3D9/D3D9Ex4539/4539pixels and state/failure/reset/fallback PASS. Endpoint max3.7252903e-9. This does not establish actual native shader appearance, scope correctness, controller reload comfort or headset/performance PASS.

Build, Steam receipt, exact deployment hashes and physical result recorded after completion. Preserve accepted Volk/Valkyrie, TMD, UI, localization and bridge; no main merge. One grouped PSVR2 session: inspect Kasimov from both sides and roll; scope-to-eye, R1 stability; fire; two-hand grip; Circle and one shoulder/insert reload; verify scope and firing afterward. Report magazine visibility/seat, scope, visual artifacts and observed performance. Native ammo must update once. Restore known base if failure, verifying all files.

## Reproduce private assets

Build tools/uedecompress/unpack_chunks.cpp using its existing x86 host toolchain; unpack user's owned RvGameMPLoader_SF.xxx into ignored build/kasimov-hd/loader.u. Run:

python tools/assets/prepare_kasimov_runtime.py --package ../kasimov_snv_e99/final_handoff --cooked build/kasimov-hd/loader.u --baseline ../dependencies/Singularity/Binaries/texture-override-remaster --output build/kasimov-hd/runtime

Run tools/tests/test-kasimov-hd-runtime.ps1. Private inputs/output are never public release assets without the established distribution workflow. No game package repacking.
