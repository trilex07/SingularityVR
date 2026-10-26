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

1. Resolve changed/unmanifested corpus intentionally. SIDE must supply current
   text/donor-to-native-package/asset mappings and a current manifest; never
   assume an extra filename belongs to a specific NPC. Obtain hash-bound final
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
**Blocked:** stale corpus provenance and final take selection/approval; storm
native graph proof; native remaster package validation.
**Physical acceptance:** PENDING. No Steam access or game launch occurred.
