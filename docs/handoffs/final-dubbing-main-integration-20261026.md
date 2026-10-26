# MAIN integration result — not full campaign acceptance

The user forwarded SIDE resolution `693f5599`. MAIN independently verified all
1258 selected file hashes, mono PCM16/24 kHz inputs and exact native identities.
The installed lookup differs from repository text; installed identities and
runtime filenames were preserved. No Steam files were written or game launched.

## Actual native build

- 116 installed banks hash-pinned and copied privately.
- 111 candidate banks independently decompressed and structurally verified.
- 702 new takes integrated; 556 selected takes retain installed audio.
- Five additional uncovered identities remain untouched; MP and Bink excluded.
- 551 retained takes need faster/shorter source delivery within native duration
  (333 require tempo >1.85; 218 exceed 1.35 but are at most 1.85).
- 14 have decoded-frame parity refusals; nine ALSO need shorter source delivery.
  Only five are codec-only: 551 timing + five codec-only = 556 retained.
- SIDE's claimed listening approval is provenance from its artifact, not a
  MAIN audition or physical acceptance. MAIN performed no listening.

`final-dubbing-main-build-20261026.json` names every retained package/asset,
source duration, native duration or native_frames/rate, and refusal reason.
This supersedes the statement that all1258 takes were ready for native banks.

## Confirmed integration correction

Installed RT99_0105_SC1 in SP_RT3_Vignette_LOC_INT is stereo 44100Hz, unlike
the other five mono vignette waves. The builder now preserves native channels
and computes frames as RawPCMDataSize/(2*channels), without rewriting metadata.

Some Vorbis encodes decode with fewer frames than submitted PCM. One bounded
retry adds only the measured missing tail silence (maximum256frames), re-encodes
the unchanged speech, and independently decodes again. Exact parity is mandatory.
This recovered22 additional takes; no speech crop or metadata adjustment.

## SIDE follow-up

Use the exact retained list, not a folder-wide regeneration. For timing cases,
derive the native budget from native_duration or native_frames/rate. Deliver
revised Polish speech fitting that budget with at most1.35x tempo and40ms guards;
preserve package/asset identity and provide source hash and audition evidence.
Do not mark all takes listened-to based on ASR, format or hash checks alone.
Codec validation remains MAIN-owned. The nine overlapping timing/codec failures
also need safe-duration SIDE takes; the five codec-only takes do not. Preserve
the currently packed702takes (offline, not in-game verified).

The corrected schema-v2 timing plan uses the actual guarded limit
`1.35 * (native_duration - 0.020)`. `source_duration` already contains the
edge-clamped40ms guards; outer silence was already removed. The prior SIDE
formula and silence-causation claim are superseded by the corrected timing
handoff and executable reconciliation tests.

## Combined candidate

Private staged manifest:
`build/next-weapon-reload/final-complete-v2-20261026/manifest.json`.
130 files:111dialogue banks +8sound-remaster banks +1native menu package +10DDS.
Renderer SHA256:23A3A768C44F1C6EB5479D899141924C4C44D5C6A73A3175A2827241105370B1.
Bridge SHA256:F58CB3A805E46C3D3F7E88137BD605AF21A8A8106C289385132F677A968E95FD.
Both x86 PE32 /O2; source38ca72f88112bfcbd26addb0d6aa4f76095aa15e.
Later commits change offline packaging/docs only, not DLL source.

26 focused audio tests PASS, real mono/stereo codec test PASS,130payload hashes
PASS. Production deploy/rollback tested only against fake Steam, including exact
texture config drift refusal. MAIN must still prepare/review the full accepted
profile/change plan before the user's next start; staged does not mean deployed.

Physical session: check native main/pause buttons with laser and PL text, current
dialogues and available weapon/TMD audio, weapon HD alpha/normals, then AgeSquare
and accepted L2/hands/black-artifact behavior. Seeker only if legitimately owned.
Unvisited music/wind/dialogue events remain NOT TESTED. Any regression stops the
candidate; restore only after game exit with hash verification.
