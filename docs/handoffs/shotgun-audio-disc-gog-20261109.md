# Shotgun player crash: confirmed bank defect; disc/GOG compatibility audit

Player reports crash after receiving shotgun, no supplied log/dump. Screenshot
shows D:\SteamLibrary\steamapps\common\Singularity\Binaries, including a 4,111 KB
d3d9.dll, texture_override.log, native codec DLLs. Directory alone establishes
neither executable provenance nor the cause/stage of the crash. Do not attribute
it to Bink, OpenXR or the launcher from the screenshot.

## Hypothesis, evidence and falsifier

Older weapon remasters used long Vorbis vendor padding, rejected by shipped UE3
decoder. Prior rifle/pistol/sniper repairs omitted shotgun. The installed bank
also matches TrilexModInstaller inventory/singularity-v7.json and its earlier
local-audio-integration receipt. This does NOT prove the remote player's hash.

Read-only copy of installed RvGame/CookedPC/wp_SP_ARShotgun_SF.xxx:
SHA256 57E8E5A37B667A4CB0A4EC4C2095DDDA3782063E92A7AA559CF314D4B84F33A4,
2,555,904 physical bytes; independent unpack gives 2,501,560 logical bytes.
Native x86 /O2 test_native_vorbis_header.cpp runs in a separate process using
the observed UE3 ov_open_callbacks ABI, final state-size argument 0x2D8.
Original decoder rejects shotgunFire35 with -133 / OV_EBADHEADER (test exit3).
Other 53 PC Vorbis waves open with 0. Vendor field is 2055 bytes.

Repair moves padding into a valid user comment, vendor 12 bytes. New reviewed
pin added to tools/audio/repair_kasimov_vorbis.py; no native codec replacement,
runtime code/renderer changes, cue changes or sound re-encoding.

Prepared candidate bank SHA256:
88548AA9F8F20044418B9CE121FDF3C74923A6BBC0DB6D39A2F2AEE340AE383F.
Logical payload offset1851881, allocation24712. Native wave properties preserved:
Duration1.5155208110809326, NumChannels2, SampleRate48000,
RawPCMDataSize290980, OnVRAMSampleRate48000, VolumeRMS0.2476121187210083.

All 54 repaired-bank waves open with native decoder. All 92 compressed audio
packets unchanged. FFmpeg decoded PCM identical: 290980 bytes,
SHA256 A5FB795D480F5AA77E657D45478176A4A430571EFBA80C703D465047E1043883.
Independent native package unpack equals intended fixed logical bytes;
all bytes outside the allocation unchanged. Vorbis allocation suite: 11 PASS,
including independent whole-package decompression (no optional skip).

This is a PROVEN asset compatibility defect and a plausible crash cause, not a
confirmed explanation of the player's crash. If player bank differs, or the
same crash survives replacement with the repaired bank, investigate native
exception/module and pickup/first-shot/reload stage separately. Header open is
not a native full-PCM decode or gameplay acceptance test.

## Installer follow-through

Read-only audit of TrilexModInstaller/tools/asset-worker/audio_remaster.py:
accepted_allocation preserves signed old recipes first, and permits repaired
allocation only for profile04_weapon_walkiria_ar9. A source repair here does
not update the published player installer. Shotgun needs a reviewed new recipe
with repaired encoded/logical/target hashes, support for its profile in that
worker, reconstruction from owned original bank, signed manifest/catalog and
bundle/reinstall verification. Do not merely replace target hash or change old
signed recipes silently. Installer files were not edited/published in this task.

## Actual player logs and collection

Mod log: %LOCALAPPDATA%\SingularityVR\view_matrix.log, not Binaries. Earlier
request for Binaries\view_matrix.log was incorrect for this source lineage.
Archived view_matrix_<timestamp>.log files survive later launches. Added
tools/collect-player-crash.ps1: explicit GameRoot, hashes of relevant binaries,
INI/bank, latest10 mod/game logs, texture_override.log, relevant Application
events1000/1001 from7days (max20 matching Singularity.exe), running-game PID.
Creates fresh output/ZIP; no game launch, registry/save/install edits. Tested
on local install, not on the player's machine. Example player path from image:

    powershell -NoProfile -File .\collect-player-crash.ps1 -GameRoot 'D:\SteamLibrary\steamapps\common\Singularity'

## Disc/GOG pack audit

C:\Users\Trilex\Downloads\SingularityPack.zip: 14,089,403,399 bytes,
240 entries, 14,121,145,558 uncompressed bytes. Only relevant native binaries,
GOG installer header EXE and official patch inspected/extracted into private
build directory. No installer or game launched; no DRM modification used.

Retail disc EXE: 32,041,808 bytes,
SHA2561004F2D0394B70AEAE0C17295BFBC0E7F11B7002A878E4318ABC080B97AC6F78.
Contains SecuROM sections, base00400000, .text RVA2000/size137FD1A,
.rdata RVA1382000, .data RVA17B6000, entry RVA21E47D0.
Current Steam EXE: 27,444,736 bytes,
SHA256BD459ACD87D4B82693928DEE4ED437C086D5928A8E0428F3AD54B6FC114C0F51,
.text RVA2000/size138011A, .rdata RVA1383000, .data RVA17B7000,
entry RVA1B542ED; Steam .text is protected on disk, not a decoded reference.
Same preferred base does not establish same map. At the mod's CalcViewRotation
address0104E390 retail file bytes begin8BC1C20400 (ret4), unlike the expected
target. Other address samples differ; do not infer one universal delta.

Disc binkw32/ogg/vorbis/vorbisfile bytes match local Steam installed libraries
exactly, including vorbisfile SHA256
2D0C4A54B5E269375AD8797CFF557536D1F778481AB2FFCC3D398D3CF01568B1.
Thus the proven header defect is relevant to the disc decoder too. This does
not establish full mod compatibility with the disc executable.

Official Retail Patch1.1.exe is present, 24,820,184 bytes,
SHA2562DF0D5FCB45618825778E1EB8D7E5FD45E8651366F79048DB493CEB48B38B1B6.
Its PE overlay was inspected; installed patched game EXE was NOT recovered or
executed. No claim of supported retail1.1 yet.

GOG supplied files: setup_singularity_2.0.0.5.exe (29,550,968 bytes,
SHA25635ED903E7B775D0BC76096EC55DFCE4F1B10BA8994BF4B883204D3DF40BCBC7F),
-1.bin4,194,304,000 bytes and -2.bin2,735,851,497 bytes. 2.0.0.5 is installer
package version, not verified game build. Inno installer payload cannot be
extracted with existing7-Zip; no local innoextract/innounp found. Actual GOG game
EXE/dependencies/assets have NOT been compared. No unsupported compatibility
claim from ENGINE_NOTES' older GOG reference.

Required to support these builds:
1. Recover actual installed GOG and official patched retail executables, hash and
   independently verify code/dependency/asset versions. Preserve native DRM and
   store launch requirements; GOG/manual uses direct EXE from Binaries, verified
   Steam install continues Steam URI route already implemented in installer.
2. Resolve/validate per-build native targets: CalcViewRotation0104E390,
   GNames01CD4A1C/GObjects01CD4A4C, line checks00BD79D0/00A108B0 and other fixed
   targets/readers (e.g. BoolExecRva8BCE0). Central build maps or audited unique
   signatures plus instruction/prefix and structure checks; refuse unsupported
   targets safely. Never enable by only loosening EXE hash gates.
3. Verify original owned bank/package hashes for each edition before recipes;
   rebuild compatible assets from each native baseline where hashes differ.
   Check movie language tracks, native Vorbis ABI and required legacy x86
   PhysX/VC/DirectX dependencies; do not install/replace them speculatively.
4. Isolated per-edition launch, menu/load, pickup/fire/reload, TMD, save/load and
   stereo/physical checks. Current status: retail and GOG support UNKNOWN.

Private evidence/candidate: build/shotgun-disc-gog-20261109/, including
archive-files.json, exe-static.json, shotgun-native-decode.json,
repair-validation.json, repaired/audio-repair.json and collector-check.zip.
Original installed bank and renderer unchanged; no release/deployment performed.
