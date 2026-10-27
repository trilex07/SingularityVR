# Polish voice profile count

Inventory generated from every `SP_*_LOC_INT.xxx` package with UModel's
`-list -sounds` mode on 2026-10-08.

## What the asset names show

- 13,502 `SoundNodeWave` entries were indexed.
- 1,295 unique named wave objects were found after duplicate references were
  collapsed.
- 1,260 story-style waves use a scene/line prefix such as `VI`, `RT`, `RL`,
  `SL`, `WL` or `FR` and a speaker code at the end of the name.
- 12,242 `SD_*` waves are generic barks, combat calls, pain, movement and
  enemy voice variants. They are counted separately because they should be
  covered by a small pool of generic profiles rather than one voice per file.

The detailed machine-readable lists are:

- `inventory/story_voice_profile_candidates.csv`
- `inventory/generic_voice_profile_candidates.csv`

## Minimum practical set

The game credits identify four principal speaking characters: James Devlin,
Nikolai Demichev, Kathryn Norvikova and Viktor Barisov. Renko is the playable
protagonist and is effectively silent in the shipped game. The credits also
list additional voice talent, so the complete campaign needs more than the
four principal profiles. [Windows credits](https://www.mobygames.com/game/47109/singularity/credits/windows/)

For a coherent Polish campaign I recommend **12 profiles initially**:

1. Barisov — MarekNeural (already selected).
2. Demichev — Piper Darkman (already selected).
3. Kathryn — new female Polish profile.
4. Devlin — new male American/command profile.
5. Narrator/announcer — ZofiaNeural (already selected).
6. Young/alternate Barisov (`BAR`/`BARY`) — reuse Barisov with a lighter
   setting unless listening proves the variant needs a separate voice.
7. Young/alternate Demichev (`DEM`/`DEMY`) — reuse Demichev with a lighter
   setting unless the scene requires a separate identity.
8. Soviet soldier / Spetsnaz A (`SPT1`).
9. Soviet soldier / Spetsnaz B (`SPT2`).
10. Soviet soldier / Spetsnaz C (`SPT3`).
11. Scientist / civilian male pool (`SC`, `CM`, `WRK`, `GRD`).
12. Civilian/female/child/PA pool (`CF`, `KID`, `PA`, `ANN` alternates).

This is a production voice-profile count, not a claim that there are only 12
characters in the story. Additional codes such as `BER`, `GST`, `NIM`, `PET`,
`IVN`, `KKN`, `NAD`, `TCH` and `OKN` need an audition of their actual audio
before deciding whether they are named characters, scene roles or reused
generic voices.

## Recommended expansion after audition

The likely final set is **15–18 profiles** if the minor named speakers are
kept distinct. Do not create 28 separate voices just because there are 28
speaker codes: several codes are time/scene variants, and many `SD_*` assets
are non-dialogue barks.

The next step is to export one representative OGG for each candidate code,
transcribe or listen to it, and assign one of the following actions:

- `reuse`: same Polish profile, different performance direction;
- `new_profile`: a distinct named character;
- `generic_pool`: one of the soldier/scientist/civilian pools;
- `non_dialogue`: preserve the original sound or process as a creature/effect.

Your own recorded voice can be tested with Pocket-TTS as a separate
experimental profile. The model card says it supports Polish reference
conditioning and asks that voice cloning/impersonation be used only with
explicit lawful consent, so we will use it for your own voice or a fictional
new profile, not for copying the original actors. [Pocket-TTS Polish model](https://huggingface.co/shefowl/pocket-tts-polish-6l)
