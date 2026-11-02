# Consolidated next candidate — 2026-11-03

Branch: `fix/tmd-gravity-rifle-audio-crash`; base `origin/main` `92fe119c187076defb2f5038e1678ec15c0da137`.
No Steam writes, game launch, save changes, merge or installer release in this task. Candidate is staged privately for tomorrow.

## Implemented, separately reversible

- **L1 gravity:** `[Input] TmdGravityGrip=1` (default). Physical-left Grip drives the existing native AntiGrav/XI_RTHUMB action when free. Support and reload/knife reserve the squeeze first. No ownership transfer mid-squeeze; release required after focus/tracking/menu interruption. OFF restores the old R3 route. L2 Impulse and Square Age remain unchanged. Current gate supports the accepted right-handed loadout; left-handed power reassignment is not claimed.
- **Rifle audio:** earlier Kasimov/Centurion repair is already on MAIN and installed banks match. Valkyrie bank was omitted. Actual installed `machinegunFire57` vendor padding is 1048 bytes: the game Vorbis DLL returns -133; repaired header returns 0. Other 42 waves open successfully. Windows event 2026-11-02 23:50:49 is XAudio2_5.dll +0x4DE73, c0000005, report d260d537-9312-4fb6-bd5b-0be1d2055cbc. Decoder defect PROVEN; causality of this crash INFERRED (no accessible dump). No XAudio DLL replacement.
- **Movies:** port scoped source changes from `039b0a0f`, not its old runtime binary. Twelve locally verified filename/memory identities select Polish native track ID 10. Old smaller-film map confused ordinal 8/9 with track ID, explaining the wrong-language receipt for PR99_0800. No movie modifications; unknown buffers retain native selection. `[Localization] PolishIntroAudioFix=0` rolls back routing.
- **Context menus:** archived log proves Dialog has null RTT and enters class-4 per-eye fallback. Its pointer used an unscaled canvas and -0.12 clip-Y while fallback draws scaled content with zero Y translation. Pointer now shares fallback inset/no-shift; projected direct canvases and captured station canvases retain their own matching geometry. Native GFx hit dispatch remains authoritative.
- **Cinema captions:** HudStereoPair previously returned into perspective CTAB routing before affine GFx classification; text without ViewProjection remained one full-SBS draw. Production affine c5 helper now duplicates complete caption geometry into both eyes and restores constants/scissor on success/failure. Continuation batches retain verified live-GFx classification. World matrices and native camera remain separate.

## Cinema edge limitation

Submission already uses a common LOCAL pose/size and equal-size SBS half rectangles. No proven unequal-rectangle defect was found offline. Actual D3D9Ex tests retain eight corner pixels and identical caption pixels, but do not establish physical edge equality. Added four bounded `cinema pair` receipts (two frames, two eyes) with source dimensions, rect, size, pose and swapchain. Do not claim the previously observed shorter edge physically fixed, or crop/stretch either eye speculatively. Tomorrow's consolidated test must check it.

## Private bank identity

- Before `59E33F9950081E91916E07F3A86473B35826572E2DBDFA26D7749A4387D2618A`.
- After `B8126A759169A89D92FA596D6FBE52E8D94F3BCBA0ACC24FD61967CAD87AFD7A`.
- Original and repaired decoded PCM hash `E61DFBB725D0EFD94BBEAB223C9312895F3B656CB409E2F1DD5FB4EBA6B9C887`.
- Fixed compressed audio packets, properties, lengths, offsets and all non-target logical bytes unchanged; independent package decompression roundtrip PASS.
- Generator: `tools/audio/repair_kasimov_vorbis.py`; private receipt `build/gravity-audio/repaired/audio-repair.json`. Proprietary outputs are ignored, not committed.

## Offline evidence

- Bink x86 /O2 CPU fixtures + actual installed codec memory Open/GetTrackID + 12 movie files PASS; no game launch.
- Gaze/cinema/damage/mutant suites x86 and x64 PASS. D3D9Ex actual pixels PASS: caption parity/nonblank content, scene disparity, eight source corners, state restoration. Legacy D3D9 HAL unavailable on host: NOT RUN, not PASS.
- Frontend comfort/menu-scale/scene/mono fixtures PASS, including fallback/direct context cursor roundtrips.
- Dual-TMD/reload/aim/holster/WeaponLocker + OLED suite PASS, including new gravity squeeze ownership fixture.
- Independent-hand/shot haptics fixture PASS.
- Vorbis fixed-allocation suite 10 PASS, 1 optional whole-package test SKIP; actual rifle bank independently roundtripped as above.
- Optimized final binary identity/provenance is in private `candidate.json`, generated after source commit.

## Tomorrow: one session, only after exact-hash guarded deployment

Retain installed settings, HD assets, bridge and localization. Snapshot current installation first: observed installed CBF16F93 differs from released A16FC192; never label an automatic snapshot a newly user-approved baseline. Use project deployment tools, closed-game preflight and hash-verified backup. Renderer and rifle bank are both required; copying only DLL omits the audio repair. No automatic launch or timer today.

1. Fire Valkyrie single/automatic shots; verify no crash, preserve native audio/VFX.
2. Free left L1 gravity with a valid native target; then support Grip and magazine acquisition. L2 and Square stay accepted controls.
3. Play an affected projector/laptop film: Polish narration, music/effects retained.
4. Load confirmation and weapon/TMD station popup: laser, native arrow and click coincide across the panel.
5. In-engine cinema: full scene, equal visible edges and complete readable captions in both eyes. Preserve accepted mutant shot assistance.

Physical PSVR2 and runtime crash acceptance: PENDING for every new change. Leave Steam unchanged until deployment is explicitly resumed.
