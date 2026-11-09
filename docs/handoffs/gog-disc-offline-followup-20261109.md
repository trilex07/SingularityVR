# Shotgun publication and offline edition checks — 2026-11-09

## Published shotgun repair

TrilexModInstaller branch fix/shotgun-audio-20261109, commit d6cf46b.
Existing installer-1.1.0 release refreshed in place, signed catalog 1.1.6,
audio-remaster v7-20261109-shotgun-header. The full ZIP contains the updated
embedded asset worker; reopening an older 1.1.0 EXE is insufficient.

[Unchanged full installer download](https://github.com/trilex07/TrilexModInstaller/releases/download/installer-1.1.0/Trilex-Mod-Installer-1.1.0-win-x64.zip).

Full ZIP SHA256 860F447E43EFC30E2171F3C72A71B1C32CF03AFBA3AE7C298EF6F9698E285E25.
Repaired shotgun bank SHA256 88548AA9F8F20044418B9CE121FDF3C74923A6BBC0DB6D39A2F2AEE340AE383F.
Only the audio repair and necessary installer/worker/catalog metadata shipped;
the renderer, hand/laser/settings packages and other game payloads were preserved.

15 producer tests, 106 core tests, desktop Release publish, frozen worker and
actual C# consumer reconstruction, isolated install/reinstall/damage detection/
repair/uninstall/exact original restoration PASS. Initial long default fixture
path failed worker extraction; the documented short TRILEX_TEST_ROOT run passed.
All five remote uploaded digests and every published SHA256SUMS entry verified.
A stale predecessor checksum for untouched wonderlands-vr-core.zip was corrected
after independently verifying that asset; its payload did not change.

Player should freshly download/extract the full ZIP, then update/reinstall
Singularity with Audio Remaster selected. Check receiving shotgun, first shot,
several shots and reload. The -133 / OV_EBADHEADER bank defect is proven;
the remote player's crash cause and gameplay clearance remain UNCONFIRMED.
If it persists, collect %LOCALAPPDATA%/SingularityVR/view_matrix.log and the
matching Windows event/dump before another launch. The user contacts the player.

## GOG files actually inspected

Supplied setup_singularity_2.0.0.5.exe and its two split .bin volumes were read
without executing setup or the game. Portable innoextract 1.9 identified setup
resources; the game payload is split RAR and selected files were extracted with
existing 7-Zip. No installed dependency, registry, Steam, save or DRM edits.

English game EXE (support/app/Binaries/Singularity.exe and lang/Singularity.exeen):
27,084,288 bytes, SHA256 8D438B6C0BD51AB8F24A6A2B6AC569DED6DEF84B20DC407357C4F78E97C0BE1E.
German lang/Singularity.exede: 27,092,304 bytes,
SHA256 49D263B2B53A9BDB770869C51AC8D13537C61BCBDAA7BA3612335CD11DAEC740.
Installer version 2.0.0.5 is not treated as a game build identifier.

All 7 owned source banks for the published audio module exactly match its
beforeSha256 pins: pistol, sniper, Valkyrie, shotgun, explore drone, boss Grom,
ocean storm. All 7 were reconstructed through actual Installer.Tools
verify-recipes, enforcing both original and final hashes: PASS. The GOG shotgun
source is 1,441,792 bytes, SHA256
50254FAD1DAA0B62D2D08740F22A447160691B5E6FC9058F83A3D6C4D3BEBC9F.
GOG RvGame.xxx: 13,369,344 bytes,
SHA256 4153C4889C90635069ED226D918CDB76BE618F46B1BAEA152AC10CFFDF59E9EF.
Selected GOG native codec/PhysX/Bink libraries match the supplied disc/local
libraries. No speculative replacement of legacy dependencies.

## Guarded renderer candidate, not published

game_build_layout.h centralizes measured core addresses and validates code plus
independent references to object/name roots. Main consumers use the selected
profile instead of assuming one universal edition delta.

| Target | English original map | Measured German core map |
|---|---|---|
| CalcViewRotation | 0104E390 | 0104E020 |
| GNames | 01CD4A1C | 01CD3A1C |
| GObjects | 01CD4A4C | 01CD3A4C |
| LineCheck | 00BD79D0 | 00BD7A50 |
| SingleLineCheck | 00A108B0 | 00A108E0 |
| bool interpreter | 0048BCE0 | 0048BCE0 |
| bool dispatch table | 01CC8730 | 01CC7730 |

Each root has 12 independently matched native references; the retained selector
checks opcode+operand at 0042FCB4/00417ED6 and readability of both TArrays. It also
checks Calc/Line prefixes, the 48-byte SingleLineCheck instruction shape excluding
edition-dependent operands, and the bool interpreter's dispatch/mask reader.
Disassembly confirms Calc ret18 and SingleLineCheck ret1C in both images.

Hook_CreateDevice checks this before its device hooks/Ex conversion/native root
walks, caches the pre-detour verdict for subsequent device creation, and routes
unrecognized/ambiguous profiles through original CreateDevice. This is a scoped
refusal of that initialization path, not a claim that the proxy's earlier
public API/input hooks cease to exist.

English GOG passes all 9 further original-map checks in
tools/audit-singularity-edition.py: 4 complete missing-mouse block fingerprints,
gamepad callback fingerprint, 2 complete TMD power fingerprints, native UI poll
prologue and exact enumeration helper. Those checks use existing production
signatures, not newly invented acceptance hashes.

**German core recognition is deliberately runtime-disabled.** Its other native
consumers differ: UI poll/getters, engine/viewport/name globals, mouse guards,
gamepad callback, TMD haptics, locker/inventory targets, scale/RTTI witnesses.
Recognizing the camera/object map does not validate those consumers. The fixture
asserts that only the English profile is allowed to initialize VR. Neither EXE
hash was added to the public installer's automatic compatibility allowlist.

## Disc boundary

Original retail EXE SHA256
1004F2D0394B70AEAE0C17295BFBC0E7F11B7002A878E4318ABC080B97AC6F78,
32,041,808 bytes, contains SecuROM-protected disk code. At 0104E390 it has a
ret4 path, unlike the expected Calc target. The mapped disk fixture refuses it;
this does not establish what its decoded runtime code would be.

Official Retail Patch1.1.exe SHA256
2DF0D5FCB45618825778E1EB8D7E5FD45E8651366F79048DB493CEB48B38B1B6,
24,820,184 bytes. Its 23,830,568-byte PE overlay is old InstallShield6 format.
An actual officially patched game EXE was not recovered. Setup/game/DRM removal
were not executed. Supplied third-party DRM-removed executables were not used.
Retail1.1 runtime mapping and full edition acceptance remain UNKNOWN.

## Validation and reproducibility

- x86 /O2 core-map fixture PASS on actual EN and DE raw PEs, without loading code.
  Every identity/code gate is independently corrupted and must refuse; unreadable
  object roots must refuse. DE recognition must not enable runtime initialization.
  Protected retail disk image refusal PASS. PE fixture accounts for GOG EN's
  unaligned SizeOfImage by rounding the final page, without executing the image.
- Existing native cinematic fixture PASS: packed bool masks, field bounds,
  unsupported reader refusal and assert-only publication.
- Main DLL x86 PE32 /O2 build and format-string analysis PASS. Build provenance
  and SHA are in spikes/view_matrix/d3d9.build.json, not a release artifact.
- Runtime/gameplay/stereo/headset tests NOT RUN. No renderer deploy or publication.

Read-only audit example:

    python tools/audit-singularity-edition.py OWNED_EXE --output REPORT.json --require-original-map

The stricter command returns 0 for the English original-map prerequisites and
2 for a mismatch; mismatch does not itself prove another edition unsupported.
Compile spikes/view_matrix/tests/test-game-build-layout.cpp as x86 /O2 and pass
OWNED_EXE plus english-map, alternate-map or unsupported. Private commands,
hash/ABI/xref/audit reports, binaries and extracted assets remain under
build/shotgun-disc-gog-20261109; no proprietary bytes are committed.
Installer consumer report: installer-shotgun-20261109/artifacts/gog-audio-consumer-check/recipe-report.json.

Next edition gates: independently map remaining German consumers; recover and
audit an official patched retail EXE; then isolated per-edition startup/menu/load,
shotgun pickup/fire/reload, TMD, save/load and stereo/headset acceptance. No
full disc/GOG support claim follows from these offline prerequisites alone.
