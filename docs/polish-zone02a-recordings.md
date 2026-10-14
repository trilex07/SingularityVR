# Polish first-room recordings — SIDE offline handoff

Branch `feature/polish-zone02a-voices`, based on accepted main `8751a27`.
MAIN owns native packaging, installation and real Steam/PSVR2 validation. This
work changes no renderer, input, installed asset, accepted intro or radio file.

## Observation and hypothesis

User heard English indoor recordings, including research laboratories, after
accepted Polish opening radio. Earlier native delivery covered intro and three
kismet banks, not these two room banks. Original `SpokenText` proves the older
Polish laboratory text omitted weapons, crops, medicine and chronology topics;
other room entries also omit clauses, reverse meaning or confuse identities.
Presence/ASR success of an external WAV never established native playback.

Hypothesis: these unchanged native room payloads still supply English. Minimum
controlled experiment belongs to MAIN: source-pinned two-bank replacement,
unchanged properties/cues and independently decoded original sample budgets.
Falsifier: both candidates demonstrably load but the same recording is English;
then prove the actual cue/wave/localization route instead of expanding blindly.

## Scope and evidence

Installed read-only inventory contains exactly two `SP_VI2_zone02a*LOC_INT`
banks. The base bank has eight waves; its2010 counterpart eleven. Nineteen
identities contain13 unique speech assets, with six identical announcements.
Do not collapse1520 future Barisov with1525 completed1955/present Demichev,
or1205 Demichev died with1210 Demichev survived. Both timelines remain distinct.

Private inventory binds physical bank SHA, logical SHA, complete native text,
exact `SpokenText` value offset/size/hash, rate/frame budget and previous source
translation/audio hash. All original assets/transcripts/translated scripts and
generated audio stay in ignored private output, never in this repository diff.
The standalone GPL chunk decoder is not linked into the mod.

The delivery has13 mono PCM16/24k WAVs:11 new and two reused source-reviewed
PA recordings. Generation repairs nine older announcement/narrative translations
and the missing short1505 phrase. An additional complete9015 take replaces a
proposed reuse whose ASR omitted element/enrichment; that ASR was a diagnostic,
not proof of an audible defect. The reusable9005/9010 source WAVs were retained
byte-for-byte, with new room identities independently bound to native evidence.

The existing installed offline Pocket-TTS Polish6L and narrator profile were
reused, without new downloads/dependencies/network TTS or full-pack generation.
Explicit complete sentences protect against premature model EOS. Four first
takes required short-sentence alternatives because ASR did not reproduce energy
safety, confidence or growing-funds clauses. Source translations remained full;
no native timing/property/cue was changed to conceal missing content.

Guarded contiguous trim uses the previously tested40ms edge preservation, no
interior cut. Maximum selected tempo is1.411, below1.85. All13 final clips pass
format/hash/timing and the conservative final-fit ASR screen. Full topic and
timeline clauses appear in selected ASR. ASR cannot approve acting, emotion,
pronunciation, voice identity or native behavior; all auditory/runtime gates
remain pending. Five synthetic provenance/format/budget/segment/binding tests
and syntax/diff checks pass. No L3/L4 success is claimed.

## Integration handoff

Private delivery: `deployment/hands-dubbing/zone02a-delivery/`:
`selected.json` (13 exact current WAV identities), `native-bindings.json`
(19 source-bound replacements), `source-banks.json` and
`native-encoding-gates.json`. Exact current WAV identity is `sha256` and
`selected_wav_sha256`; previous/generated input hashes are provenance only.
Complete source EN/PL and QA details are private binding fields.

MAIN must independently verify physical/logical/property/WAV hashes, preserve
all native properties and cues, and require exact independently decoded mono48k
sample counts after Vorbis encoding. Nominal encoder duration alone does not
pass; reject mismatches rather than guessing codec delay or cropping speech.
Original source files were read-only and rehashed after preparation. Existing
intro/radio candidates remain separate and unmodified.
