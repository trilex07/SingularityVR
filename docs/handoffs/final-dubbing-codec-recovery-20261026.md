# MAIN: five short-file codec refusals recovered

Evidence level: offline codec/container PASS; Steam, native playback and physical
listening NOT RUN. SIDE 004e3051 synchronized the timing plan but delivered no
new recordings. Its 551-take work list is unchanged.

## Proven mechanism and bounded correction

All five safe-duration refused encodes had exactly one audio page, also EOS.
The installed native originals had multiple audio pages. FFmpeg's two Vorbis
decoders returned an incorrect frame count for the candidates; packet timestamps
started at zero rather than the native -128, with some final duration underflows.
Both decoders share Ogg demuxing: their agreement is not fully independent proof.

Re-encoding the SAME padded PCM at the SAME quality with Ogg page_duration=200000
instead of the default 1000000 produced byte-identical compressed audio packets,
an unchanged EOS granule and exactly the native PCM frame count in both decoders.
This demonstrates a packaging/demux timing problem for these local short files,
not a need to shorten speech or change native duration metadata.

The fitter retries this route only after failed parity on a single audio page.
It requires identical compressed packets and exact decoded format/frame parity.
Otherwise the existing bounded retry/refusal remains. No speech crop, speed
change, extra tail padding, granule rewrite or native property change was used
for these five recoveries.

## Recovery identities

- SP_VI5_kismet01_LOC_INT: VI4_4205_GST, 48000 Hz / 37745 frames.
- SP_VI5_kismet03_LOC_INT: SD_SPT1_FIRE_GRENADE1, 44100 Hz / 28156 frames;
  SD_SPT1_SEE_TARGET1, 44100 / 43839; SD_SPT3_MOVE_IN2, 44100 / 44964;
  SD_SPT3_SEE_TARGET4, 44100 / 42183. All mono.

Only those two private V2 candidate banks changed. All previously packed 702
takes remain unchanged. The native package builder verifies all properties,
other platform payloads and non-target exports, followed by a separate container
decompression/byte comparison. Committed JSON records bank and packet hashes.

## Result and remaining work

V3: 707 replacements, 551 retained source recordings; 111 changed banks out of
116 checked. The combined payload still contains 130 files: 111 dialogue banks,
eight sound-remaster banks, one menu package and ten DDS. V2 remains preserved.

Nine retained codec refusals overlap the 551 timing cases and need shorter SIDE
recordings before codec revalidation. There are zero remaining codec-only cases.
Five additional uncovered native identities remain outside the selected pack.
This is partial coverage, not full Polish campaign acceptance.

## Reproduction

Use tools/audio/recover_native_codec_cases.py with the pinned V2 manifest,
the V2 report/timing plan, private staged source WAVs, FFmpeg and the existing
UE3 container decoder. Output must be a new directory under this checkout/build.
The script does not install files. Private output is
build/next-weapon-reload/final-complete-v3-20261026; audit originals/packet timing
are in build/audio-release-private/codec-audit-20261026.

21 focused Python tests PASS, including a real FFmpeg single-page reproduction
at 48/44.1 kHz, unchanged packet comparison, and both decoder frame counts.
Five actual-source recoveries and two native bank round trips PASS. Production
ValidateCandidate verified 130 payload hashes. No C++ change: the clean x86 /O2
renderer/bridge pair remains the build from source 38ca72f8.
