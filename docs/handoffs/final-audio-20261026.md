# Final audio delivery — 2026-10-26

Branch: `side/final-audio-20261026`, based on `71728b5d7e2712d1640fa18fa58e48bd6dc708ee`.

**GO for offline tooling integration; NO-GO for replacing the installed audio corpus.**
This changes no runtime playback, input, camera, assets or Steam files. The original
native SoundCue events, spatialization, energy gates and timings remain authoritative.

## Actual inventory, not historical delivery claims

Reviewed `side/dubbing-corrections` (`c3c911d8`) and
`side/audio-music-sfx-remaster` (`ba8774c7`) against current private files.
The accepted base already includes their production/staging tools and reports;
cherry-picking either whole branch would not install approved native audio banks.

`C:/Dubbing-aktualny` currently has **2459 WAVs**:
story 1668, barks 268, multiplayer 409, skipped 97, intro 11, vignette 6.
The synchronized `deploy_full_manifest.jsonl` declares all **2459**:
story 1668, barks 268, multiplayer 409, skipped 97, intro 11, vignette 6.
**0 declared WAV SHA-256 values disagree with current bytes (100% hash pass).**
There are **0 unmanifested files**.
The scanner read complete PCM payloads; all files conform to mono PCM16 24 kHz.
Dubbing status is updated to **TECHNICAL_PASS_APPROVAL_PENDING**.

Manifest SHA-256:
`4265d48aeb82182ef0f70775514cb4391218398aa100b836edba4d336f0ae99e`.
Full local inventory (each declared WAV SHA and PCM information):
`build/audio-release-private/inventory-final-20261026.json`.
Its SHA-256 is
`2fb5ceee425dbf5f594d074b9a9bc9b784bd43f37f0517fa30817a28d5b99353`.
The committed `final-audio-inventory-20261026.json` contains counts, all 24
remaster hashes/native identities and a sorted actual-file-hash-list digest.
No extracted audio or proprietary package is committed.

The source handoff records user approval of **Kathryn voice profile B** and a
radio-filter style. That is not a recorded final selection of every currently
changed WAV. Donor references identify original game actors, but no checked
record establishes permission to publicly redistribute their generated clones.
Retain assets privately; MAIN must distinguish private testing permission from
any public audio distribution decision. No rights claim has been fabricated.

## Remaster: eight sets, 24 existing listening variants

For each set, original / conservative / enhanced WAVs have identical decoded
PCM channel count, sample width, sample rate and frame count. This does not prove
creative quality, loudness comfort, native encoding or in-game performance.

Seven exact graph identities are documented in their private metadata:

- Centurion: `wp_SP_ARPistol_SF.xxx / arPistolFire03 / arPistolFireNEW`.
- Kasimov: `wp_SP_ARSniperRifle_SF.xxx / sniperFire08`.
- TMD impulse: `RvGame.xxx / tmdFireImpulse16`.
- Valkyrie: `wp_SP_ARMachineGun_SF.xxx / machinegunFire57`.
- Shotgun: `wp_SP_ARShotgun_SF.xxx / shotgunFire35`.
- Exploration: `MUSIC_exploreDrone01_XSA.xxx / CONCEPTexploreDrone01`.
- Grom combat: `MUSIC_combatBossGrom01_XSA.xxx / combatBossGrom01`.

`SP_Freight_Audio_2010_XSA.xxx / windStormHigh01_R` is still a heuristic graph
match and is rejected by staging even if a WAV is selected. The old reports
refer to approval of three pilot comparisons, not an exact conservative/enhanced
selection per current hash. Five newer sets have no final listening receipt.
Close/distant/reflection layers and random alternate waves must remain intact.
In particular, replacing shotgun `shotgunFire35` does not replace its random
alternate `shotgunFire36` or the complete gun sound.

## Implemented release path

`tools/audio/audio_release_gate.py` provides three offline commands:

1. `audit`: checks actual bytes, full PCM payloads, manifests, unexpected files,
   all 24 variants and graph confidence. Never silently updates approval pins.
2. `stage-dubbing`: requires an exact audit-hash approval record, chosen WAV
   hashes and explicit existing **package + asset** identities. Joins the pinned
   accepted lookup, preserves its bytes and existing runtime WAV filenames.
   Only campaign story/skipped/vignette can enter; intro/UI/barks/MP are excluded.
3. `stage-remaster`: stages one approved take per measured native wave plus its
   pinned graph metadata. The heuristic storm cannot enter. Output is explicitly
   **native-bank-build pending**, not a deployable `.xxx` or playback overlay.

Both stagers refuse changed sources, missing listening/rights evidence, duplicate
identities, external output and existing output. Files stay below ignored
`build/audio-release-private/`. Approval records use schema
`singularityvr-audio-approval-v1`, exact `audit_sha256`, explicit
`listening_approved`, `rights_approved`, `approved_by`, `listening_evidence`,
`rights_evidence`, and `selected` entries with exact `id` and `sha256`.
Dubbing entries additionally name `package` and `asset`. Remaster IDs are
`<canonical-set>/restored_conservative.wav` or `.../remaster_enhanced.wav`.
No production approval record is created by a tool or test.

`prepare_xtts_native_banks.py` now accepts optional exact `nativeScope` entries.
This lets an approved subset replace its actual native waves without requiring
unapproved WAVs for every other line in the same bank. Legacy manifests preserve
their prior behavior. Missing/ambiguous identities refuse; unmatched native
audio stays unchanged through existing `native_story_package.build`.

Actual campaign dialogue consumes **native SoundNodeWave PC Vorbis payloads**.
Copying WAVs and `polish_story_voice.jsonl` alone does not replace NPC/radio audio.
The existing native tool fits speech into the original frame budget, preserves
wave properties/non-PC payloads/SoundCue graph and validates container inverse
and independent decoder roundtrip. Unsafe or >1.35x takes retain accepted audio.
Historical native bank work exists; its reports do not approve these new bytes.

For remaster, do not write SoundCue event/attenuation/pitch settings or play a
second overlay. After selection, pin actual accepted physical/logical packages,
encode the selected wave with the original channels/rate/frame budget and use
the existing guarded bank writer. It refuses unknown package layouts or moving
non-wave exports. No actual remaster bank has passed that gate in this task;
compatibility of each non-localized target bank remains **NOT RUN**.

## Commands and verification

From this worktree, repeat the actual inventory:

```powershell
python tools/audio/audio_release_gate.py audit --dubbing-root C:/Dubbing-aktualny --dubbing-manifest C:/Users/Trilex/vr-mods/SingularityVR-dubbing-corrections/dubbing-corrections/deploy_full_manifest.jsonl --audition-root C:/Users/Trilex/vr-mods/SingularityVR-audio-music-sfx-remaster/audio-remaster-private/audition --out build/audio-release-private/new-inventory.json
```

Run focused tests from `tools/audio`:

```powershell
python -m unittest test_audio_release_gate test_xtts_native_banks test_stage_xtts_native_delta test_native_voice_coverage test_kathryn_repair_handoff test_room_voice_audit
```

**PASS: 32 tests**, including 12 new safety/identity/PCM regression tests.
Broad `unittest discover test_*.py` is unsuitable here: existing
`test_opening_dialogue.py` is a fixture CLI requiring `--fixture`, not an
importable unittest module. Its private native fixture test was **NOT RUN**.
Existing audio semantic/ASR, production rights and listening approval: **NOT RUN**.

**PASS: x86 PE32 `/O2` build**, no new runtime C++ modifications.
Offline DLL: `spikes/view_matrix/d3d9.dll`, SHA-256
`6575C4891928057EDD6305129589419BA45346FF38C5B21167133F37A8E5BB40`.
Build log: `build/audio-release-private/x86-build.log`.
This DLL is not an audio replacement and was never deployed. Existing C++17
codecvt deprecation warnings remain; build and format-string guard passed.

## Required closure gates for MAIN

1. Refreshed manifest integrity is independently PASS (see follow-up below).
   Resolve the five uncovered native lines and the 103 competing-take choices;
   never assume a folder name proves native campaign ownership. Obtain hash-bound final
   listening selections; keep any public redistribution decision separate.
2. Stage only approved exact takes. Build native banks against the accepted
   pins with existing tools and independent container/decoded-PCM checks.
   No silent partial-success claim: report each retained English/accepted line.
3. Select conservative/enhanced takes for seven proven graphs; prove the storm
   graph separately or keep original. Validate native bank layouts before use.
4. One grouped MAIN/PSVR2 session after the above: known radio + NPC + recording
   in PL; each selected weapon/TMD/environment sample versus original; verify
   no duplicate speech/effects, cut words, timing/energy/spatialization changes
   or mix imbalance. English/unselected events remain unchanged. Revert an
   unsuccessful audio experiment to exact accepted hashes.

**Completed:** inventory, exact-byte release gates, native subset integration,
offline selected-remaster packaging, regression/build verification.
**Blocked:** final take selection/approval, five missing native takes; storm
native graph proof; native remaster package validation.
**Physical acceptance:** PENDING. No Steam access or game launch occurred.

## Independently verified refreshed delivery — follow-up to `6d5d9125`

Inspected the external manifest refresh `6d5d9125` and dubbing SIDE
`9c260562`. Re-ran full PCM/SHA inventory rather than trusting their updated
summary. The audit reproduces SHA
`2fb5ceee425dbf5f594d074b9a9bc9b784bd43f37f0517fa30817a28d5b99353`
exactly: **2459 files, zero mismatches, zero unmanifested files**. This resolves
the previous integrity blocker, not final content approval or native delivery.

Implemented `tools/audio/plan_campaign_audio.py` and seven identity regressions.
Current checked lookup SHA is
`4feec18c3c8e5d9462c56a6335bec645ff39d5d2ff8aca6138fe50c59a69e020`.
This is the repository lookup, not a claim that installed Steam matches it.
Curated skipped/vignette source pins and every exact bank/asset are recorded in
`final-audio-campaign-plan-20261026.json` beside this handoff.

**Exact native mapping results:**

- 1667 story basenames uniquely match the lookup; 409 of those map to **MP**
  native banks and are excluded from campaign delivery, regardless of folder.
- 1258 story campaign takes + 97 curated skipped + 6 curated vignette map to
  **1361 takes for 1258 unique native SoundNodeWaves in 116 campaign banks**.
- All 103 skipped/vignette aliases collide with an existing story take for the
  same exact native identity; every pair has different bytes. The plan exposes
  both hashes. MAIN/SIDE must select exactly one approved take per identity;
  no precedence or creative approval is invented by this tooling.
- `story/ae99b20311bcf364.wav.tmp.wav` is the one unmapped file. Its SHA is
  `4092d0ea4d73ffa8f20289f064f9e81ee435690d10abcb6af904fc6923dfd75a`.
  It is a temporary artifact, not a new native dialogue target. Leave it private.
- Five existing campaign lookup rows have **no current WAV**, all with empty
  runtime filename: `SP_VI2_zone02a_2010_LOC_INT/VI1_1505_ANN`,
  `SP_VI2_zone02a_LOC_INT/VI1_1505_ANN`,
  `SP_VI2_kismet02_LOC_INT/VI2_5012_DEMY`,
  `SP_VI2_kismet03_LOC_INT/FR8_4406_GRD2`,
  `SP_VI2_kismet04_LOC_INT/VI2_5012_DEMY`.
  Retain accepted native audio until SIDE provides reviewed exact takes.

Full plan: `build/audio-release-private/campaign-native-plan-final-20261026.json`,
SHA `ae860d9f5f1acac11793c36ae836d624b2d831ebbcd45dadcb5c3191230ca059`.
Exact 1361-row mapping CSV beside it:
`campaign-native-plan-final-20261026.mapping.csv`, SHA
`f559ecf41dfecafccafb19f49f3be0cdcfb8688b05aa8b8ae57b7057cdd565e5`.
This is identity mapping, not final text/voice approval or native export decoding.

The committed plan lists **all 116 required native banks and their exact waves**.
Next build inputs are an approved selected-take manifest, MAIN's current accepted
profile containing each chosen bank's physical SHA, a private offline copy of
those accepted banks, an independently checked decoder and ffmpeg. Current
checked-in UI/HUD accepted profiles contain no native-bank pins. Do not borrow
an unverified original bank just to satisfy the builder. Derive decoded logical
SHA and original PCM budgets from those pinned offline copies; unknown layouts
must refuse. A fallback to accepted audio is a retained line, not a successful
new replacement. No bank, lookup or file from Steam was accessed for this task.

Repeat the identity plan with the independent audit, repository lookup and
SIDE's explicitly pinned `skipped_pl_curated.json`/`vignette_pl_curated.json`:

```powershell
python tools/audio/plan_campaign_audio.py --audit build/audio-release-private/inventory-independent-20261026.json --lookup localization/pl/polish_story_voice.jsonl --skipped C:/Users/Trilex/vr-mods/SingularityVR-dubbing-corrections/dubbing-corrections/skipped_pl_curated.json --vignette C:/Users/Trilex/vr-mods/SingularityVR-dubbing-corrections/dubbing-corrections/vignette_pl_curated.json --out build/audio-release-private/new-campaign-plan.json
```

**PASS: 39 focused tests** (previous 32 plus seven campaign identity tests).
No C++ source or audio bytes changed in this follow-up; last offline x86 `/O2`
build/hash above remains the runtime-source checkpoint. MAIN rebuilds the final
integrated candidate. Creative approval, new native banks and headset audio
acceptance remain explicitly pending; no generation or deployment occurred.


## SFX/music execution update — user delegated selection, 2026-10-26

**SUPERSEDES the earlier SFX creative-selection blocker only. Dubbing approval
and generation remain with the separate SIDE. No dubbing file was changed.**

MAIN relayed the user's explicit authorization to choose the best of the existing
24 variants and finish native integration. I selected `restored_conservative.wav`
for all eight sets, favoring the least altered existing game audio, unchanged
channel/frame counts, no added clipping, and minimal additional mastering. This
is engineering selection under delegated authority, **not human listening** and
not a claim that enhanced versions sound worse. All audio derives from the user's
existing private game-source lab; no third-party music, new voices or new content
was introduced. Extracted/generated audio and complete banks remain ignored.

### Actual usable native output

Eight `.xxx` candidate banks now exist in this worktree:
`build/audio-release-private/remaster/native_candidate/`.
The complete receipt is `candidate-manifest.json`, SHA `2f4302a34014c5863d1c8dad379fbce01768acb3a6fb204d4fdbe2646f5f588e`.
A metadata-only copy is committed as
`docs/handoffs/final-native-remaster-candidate-20261026.json`.
The status must equal `COMPLETE_OFFLINE_ONLY`, and `INCOMPLETE` must be absent.

- `wp_SP_ARPistol_SF.xxx` / `arPistolFire03`: SHA `a5fa0d72bb6b32d845e6d0fc89990f18d3a87a9fac444d698689d74233756d39`, Vorbis q2.5, codec gain -1 dB, 2 channels / 48000 Hz / 60000 frames.
- `wp_SP_ARSniperRifle_SF.xxx` / `sniperFire08`: SHA `e2d5dcae9fa7815d8ee944c4ce62c0fa2bbde60a1303f45c61221c9cab60362b`, Vorbis q2.5, codec gain -1 dB, 2 channels / 48000 Hz / 108000 frames.
- `RvGame.xxx` / `tmdFireImpulse16`: SHA `5c0cce4713cf76f857b2b771b83353ece3a0028d46317db6e4b7f499c80bbf10`, Vorbis q1.5, codec gain 0 dB, 2 channels / 48000 Hz / 96000 frames.
- `wp_SP_ARMachineGun_SF.xxx` / `machinegunFire57`: SHA `59e33f9950081e91916e07f3a86473b35826572e2dbdfa26d7749a4387d2618a`, Vorbis q2.5, codec gain -2 dB, 2 channels / 48000 Hz / 66603 frames.
- `wp_SP_ARShotgun_SF.xxx` / `shotgunFire35`: SHA `57e8e5a37b667a4cb0a4ec4c2095ddda3782063e92a7aa559cf314d4b84f33a4`, Vorbis q2.5, codec gain -2 dB, 2 channels / 48000 Hz / 72745 frames.
- `MUSIC_exploreDrone01_XSA.xxx` / `CONCEPTexploreDrone01`: SHA `20f4d520538691383a7e6fc9ccfe7b4dff6682f978db4cee2ef5b8217806da14`, Vorbis q2.5, codec gain 0 dB, 2 channels / 32000 Hz / 4224000 frames.
- `MUSIC_combatBossGrom01_XSA.xxx` / `combatBossGrom01`: SHA `eef7866eb2541ead378fc0de4a39bea5e7517c7488952404e5b1925c80fd9133`, Vorbis q4, codec gain 0 dB, 6 channels / 44100 Hz / 3432648 frames.
- `SP_Freight_Audio_2010_XSA.xxx` / `windStormHigh01_R`: SHA `5a8c5cf19c38e97e4a5c4c7fef476385739978b6a1b0347767e42b194d3a2818`, Vorbis q1.5, codec gain 0 dB, 1 channels / 22050 Hz / 1323000 frames.

Native integration replaces the exact existing SoundNodeWave **PC Vorbis bulk
payload**. No new playback hook or parallel WAV player is introduced. Every
non-target logical byte (including every SoundCue node, randomization, event,
attenuation, loop/pitch/volume property, other wave, native code/export table and
offset) remains byte-identical. Native events still own scheduling, power gates,
energy, spatial behavior and playback. The existing English and Polish voices
are outside the replacement ranges and remain byte-identical to the chosen
source package. Each audition `original.wav` was also re-decoded from the actual
native input and compared PCM-byte-for-byte: all eight matched.

Vorbis encoding must fit the existing allocation to avoid speculative UE3
relocation. Valid vendor-comment padding fills that allocation exactly; no junk
is appended after EOS. Ogg CRCs, audio packets and granules are checked, as are
exact decoded frame count, channels and sample rate. The highest enumerated
Vorbis quality fitting that budget is used. Lossy encode overshoots were found
in actual tests, so only affected takes receive the minimum enumerated 1 dB
codec headroom step (maximum 3 dB); the exact gain is recorded above. Final
PCM16 decodes have **zero saturated samples** in all eight outputs. This is
another lossy encode, not lossless restoration or perceptual quality proof.

### Proven native producers and honest scope

- All five weapon/TMD waves were independently traversed again from native
  SoundCue `FirstNode`, `ChildNodes` and Raven `LinkedCue` references. Full paths
  are in each receipt's `native_consumer`. TMD's `tmdFireImpulse16` is also shared
  by native `tmdDeadLockFire`; this shared use is deliberately preserved.
- Drone: actual `MUSIC_exploreDrone01.xxx`, SoundCue export 16,
  `FirstNode` -> imported SoundNodeWave -23 `CONCEPTexploreDrone01`.
- Boss music: actual `MUSIC_combatBossGrom01.xxx`, **DynamicMusic export 1**,
  `MusicTracks` count 1 -> imported SoundNodeWave -58 `combatBossGrom01`.
  Earlier wording calling this a SoundCue was imprecise; its existing dynamic
  six-channel music consumer is preserved without modifying the companion.
- Storm: actual `SP_Freight_Audio_2010.xxx`, SoundCue export 268 `FirstNode`
  -> SoundNodeAmbient export 281 `Wave` -> imported SoundNodeWave -98
  `windStormHigh01_R`. Companion physical/logical hashes are pinned in the
  receipt. This proves a **Freighter high-wind layer**, not all island weather.
  Low wind, sea wind and thunder are untouched. Previous HEURISTIC_MATCH is
  resolved for this exact producer only.
- Shotgun has other randomized takes such as `shotgunFire36`; those were not in
  the prepared 24 variants and remain native. This finite eight-wave candidate
  is not an all-weapons/all-environments remaster. No new missing assets were
  invented or synthesized to pretend otherwise.

### MAIN composition / preservation gate

Sources are private lab originals at
`C:/Users/Trilex/vr-mods/test-games/assets/Singularity/RvGame/CookedPC`.
They are **not assumed to be the current accepted installed banks**. Before
MAIN stages any bank, compare the accepted bank SHA to `source_package_sha256`.
A mismatch is a STOP for direct staging: copy the accepted bank to an offline
private source folder and rerun the builder there. It verifies that the exact
audition original still matches the target native PCM, preserves all other
accepted logical bytes, and independently roundtrips the output. Never replace
an accepted modified `RvGame.xxx` with a stock lab bank merely to enable SFX.
Use the existing MAIN backup/active-game guard and hash-verified asset lease.
This SIDE did not read/write Steam, deploy assets, launch gameplay or alter INI.

Literal-only LZO repacking increases physical bytes, especially `RvGame.xxx`
(13,369,344 -> 28,137,730 bytes), while logical size is unchanged. Native audio
allocation and decoding workload are unchanged; startup I/O may differ. No
performance claim or 90 Hz acceptance is made. MAIN can profile startup in the
grouped acceptance session; do not add a new compressor dependency speculatively.

### Reproduce and verify

```powershell
python tools/audio/build_native_remaster_candidate.py --sources C:/Users/Trilex/vr-mods/test-games/assets/Singularity/RvGame/CookedPC --audition C:/Users/Trilex/vr-mods/SingularityVR-audio-music-sfx-remaster/audio-remaster-private/audition --decoder C:/Users/Trilex/vr-mods/SingularityVR-audio-music-sfx-remaster/tools/uedecompress --ffmpeg C:/Users/Trilex/AppData/Local/Microsoft/WinGet/Packages/Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-9.0.2-full_build/bin/ffmpeg.exe --output build/audio-release-private/remaster/native_candidate
```

**Offline PASS:** all eight real banks independently unpacked by the existing
UE3 LZO decoder to byte-identical expected logical output; all eight native
Vorbis payloads decoded by FFmpeg with exact duration/channel/rate and zero
saturation; all eight source audition PCM identities checked against native.

**Regression PASS: 49 tests**, including 10 new tests using a real Vorbis
encoder/decoder, CRC corruption/truncation refusal, packet/granule preservation,
non-target logical byte preservation, allocation-growth refusal, Steam-path
refusal and independent whole-package LZO decode. The prior 39 dubbing/staging
regressions still pass. Set `SINGULARITYVR_FFMPEG` and
`SINGULARITYVR_LZO_DECODER` for the real codec/container tests.

No C++/runtime DLL changes: previous offline x86 PE32 `/O2` hash remains
`6575C4891928057EDD6305129589419BA45346FF38C5B21167133F37A8E5BB40`.
MAIN builds its combined runtime independently. **GO for MAIN review and guarded
composition only; Steam/audio/physical acceptance PENDING.**

One grouped PSVR2 audio check: fire available remastered weapons and use TMD,
confirm original scheduling, direction and layer balance with no missing tails
or distortion; allow native exploration/combat music and the Freighter wind when
encountered, marking unavailable scenes NOT TESTED. Listen for voice audibility
against effects, then quit normally. No claim that an eight-wave pack finishes
all campaign audio or replaces the separate new Polish dubbing delivery.
