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
A/B reel pairs each affected profile's prompt reference with every flagged
candidate and one unflagged example per profile/pack, with previous and new
recordings adjacent. Perceptual approval still requires actual listening.

## Current local candidate

The selected full set is staged at
`audio/polish/repair_candidate_20261013/full_audit_pocket_selected/` (205 WAVs,
24 kHz mono PCM16). It selects the strongest of three same-profile Pocket-TTS
generation variants per line using transcript similarity and word coverage.
No technical WAV flags remain. On the selected set, faster-whisper `base`
passed 171/205 and marked 34 for review; the independent `small` screen passed
167/205 and marked 38. These are review flags, not a subjective quality grade.

The A/B reel is
`audio/polish/repair_candidate_20261013/pocket_voice_ab_review_20261013.wav`
(about 6m22s); its `.md` timeline and `.json` index identify profile references,
all rows flagged by either ASR model, and one clean example per profile/pack.
Per-file profile references and generation settings are recorded in the local
`full_audit_pocket_selected/manifest.jsonl`.

Generated WAVs, manifests, references, transcripts and correction data are
local-only and ignored by Git. Only this safe workflow documentation and the
generator/QA/reel tools belong in source control. Steam integration remains
owned by MAIN.
