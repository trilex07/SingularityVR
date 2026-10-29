# Kasimov / Centurion physical follow-up — 2026-10-30

Branch: `fix/hd-kasimov-centurion-physical-followup`.
Previous tested binary: FB1DE1BC421904A17C13F3A3C582104D466D71DA8037268168EADE072C3F40B8.
User requests fixes deployed and retained for morning testing; no launch or timed rollback. Prior session had already restored before that instruction. Backups remain mandatory.

## Evidence and corrections

1. Reload input/FSM is not the first failure: archived log records `grab-fresh` for Centurion and Kasimov at shoulder distances 0.05–0.164 m, followed by discard without insertion. Kasimov carried-wrist publication excluded the active native TMD material-0 route; Centurion palm publication similarly required the no-TMD glove. Extend those specific routes and use the actual final `local * BuildTmdAssemblyC` chain for moved=3. Preserve pawn/weapon/transaction identity, tracking and freshness checks, native ammo authority, existing shoulder/seat thresholds. Actual carried visibility and insertion remain physical PENDING.
2. Centurion HD witness had a four-pointer-change verification budget: two valid alternating IBs exhausted it and permanently selected native fallback. Replace with a bounded eight-resource positive/negative cache; verify each unique resource once, retain COM identities until reset. GPU regression alternates two actual valid IBs 24 times. This proves the defect and correction, but old runtime logs did not establish that this exact budget caused the user's appearance change. Add at most 16 route transitions to distinguish remaining causes.
3. Kasimov two-hand support reused an incompatible paired native pose after the independent TMD glove became active. Derive wrist-relative-to-cog from original `ch_inview_sniper_rifle_mesh` PSK and original sniper idle PSA, not guessed offsets. Idle/fire maximum matrix difference 0.00655444 UU. Publish exact native body root only after successful stereo submission/restoration; solve the existing support equation against final BuildGunC. Preserve original finger animation and all other weapons. Numerical tests exercise shader palette, inverse ride, head-local changes, wrist rotations, identity/freshness and both eyes.
4. Kasimov had no diegetic HUD mount/body signature. Add its exact 11389-vertex/12942-face body and use the source receiver bounds at root bind station for the housing, keeping accepted projection/reflection. Four separate ownership latches; no legacy plate suppression unless both eyes submitted. This is a new UI placement requiring PSVR2 readability/scope clearance acceptance.
5. WER: 2026-10-30 00:11:10, XAudio2_5.dll 9.27.1734.0, c0000005 +0x4DE73, report a4951b3f-3ddf-4b7c-8156-05a9fdb1a05a. No surviving dump/original caller chain. Installed remaster `sniperFire08` has a vendor string padded to 1924 bytes. Shipped game vorbisfile.dll rejects it with -133 (OV_EBADHEADER); identical compressed audio with short vendor and user-comment padding opens successfully. Controlled comment/page variants isolate vendor padding. Move padding to a valid user comment, preserve every audio packet/granule, sound property and non-target logical byte. FFmpeg PCM regression and independent package decompression pass. This is a PROVEN asset compatibility defect; causal connection to XAudio crash remains INFERRED, crash-free gameplay PENDING.

## Reproducibility and assets

- Private evidence: `build/post-psvr2-20261030/` (ignored): copied session log/Report.wer, original PSK/PSA measurements, decoder disassembly and native-header fixtures, audio repair and GPU logs. No proprietary game content committed.
- `tools/reload/audit_kasimov_attachment.py --psk <original PSK> --psa <sniper.psa> --report <private JSON> --header <generated header>`; skin matrices are already native Y-reflected, PSA reflected exactly once. Header constants in `kasimov_support_geometry.h`.
- `tools/audio/repair_kasimov_vorbis.py --bank <private bank> --logical <unpack_chunks output> --output <private directory>` refuses unknown source SHA and Steam paths. Independently decompress result and compare logical output.
- Bank source SHA E2D5DCAE9FA7815D8EE944C4CE62C0FA2BBDE60A1303F45C61221C9CAB60362B; repaired SHA 41DC6FE2034688A7110E528BEA113CCFAF386994932E8B8EB297CDC5E8B68198.
- Exact 32629-byte PC payload at logical offset 1895751; candidate Ogg SHA 0F49D254C210632118E7B0D529B41B4F75E248B83584570A384827CF2902368D. Source Ogg A28D0B0B7F891E615984D1FD78D2BD0B106CED65FE1C1DF99B80F631599A00B6.
- `test_native_vorbis_header.cpp` uses the shipped UE3 ABI: ov_open_callbacks has a final state-size argument 0x2D8 (export checks [esp+24h]), not the stock five-argument signature. Run in own process against private copied game DLLs. Native Vorbisfile SHA 2D0C4A54B5E269375AD8797CFF557536D1F778481AB2FFCC3D398D3CF01568B1. Header acceptance only; no native full-PCM decode claim.

## Validation

- x86 /O2 Centurion physical/palm, reload controls, settings and native detour ABI PASS.
- Original/private HD assets, source skinning, animation/wires and texture hash checks PASS.
- Actual D3D9 + D3D9Ex Centurion/Kasimov stereo pixels, state/failure/reset/fallback PASS. Centurion 24 IB alternations PASS.
- Mounted HUD: 32 stereo projection checks, existing observed-pose fixtures and actual atlas hostile-state/restoration/reset tests including Kasimov PASS.
- Existing Seeker ABI/power/ownership regression unchanged; Kasimov support 18 final shader/ride/eye cases PASS.
- Dual-TMD, independent hands/shot haptics, carry/wrist identity and deployment guard regressions PASS (see private logs).
- Vorbis 11 tests, one optional whole-package test skipped; independent actual repaired-bank roundtrip separately PASS. Native old header -133 / repaired 0; FFmpeg decode PASS.
- No real game launch, runtime performance measurement or physical PASS from these checks. No 90 Hz claim. No PR merge.

## One morning acceptance session

1. Kasimov: fire one shot, then several; confirm no crash, scope/R1 and correct aiming. Inspect mounted HUD without scope obstruction.
2. Kasimov: Circle, left shoulder Grip, visibly carry and insert magazine, fire; two-hand Grip attaches glove at fore-end and releases naturally.
3. Centurion: existing Circle/open/eject sequence, shoulder Grip, cylinder insertion/close, fire. HD body must remain stable throughout.
4. Quick Volk/Valkyrie reload and TMD check only if a regression appears. Close normally and report failures. Native ammo must transfer once; offline tests do not certify this session.

Deployment identities/backup paths are recorded in the final follow-up candidate JSON after build. `VrSettingsSessionOnly=0`, NIS Sharpen 100%, OLED Cinematic. Candidate remains installed without a timer, as explicitly requested.

## PR71 follow-up optimization

The exact MenuDrawTrace code/test delta from a9a6c635 is included on this branch, with its historical evidence index copied under docs/performance. Normal startup defaults to OFF and reads Diagnostic/MenuDrawTrace with default 0; explicit 1 enables the existing census. The disabled frame gate also avoids clearing the group table or querying menu tokens. The earlier independent microstutter capture subsystem (47c7b6af) is not part of this scoped integration; the copied historical report describes SIDE's branch, not enabled instrumentation here.

Production-header test test-menu-trace-default-off.cpp executes 20,000 disabled calls without a D3D device and checks no token reads/table clears. Trace policy, suppression policy and sparse-flush tests also pass. This establishes the removed diagnostic work, not a current frame-time or FPS gain. Preserve the existing HD fixes, native audio repair and installed INI unchanged. The updated candidate receipt records the new combined binary and exact immediate rollback identity. No launch/automatic restore.
