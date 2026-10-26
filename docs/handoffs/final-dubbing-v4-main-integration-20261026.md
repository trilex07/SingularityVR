# MAIN: 551-take delivery integrated offline with ten content refusals

Status: **offline audio packaging PASS; deployment NO-GO**. The existing V3
startup crash is unresolved. No Steam writes or game launches in this delivery.
Installed AB348E06 rollback profile: all 159 file pins independently verified.

## Delivered result

- SIDE source: `c5b56ad1e21d1cccc8df4a0e38e6c6c8d68f5b59`, branch
  `side/final-audio-20261026`.
- Source delivery manifest SHA256:
  `9a461523d460d72bc1e06f40f2a7d5a8ea7338c2af79a58f87b6002b1689d11b`.
- MAIN checked actual PCM/hash/format/peak/guarded duration/tempo for **551/551**.
  Do not equate this result with correct translation, performance or listening.
- **541 new takes** packed; **707 existing PC audio payloads preserved**,
  including the five earlier codec recoveries. Total **1248** replacements
  within the existing 1258-identity native-bank scope. This is not a claim of
  complete game dialogue coverage.
- **Ten content refusals retained their base audio**, verified independently in
  final logical banks. They omit essential instructions or reverse "go" to
  "stop". Exact identities/filenames/text/hashes/budgets are in
  `final-dubbing-551-main-review-20261026.json`; copyable SIDE task is in
  `side-timing-551-content-corrections-20261026.md`.
- **Zero new codec refusals.** 57 changed banks independently unpacked and
  compared with expected rebuilt bytes. Native sample rate, channels, frame
  budget, wave properties, non-PC data and non-target exports preserved.
- All 116 campaign banks now contain at least one replacement. Together with
  unchanged eight sound banks, menu package and ten DDS files: **135 assets**.
  The four runtime/config files and staging metadata are identical to V3.

Private output, relative to this worktree:

`build/next-weapon-reload/final-complete-v4-audio-verified-20261026/`

Manifest SHA256:
`17ade647497964008c216ba2684090df326e01c3c37a99533f3398e0c3f185b7`.

Exact receipts and runtime hashes:
`docs/handoffs/final-dubbing-v4-main-integration-20261026.json`.
Detailed private per-bank results: `timing-delta.json`, `combined-report.json`,
`delivery-audit.json`, and `timing-delta/<package>/base.u` / `roundtrip.u`.
Audio, native banks and extracted assets remain ignored/private.

## Tool and verification

`tools/audio/integrate_timing_delivery.py` consumes the pinned V3 manifest,
reviewed 551-delivery manifest, MAIN timing plan/content exclusions and native
source snapshot. It refuses identity drift, duplicate/aliased paths, altered
source banks or previous takes. It recomputes timing instead of trusting SIDE's
PASS fields, fits with the existing pipeline and requires native PCM-frame
parity from FFmpeg's Vorbis decoder and independent libvorbis decoding.

The correct existing decoder for these internally compressed v584 packages is
`tools/uedecompress/unpack_chunks.exe`, built from `unpack_chunks.cpp` with MSVC
x86 `/O2 /std:c++17 /EHsc`. `uedecompress.exe` is for whole-file compression;
using it here was refused before any bank write. Two earlier incomplete private
attempt directories remain evidence, not candidates.

Reproduction inputs to the CLI (use a fresh `--out` under
`build/next-weapon-reload`; it never overwrites a candidate):

- `--base build/next-weapon-reload/final-complete-v3-20261026`
- `--base-manifest-sha256 419975c167fcda2519866c5bbf4b45da6da2f9701ac1a65717e2db2cf640d97e`
- `--delivery ../SingularityVR-final-audio/docs/handoffs/final-dubbing-v3-manifest-20261026.json`
- `--wav-dir ../SingularityVR-final-audio/build/audio-release-private/timing_fix_551`
- `--plan docs/handoffs/final-dubbing-timing-plan-20261026.json`
- `--review docs/handoffs/final-dubbing-551-main-review-20261026.json`
- `--source-profile build/audio-release-private/campaign-20261026/source-profile.json`
- `--source-profile-sha256 01C40D801797672858B306D9A48F6C5D1A00C3A6F5F4953C7E4FE41089310732`
- `--runtime-receipt docs/release/final-session-readiness-20261026.json`
- `--decoder tools/uedecompress/unpack_chunks.exe`
- `--ffmpeg` points to the existing private FFmpeg executable; the actual run
  used `../SingularityVR-polish-integration/tools/ffmpeg/ffmpeg-9.0.2-essentials_build/bin/ffmpeg.exe`.
- `--cooked` and `--lookup` point to the pinned installed `RvGame/CookedPC` and
  `Binaries/vr-assets/localization/pl/polish_story_voice.jsonl`. Read-only inputs.

Focused audio regression suite: **19/19 PASS**, including real mono/stereo
encoding, byte-identical speech packets after single-page repaging, exact bank
identity, timing/hash/PCM corruption refusals and content exclusions bound to
the reviewed take. All 551 actual files also rechecked with the final verifier.

No C++ runtime source changed; no new renderer build is claimed. Reused V3
x86 PE32 `/O2` renderer SHA256:
`23A3A768C44F1C6EB5479D899141924C4C44D5C6A73A3175A2827241105370B1`.
It has an unresolved live startup FAIL; this audio work does not repair it.

## Remaining gates

1. SIDE corrects the ten meaning failures and returns an exact delta manifest.
   Actual SIDE synthesis code loads Pocket-TTS Polish 6L, not XTTS. No per-file
   generator witness was independently observed; do not relabel its output.
2. MAIN isolates and fixes the V3 startup failure before another headset request.
   See `final-v3-startup-failure-20261026.md`; allocator corruption's first writer
   remains unproven. No inference that these new WAVs caused the earlier crash.
3. Before eventual deployment, create a fresh combined readiness/backup profile:
   five additional native banks now change. V3's 134-source guard/profile must
   not be reused as a V4 deployment receipt. This output has no armed guard.
4. Only then perform grouped in-game listening plus UI/material/gameplay checks.
   No listening, creative approval, physical PASS or new stable baseline has
   been established by these offline tests.
