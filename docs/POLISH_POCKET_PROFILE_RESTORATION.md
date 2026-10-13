# Restoring the Polish Pocket-TTS repair voices

The 205 failed-line rows are read from the existing repair manifest; the
corrected text and original replacement WAVs remain unchanged. New outputs go
to the ignored local directory `audio/polish/repair_candidate_20261013/full_audit_pocket/`.
No source pack, Steam file, or accepted voicepack is overwritten.

## Recovered profile mapping

| Dialogue profile | Authorized local prompt reference | Rows in the 205-line repair set | Original settings |
|---|---|---:|---|
| `narrator_user` | `audio/polish/user_reference/spokoj_reference.wav` | 176 | Pocket-TTS Polish 6L, quantized; story 0.55, UI 0.45 |
| `renko_user` | `audio/polish/user_reference/spokoj_reference.wav` | 1 | Pocket-TTS Polish 6L, quantized, 0.55 |
| `barisov_dmitri` | `audio/polish/accent_reference/ru_dmitri_reference.wav` | 8 | Pocket-TTS Polish 6L, quantized, 0.55 |
| `demichev_ruslan` | `audio/polish/accent_reference/ru_ruslan_reference.wav` | 4 | Pocket-TTS Polish 6L, quantized, 0.55 |
| `kathryn_tetiana` | `audio/polish/accent_reference/uk_tetiana_reference.wav` | 13 | Pocket-TTS Polish 6L, quantized, 0.55 |
| `devlin_mykyta` | `audio/polish/accent_reference/uk_mykyta_reference.wav` | 3 | Pocket-TTS Polish 6L, quantized, 0.55 |

The character-specific Russian and Ukrainian references were generated from
the project's local Piper Russian/Ukrainian models (including distinct
multi-speaker IDs), then used as Pocket-TTS conditioning samples. They are
synthetic voice anchors, not recordings of the game's original actors. Renko
and the narrator use the user's explicitly authorized recording. This
restoration follows the original manifests rather than silently replacing
those established identities.

Three radio lines that explicitly identify Devlin were reassigned from the
fallback narrator profile to the existing `devlin_mykyta` profile. Five clearly
broken UI translations were corrected locally (party invite, hit-sound labels,
and two weapon-start descriptions). Together with the previously preserved 80
correction occurrences, the local candidate now carries 85 corrected text
occurrences. Devlin's callsign is phoneticized only in `tts_text`; the Polish
transcript retains the numeric forms.

## Reproducible generation and checks

Run `tools/audio/restore_pocket_voice_candidates.py` to resume generation, or
pass `--force` to regenerate. It deep-copies the conditioning state for each
line, preserves the existing corrected `tts_text`, and records the reference,
profile, sampling temperature, and duration in the local manifest. The
generation script trims only edge silence after PCM16 conversion, leaving a
short natural release and a short fade to avoid clicks.

Run:

```powershell
python tools/audio/qa_repair_candidates.py --candidate-dir audio/polish/repair_candidate_20261013/full_audit_pocket
python tools/audio/build_pocket_ab_review.py
```

The automated QA covers sample rate, channels, PCM16 format, silence, clipping,
approximate transcript and duration. ASR flags remain review prompts; they do
not prove a recording is wrong, nor certify acting or speaker identity. The
A/B reel pairs each affected profile's **Polish character sample** with every
flagged candidate and one unflagged example per profile/pack, with previous
and new recordings adjacent. Russian/Ukrainian WAVs are Pocket-TTS conditioning
anchors only; they are not Polish listening examples and must never be placed
at the start of the Polish review reel. Perceptual approval still requires
actual listening.

## Current local candidate

The selected full set is staged at
`audio/polish/repair_candidate_20261013/full_audit_pocket_selected/` (205 WAVs,
24 kHz mono PCM16). It selects the strongest of three same-profile Pocket-TTS
generation variants per line using transcript similarity and word coverage.
No technical WAV flags remain. On the selected set, faster-whisper `base`
passed 171/205 and marked 34 for review; the independent `small` screen passed
167/205 and marked 38. These are review flags, not a subjective quality grade.

The current A/B reel is
`audio/polish/repair_candidate_20261013/pocket_voice_ab_review_intro_20261013.wav`
(2m29s). It opens with the approved intro narrator excerpt and two newly
generated Polish tests, then pairs flagged candidates with their previous
recordings. The `.md` timeline and `.json` index preserve segment labels and
source paths. It excludes the broken 1.28-second narrator demos and raw
Russian/Ukrainian conditioning audio.
Per-file profile references and generation settings are recorded in the local
`full_audit_pocket_selected/manifest.jsonl`.

## Approved intro voice reference

The user's `SNG_intro_polish_voice.wav` is 123.04 seconds, mono, 48 kHz PCM16.
A 10-second clean narration excerpt (8–18 s) is staged locally at
`audio/polish/repair_candidate_20261013/intro_reference_experiment/intro_narrator_reference.wav`.
Pocket-TTS generated the complete 5.68-second line “Musimy działać szybko.
Sprawdźmy, co dzieje się w laboratorium.” from this reference; local Polish
Whisper recovered the full sentence. A second 3.60-second test exercises
speech-only spellings for `Rocket Launcher` and `online`. These tests are local
staging only; no original or installed voicepack is changed.

The earlier two user-reference demo renders were each only 1.28 seconds and
Whisper failed to recognize them as Polish. The source user recordings are
about 25 seconds each, so duration alone is not the problem. A retry from
those references still yielded only 1.60 and 2.72 seconds with incomplete ASR.
The intro reference works substantially better and is sufficient for the
narrator profile; no longer recording is needed for that voice. The intro is a
narration voice, not a set of separate character voices.

When the intro reference exists, the Pocket repair generator uses it for
narrator/Renko lines; existing generated packs remain untouched until an
explicit candidate regeneration. Repair synthesis keeps its tested .55/.45
per-pack temperatures, raises text chunks to 100 tokens, and keeps four
post-EOS frames. The .55/.45 run scored 196/205 on base Whisper versus 192/205
for a .35 comparison. Speech-only phonetic rules live in
`tools/audio/tts_pronunciation.py` (`Rocket Launcher` → `roket lanczer`,
`online` → `onlajn`); subtitle/localization display text remains unchanged.
The 205-file repair candidate was generated from this reference for
narrator/Renko lines. Base Whisper passes 196/205; small Whisper passes 200/205;
the union flags 10 lines for listening/review and reports no technical WAV
defects. This is an automated screen, not a perceptual approval.
Reproduce the staged samples with:

```powershell
python tools/audio/generate_intro_reference_candidate.py
```

## Full revoice candidate

The user approved the intro recording as the narrator quality reference. A
separate, non-installed candidate now contains all 1,886 available rows: 894
localization/UI and 992 story lines. It is at
`audio/polish/repair_candidate_20261013/full_revoice_intro/`; the root
`manifest.jsonl` keeps every source key, Polish text, character profile, prior
WAV path, and candidate WAV path. Narrator/Renko use the intro reference;
Barisov, Demichev, Kathryn, and Devlin retain their distinct Polish character
references. The generator reads the Polish draft field, not the old partially
English `tts_text` field, and applies explicit corrections to the lines found
truncated in the first full screen.

After regeneration, trimming the measured low-level pre-roll, and retrying 32
flagged lines in smaller sentence chunks, the full base-Whisper screen reports
1,740/1,886 `PASS_AUTOMATED_SCREEN`; 146 remain for review. There are no format,
clipping, long-silence, or possible-truncation flags after the targeted repair.
The remaining review flags are 142 content/pronunciation and 33 language /
pronunciation findings (categories overlap). They include machine-draft text,
foreign names/callsigns and voice pronunciation. ASR does not establish good
acting, translation accuracy, or pleasant sound; the full candidate still
needs focused listening and language corrections before integration.

The local generation engine is Pocket-TTS Polish 6L, not Piper. This work
creates a candidate voicepack only; it does not switch a runtime engine or
touch the game installation. Generated WAVs and QA reports remain local-only.

Generated WAVs, manifests, references, transcripts and correction data are
local-only and ignored by Git. Only this safe workflow documentation and the
generator/QA/reel tools belong in source control. Steam integration remains
owned by MAIN.
