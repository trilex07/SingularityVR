# Bink Polish audio pilot — SNG_intro

## Result

The new-game intro is `SNG_intro_12-14-09.bik` (123.023 s, 1280×720, 2997/100 fps). The checked local copy already contains an aligned Polish narration track. This changes the pilot from “create a new Polish dub” to “verify the existing Polish track and its runtime selection”; no synthetic voice has been generated and no movie has been modified.

The copy was fully decoded offline with the local FFmpeg build. `ffprobe` reports 11 Bink Audio DCT tracks and no subtitle stream. The first copy and the separate L3 sandbox copy have the same SHA-256: `AF53C8F8CFE0885D6E54CC259C305C1B616053750F3FE8DAA0084635CA9A3686`.

Audio ordinals below are FFmpeg's zero-based `0:a:N` indices, not the global stream index. Local Whisper base/small recognition finds the following narration content at matching times:

| Audio ordinal | Recognition | Evidence / status |
|---:|---|---|
| 0 | No narration recognized; stereo | Content not acoustically identified |
| 1 | English narration | 1945–2010 story, about 6.2–113.5 s |
| 2 | No narration recognized; mono | Content not acoustically identified |
| 3 | No narration recognized; stereo | Content not acoustically identified |
| 4 | French narration | Same story and timing |
| 5 | Italian narration | Same story and timing |
| 6 | Spanish narration | Same story and timing |
| 7 | German narration | Same story and timing |
| 8 | Japanese narration | Same story and timing |
| 9 | Russian narration | Same story and timing |
| 10 | Polish narration | Same story and timing, about 6.2–110.0 s |

Language identification is offline ASR evidence, not a human listening approval. The Polish recognition was reviewed against the English narration to correct proper names and obvious ASR errors. The resulting 23-cue SRT is a subtitle draft for listening/review; the original Bink has no subtitle stream, so the sidecar will not appear in-game by itself.

Local pilot outputs are kept outside Git:

- `bink-pilot-local/pilot/SNG_intro_polish_voice.wav` — extracted Polish narration only, mono PCM 16-bit, 48 kHz, 123.04 s, mean -23.4 dBFS, peak -5.4 dBFS; SHA-256 `B97FCFEEEB7E06E951491693D0B9B4397919570E65A30B958031D617BE9DAC70`.
- `bink-pilot-local/pilot/SNG_intro_polish.srt` — 23 ordered cues ending at 110.020 s; structural validation passes; SHA-256 `B2ACE2ED2A22123501D1331693ECE9D4177ADE6114CD3191AFF19C3C2122AAAD`.
- `bink-pilot-local/pilot/SNG_intro_transcript_en_pl.md` — timed EN/PL ASR draft, uncertainty notes, and editorial decisions.
- `bink-pilot-local/movie_inventory.json` — metadata and SHA-256 for all 25 local `.bik` files.

The SRT translation is intended to read naturally and follow the original English meaning. It is not claimed to be a verbatim transcription of the Polish recording. ASR suggests a possible meaning mismatch around “volatile”: the existing Polish line may say “łatwopalna”, while the English narration describes E-99 as unstable. This needs human listening/translation review. The WAV is an isolated narration reference, not a finished mixed soundtrack: the other Bink tracks were not auditioned, and their roles (music, ambience, effects, alternate audio) remain unknown.

## Encoder and file boundary

The installed local FFmpeg build decodes `binkvideo`/`binkaudio_dct`, but `ffmpeg -h encoder=binkvideo` reports that no Bink encoder is available. No RAD/Bink encoder executable was found in PATH, the project tree, or the usual installed-program directories. No encoder or dependency was installed. Therefore this pilot creates no replacement `.bik`; it relies on the Polish stream already present in the original multilingual Bink. The legality and availability of a future RAD encoder must be checked against the vendor's current terms before obtaining or using one.

The Singularity executable imports `BinkSetSoundTrack`, confirming an API seam for choosing Bink audio tracks. Static import presence does not prove how this build selects a track or whether Polish game language selects ordinal 10. MAIN must verify that in the installed game; SIDE did not launch the game or Steam.

## Offline reproduction

Use a private copy of the user's movie files and an already available FFmpeg build. The tool does not edit the input movie, and it does not download anything:

```powershell
py -3 tools/bink_media.py inventory "<private-movie-folder>" `
  --ffprobe "<existing-ffprobe.exe>" `
  --output "<private-output>/movie_inventory.json"

py -3 tools/bink_media.py extract-audio "<private-movie-folder>/SNG_intro_12-14-09.bik" `
  --audio-ordinal 10 --ffmpeg "<existing-ffmpeg.exe>" `
  --output "<private-output>/SNG_intro_polish_voice.wav"

py -3 tools/bink_media.py validate-srt "<private-output>/SNG_intro_polish.srt" `
  --duration 123.023023

py -3 tools/bink_media.py validate-video "<original.bik>" "<candidate.bik>" `
  --ffprobe "<existing-ffprobe.exe>" --ffmpeg "<existing-ffmpeg.exe>"
```

`validate-video` compares codec, dimensions, frame rates, frame count, duration within one frame, audio/subtitle stream count/layout, then fully decodes the candidate. It has not been run on a replacement candidate because no Bink encoder is available. Its source-equals-source self-check passes and only validates the harness.

## Local movie inventory

Metadata below is from the local test-game copy. Each has 1280×720 video. Subtitle stream count is zero for all 25. Audio ordinals/languages were only classified for the intro pilot; audio-bearing films are not yet dubbed or otherwise audited.

| Movie | Duration (s) | FPS | Audio streams | Status |
|---|---:|---:|---:|---|
| `ATVI_logo.bik` | 7.52 | 59.94 | 4 | Logo; audio content not audited |
| `badEnding.bik` | 83.00 | 24 | 11 | Audio present; not audited |
| `barisovTowerFilmReel.bik` | 122.38 | 24 | 10 | Audio present; not audited |
| `burningBuildingRecap.bik` | 23.33 | 24 | 11 | Audio present; not audited |
| `Copyright_Sing.bik` | 5.00 | 30 | 1 | Copyright/logo; not audited |
| `copyright_sing_ESN.bik` | 5.00 | 30 | 1 | Copyright/logo; not audited |
| `Copyright_sing_eu.bik` | 5.00 | 30 | 1 | Copyright/logo; not audited |
| `copyright_sing_fra.bik` | 5.00 | 30 | 1 | Copyright/logo; not audited |
| `Copyright_sing_fra_na.bik` | 5.00 | 30 | 1 | Copyright/logo; not audited |
| `copyright_sing_ita.bik` | 5.00 | 30 | 1 | Copyright/logo; not audited |
| `goodEnding.bik` | 80.00 | 24 | 11 | Audio present; not audited |
| `katyaEnding.bik` | 89.08 | 24 | 9 | Audio present; not audited |
| `loading_screen.bik` | 2.00 | 24 | 0 | No audio stream |
| `MA2_5500.bik` | 70.00 | 30 | 9 | Audio present; not audited |
| `MA6_1000.bik` | 55.03 | 30 | 9 | Audio present; not audited |
| `mir12_video.bik` | 109.17 | 24 | 11 | Audio present; not audited |
| `PR99_0800.bik` | 54.27 | 30 | 9 | Audio present; not audited |
| `ravenlogo.bik` | 15.90 | 30 | 4 | Logo; audio content not audited |
| `saveDemichevRecap.bik` | 14.92 | 24 | 11 | Audio present; not audited |
| `SNG_intro_12-14-09.bik` | 123.02 | 29.97 | 11 | Pilot: embedded Polish narration found; game selection unverified |
| `SNG_logo.bik` | 11.00 | 30 | 1 | Logo; audio content not audited |
| `SP_Freighter.bik` | 32.37 | 30 | 4 | Audio present; not audited |
| `UE3_logo.bik` | 5.00 | 24 | 0 | No audio stream |
| `uglyEnding.bik` | 97.33 | 24 | 11 | Audio present; not audited |
| `VI2_3000.bik` | 70.00 | 30 | 9 | Audio present; not audited |

## QA boundary and handoff

- Offline structural checks: source intro fully decodes; Polish audio extraction succeeds; SRT validates; original copies hash-identically.
- Automated audio checks: Polish voice track is 48 kHz mono PCM after extraction, 123.04 s, peak below 0 dBFS. This does not establish perceived mix quality or that dialogue/music/effects are separated.
- Human listening: not performed by SIDE. MAIN/user should listen to the extracted WAV and compare it with the running intro before deciding whether the existing Polish narration is acceptable or should be replaced by a newly generated performance.
- Runtime: not tested. MAIN must verify that the installed game's Polish language selects the embedded Polish track and that the track plays through the normal Bink path. No Bink candidate should be deployed from this branch.
- Rollback: no game files were written. If MAIN later tests any replacement, first make a timestamped backup of the exact `.bik`; restore that file and verify its SHA-256 if the test fails.

No copyrighted Bink, extracted WAV, or voicepack is committed in this branch.
