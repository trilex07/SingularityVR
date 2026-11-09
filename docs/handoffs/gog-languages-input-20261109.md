# GOG languages and German input contracts — 2026-11-09

## Observed scope and hypothesis

Continue the offline edition port and determine whether Spanish has a distinct
native executable. The owned GOG language configuration and archive are the
source of truth, not a language name or assumed universal address delta.
Hypothesis: localized languages share native code except German; independently
matching German input producers will allow those consumers to use explicit
profiles. Falsifiers: distinct configured/hash-verified EXEs, nonunique code
matches, disagreement between native references, ABI/control-flow differences,
or a failed profile/mutation fixture. No installer/game/registry execution.

## Spanish is present and uses the English EXE

Extracted game/language_setup.ini explicitly configures:

| Language | Executable source | configured gamelanguage DWORD | LOC packages |
|---|---|---|---|
| English | Binaries/lang/Singularity.exeen | 0 | 168 INT |
| German | Binaries/lang/Singularity.exede | 3 | 168 DEU |
| French | Binaries/lang/Singularity.exeen | 1 | 168 FRA |
| Spanish | Binaries/lang/Singularity.exeen | 4 | 168 ESN |
| Italian | Binaries/lang/Singularity.exeen | 2 | 168 ITA |

The configured executable is copied by GOG to Binaries/Singularity.exe. Reading
this configuration did not run that copy or change any registry value.
There are only two configured executable hashes:

- EN/FR/ES/IT: 27,084,288 bytes,
  8D438B6C0BD51AB8F24A6A2B6AC569DED6DEF84B20DC407357C4F78E97C0BE1E.
- DE: 27,092,304 bytes,
  49D263B2B53A9BDB770869C51AC8D13537C61BCBDAA7BA3612335CD11DAEC740.

Spanish also has game/RvGame/Movies/copyright_sing_ESN.bik. The archive contains
168 actual *_LOC_ESN.xxx packages plus RavenShared_LOC_ESN.xxx.uncompressed_size
metadata; a loose LOC_ESN substring count gives169 and overcounts packages.
Native address mapping for Spanish is therefore the English mapping. Localized
assets do differ, and their loading, text, audio and stereo are not runtime
verified by an identical executable. No separate Spanish-address guess is added.

## German producers independently matched

tools/map-singularity-input.py masks only PE HIGHLOW relocation operands and
decoded direct branch/call immediates, retaining instruction structure and all
other bytes. All12 specified blocks have exactly one match in candidate .text.
Lengths are explicit; short blocks/prologues are not claimed to identify an
entire function. Full input hashes are separately pinned and checked by C++.

| Block | EN | DE | matched bytes |
|---|---|---|---|
| pad enumeration | 011D2FC0 | 011D2C50 | 29 |
| pad callback | 011D2180 | 011D1E10 | 787 |
| UI poll | 011D31C0 | 011D2E50 | 128 |
| cursor X | 011CFC10 | 011CF8A0 | 48 |
| cursor Y | 011CFC40 | 011CF8D0 | 48 |
| cursor position | 011CFC70 | 011CF900 | 48 |
| TMD StartFirePower | 0100D790 | 0100D420 | 248 |
| TMD StopFirePower | 0100AEC0 | 0100AB50 | 194 |
| mouse initialization block | 011D7439 | 011D70C9 | 90 |
| mouse polling block | 011D325B | 011D2EEB | 1629 |
| mouse cooperative block | 011D5F2B | 011D5BBB | 32 |
| mouse helper | 011CF3E0 | 011CF070 | 96 |

Seven data globals have24 independently matched native reference contexts,
with agreement within each group. Engine/viewports/left-button/pad/mouse-slot
have4 each, MouseX3, MouseY1:

| Global | EN | DE |
|---|---|---|
| engine | 01CF8778 | 01CF7778 |
| viewports | 01D1FCC4 | 01D1ECC4 |
| MouseX FName | 01CF566C | 01CF466C |
| MouseY FName | 01CF5674 | 01CF4674 |
| left-button FName | 01CF568C | 01CF468C |
| pad identity | 01D1FC7C | 01D1EC7C |
| mouse slot | 01D1FC78 | 01D1EC78 |

UI poll's exception-handler operand is independently read as EN0170768A /
DE0170728A. The two TMD methods have identical complete bytes despite different
locations; their existing 64-bit fingerprints and native observer ABI remain
unchanged. Address changes elsewhere differ (e.g. engine references move+C0,
MouseX references+30, late input blocks-370); no blanket code delta is used.

## Implemented, deliberately not activated for DE

game_build_input.h stores both measured contracts. Native menu engine/viewport/
FName reads and cursor/poll hooks, pad enumeration/callback identity, all4
missing-mouse guards, and TMD observer install/remove now consume the profile.
Missing-mouse continuations are each the measured block end; original EN
continuation addresses remain unchanged. Exact signatures, hash gates and hook
failure/rollback behavior are retained. UI instruction bytes and the exact pad
helper include the selected profile operands.

Core fixture now checks both actual PEs against complete input contracts:
4 mouse hashes, pad callback, 3 cursor blocks, 2 TMD bodies, poll prologue,
enumeration helper, 5 native global operands and target readability. It rejects
mutated gates, wrong mouse slot/pad identity, mixed EN/DE profiles and an
unreadable engine global, in addition to the earlier core-map/root checks.

**RuntimeAllowed still refuses DE.** Remaining fixed-address consumers include
locker/inventory operations, dual-gun scaling and pause RTTI/RTT witnesses.
Those must be independently ported or explicitly scoped before enabling the
whole alternate renderer. Source-level input contracts alone do not prove
German gameplay, native menus, VR tracking, hotplug, haptics or stereo.
No new compatibility hashes or renderer are added to the published installer.

## Verification

- Real EN/DE PE fixture: core+input contracts, all mutation/refusal cases PASS;
  protected retail on-disk core refusal PASS. No owned game code executed.
- Existing native cinematic CPU suite PASS.
- Native gamepad enumeration fixture x86 /O2 PASS: native result preservation,
  failed enumeration not cached, invalidation/owner-thread cases, unreadable
  code causes no hooks.
- Context cursor script PASS, including current focus, default-OFF native cursor,
  native-dispatch receipt and per-eye clipping checks.
- Menu polish CPU fixture PASS; full older menu script FAIL at existing
  configuration expectation `frontend menu scale is not 72 percent`.
  Both parent HEAD and current unmodified INI have FrontMenuScalePct=100;
  the script expects72. No unrelated INI or test-expectation change was made.
- Main DLL x86 PE32 /O2, format analysis PASS. Initial new-header inclusion from
  an anonymous namespace failed compilation; explicit inclusion alongside the
  core profile fixes the namespace ownership. Final build manifest records SHA.
- Runtime, launcher, stereo and headset tests NOT RUN. Installed Steam artifacts,
  saves, registry, GOG setup and existing public installer remain untouched.

Read-only repeatable tools:

    python tools/audit-singularity-languages.py EXTRACTED_GAME_ROOT RAR_LIST --output LANGUAGES.json
    python tools/map-singularity-input.py OWNED_EN_EXE OWNED_DE_EXE --output INPUT_MAP.json

Private reports are under build/shotgun-disc-gog-20261109: gog-languages.json,
german-input-map.json, german-input-global-xrefs.json, German/EN native fixtures,
and german-input-build.log. Mapper requires existing pefile/capstone modules.
No executable code, localized game assets or decoded function bytes are committed.
