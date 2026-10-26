# MAIN correction of SIDE's timing plan — 2026-10-26

This supersedes the timing classification/formula in SIDE d38b2407 and MAIN
364c32b4. It does not change audio, approved identities or candidate V2 hashes.

## Actual status

702 replacements are packed in 111 PRIVATE candidate banks. They are not
installed in Steam and have not been heard in-game. 556 selected takes retain
the installed source audio. No MAIN listening or physical acceptance is claimed.
The combined V2 payload has 130 hash-verified files; controlled runtime
deployment/rollback preparation is separate from asset staging.

## Correct work partition

- 551 takes require shorter/faster source delivery: 333 above 1.85x and 218
  above 1.35x but at most 1.85x.
- 14 takes had a codec frame-parity refusal; NINE also exceed 1.35x. They
  belong in the SIDE timing list too, while codec validation remains MAIN-owned.
- Only FIVE are codec-only. Thus 551 timing + 5 codec-only = 556 retained.
- Correct excess-duration groups: 317 at most 1s, 151 from 1–2s, 83 above 2s.
  These are scheduling groups, not proof that any recording can be rescued by
  trimming silence or that it is a particular type of monologue.

`final-dubbing-timing-plan-20261026.json` schema v2 contains all 551 exact
package/asset identities, current text, budgets, overlap flags and five
codec-only identities. Text comes from the preserved installed lookup, whose
hash is recorded; speaker guesses remain guesses, not verified casting.

## Exact fitter budget

The MAIN receipt's `source_duration` is ALREADY the duration returned by
`trim_guarded`: first-to-last audible sample with up to 40ms on each side,
clamped to the original file edges. Outer silence was already removed.

The real fitter computes:

    tempo = max(1, guarded_duration / (native_duration - 0.020))
    max_guarded_duration = 1.35 * (native_duration - 0.020)

Do not subtract the 80ms guards again. If planning an unguarded speech span with
both full guards present, its conservative limit is max_guarded_duration - 0.080;
the final WAV must always pass the actual guarded-duration measurement. Internal
pauses and translation quality require a line-specific review, not an assumption
from the duration alone. Never crop speech or automatically compress all pauses.

## SIDE delivery instruction

Work ONLY on `timing_cases` in schema v2, on your own branch/worktree. Preserve
the current 702 packed replacements byte-for-byte; do not commit to MAIN's branch.
For each revised take, preserve identity/approved actor, use concise correct PL
without losing meaning, measure the guarded limit, and supply SHA256 plus honest
audition provenance. ASR/format checks are not listening approval. Return exact
replacements and a manifest; MAIN alone rebuilds and verifies the native banks.
Do not regenerate the five codec-only takes just to accommodate the encoder.
The nine overlapping failures still require a new safe-duration take.

MAIN's codec-only list:

- VI4_4205_GST
- SD_SPT1_FIRE_GRENADE1
- SD_SPT1_SEE_TARGET1
- SD_SPT3_MOVE_IN2
- SD_SPT3_SEE_TARGET4

## Reproduction and evidence

Run `tools/audio/reconcile_dubbing_timing.py` with `--build` pointing to
`docs/handoffs/final-dubbing-main-build-20261026.json`, `--lookup` pointing to the
PRIVATE preserved installed lookup, and `--out` pointing to the timing plan.
The script rejects duplicate identities and any disagreement with the production
fitter's tempo calculation. Both input hashes are recorded.

20 focused audio regressions PASS, including overlap, exact budget, contradictory
receipt rejection, and real FFmpeg mono/stereo frame/channel preservation. No
runtime C++ change or new DLL build; renderer/bridge/payload hashes remain those
of candidate V2. Steam untouched, no game launch, PSVR2 acceptance PENDING.
