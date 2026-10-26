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
