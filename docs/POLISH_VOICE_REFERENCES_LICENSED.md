# Licensed voice references (PL / RU / UK) — 2026-10-14

This note is for **planning new character voices**, not for replacing the
user-approved intro narrator or the restored UI lektor. Do not treat public
recordings of real people as consent for cloning.

## Project-local, already authorized

| Source | Link | License / consent | AI clone / generate | Commercial | Attribution | Notes |
|---|---|---|---|---|---|---|
| User intro (`SNG_intro_polish_voice.wav`) | local file (SHA-256 `b97fcfee…dac70`) | User-owned recording supplied for this mod | Yes — explicit project use | Per user grant to project | N/A (author is licensor) | Quality reference for **intro narration**; not a blanket license for unrelated voices |
| User `spokoj_reference.wav` | `audio/polish/user_reference/spokoj_reference.wav` | User-owned reference for Pocket-TTS | Yes — used in `full_voicepack` / UI lektor restore | Per user grant | N/A | **Previous menu/loading lektor** conditioning sample |
| Pocket-TTS Polish 6L | [shefowl/pocket-tts-polish-6l](https://huggingface.co/shefowl/pocket-tts-polish-6l) | **CC-BY-4.0** (inherits [kyutai/pocket-tts](https://huggingface.co/kyutai/pocket-tts)) | Yes — zero-shot from reference WAV | Allowed with attribution | Credit model + upstream Kyutai terms | Upstream **prohibited use** text: no impersonation without lawful consent; do not present synth as genuine recordings |
| Bundled CML-TTS reference in Polish 6L card | Same HF repo (`reference.wav`) | **CC-BY-4.0** (CML-TTS) | Yes for that bundled clip only | Allowed with attribution | Per CML-TTS / model card | Neutral demo voice — **not** used for Singularity characters |

## Piper / OpenVoice-style checkpoints (local synthesis anchors)

Engine: [OHF-Voice/piper1-gpl](https://github.com/OHF-Voice/piper1-gpl) (**GPL-3.0**). Voice **weights** have per-voice terms on Hugging Face (`rhasspy/piper-voices`). Cloning in the ML sense is N/A (parametric TTS); **generating new speech from the checkpoint is allowed only if that voice’s MODEL_CARD permits your use case.**

| Voice | Link | Typical license flag | AI generate new speech | Commercial | Attribution / limits |
|---|---|---|---|---|---|
| pl_PL **mc_speech** medium | [HF piper-voices](https://huggingface.co/rhasspy/piper-voices/tree/main/pl/pl_PL/mc_speech/medium) | Check MODEL_CARD | Yes (TTS inference) | Often allowed — verify card | Per voice card |
| pl_PL **gosia** medium | Same tree | Check MODEL_CARD | Yes | Verify card | Per voice card |
| pl_PL **darkman** medium | Same tree | Often **personal / non-commercial** in community summaries | Yes locally | **Review before ship** | Project already used Darkman only where matched to Demichev profile |
| ru_RU **dmitri**, **ruslan**, **irina**, **denis** | [ru_RU voices](https://huggingface.co/rhasspy/piper-voices/tree/main/ru/ru_RU) | Per MODEL_CARD | Yes (inference) | **denis/darkman often NC** — verify each | Used only as **Polish dialogue accent anchors** via Pocket conditioning in this project |
| uk_UA **lada**, **ukrainian_tts** | [uk_UA voices](https://huggingface.co/rhasspy/piper-voices/tree/main/uk/uk_UA) | Per MODEL_CARD | Yes (inference) | Verify card | Same accent-anchor pattern as above |

Catalog index: [rhasspy/piper VOICES.md](https://github.com/rhasspy/piper/blob/master/VOICES.md).

## Datasets (training) — legal caution

| Source | Link | License | Train / clone | Commercial | Notes |
|---|---|---|---|---|---|
| Mozilla Common Voice | [Terms](https://github.com/mozilla/legal-docs/blob/main/en/common_voice_terms.md) / [MDC](https://mozilladatacollective.com/) | **CC0-1.0** | Legally permitted; Mozilla asks not to identify speakers | Generally yes under CC0 | **Policy** discourages voice replication; CC0 cannot add a hard TTS ban ([discussion](https://discourse.mozilla.org/t/explicitly-forbidding-limiting-tts-usage/115072)). Not a substitute for recorded consent for recognizable clones. |
| Kyutai pre-made voices | [kyutai/tts-voices](https://huggingface.co/kyutai/tts-voices) | Per-voice page | Precomputed embeddings for Pocket-TTS | Per voice | Use voice pages linked from [Pocket README](https://huggingface.co/kyutai/pocket-tts/blob/main/README.md) |

## Not approved for this project

- Cloning **recognizable deceased or living public figures** (e.g. celebrity narrators) without documented rights holder consent for synthesis.
- Random YouTube / game rips / broadcast archives as “reference.”
- **Coqui XTTS** and similar models whose license is **non-commercial only**, unless the task explicitly moves to a NC distribution (this mod targets a shippable voicepack — treat NC models as research-only).

## Recommended direction for new PL/RU/UK **character** identities

1. **Fictional targets**: write a brief casting note (age, register, accent strength) and generate from Piper + Pocket **license-clean** anchors, as this repo already does for Barisov/Demichev/Kathryn/Devlin.
2. **User-owned short references** (10–30 s clean speech) remain the strongest legal basis for Pocket-TTS cloning.
3. For Russian/Ukrainian **accent in Polish dialogue**, keep using distinct Piper-derived anchor WAVs; do not merge into one “Eastern European” voice.
