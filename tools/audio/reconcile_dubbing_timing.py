"""Reconcile retained MAIN takes against the fitter's actual guarded budget.

Offline only. Never edits audio, native banks, the installed lookup or Steam.
The input source_duration is ALREADY trimmed with up to 40 ms on either side.
Codec and timing failures can overlap; never classify only by first refusal.
"""
import argparse
import hashlib
import json
import math
from collections import Counter
from pathlib import Path

from native_voice_fit import MAX_DEPLOYMENT_TEMPO, native_tempo


def reconcile(build, lookup):
    by_id = {}
    for row in lookup:
        key = (row['package'].casefold(), row['asset'])
        if key in by_id:
            raise ValueError('duplicate lookup identity: ' + str(key))
        by_id[key] = row
    timing, codec, codec_only = [], [], []
    seen = set()
    for row in build['retained']:
        key = (row['package'].casefold(), row['asset'])
        if key in seen:
            raise ValueError('duplicate retained identity: ' + str(key))
        seen.add(key)
        lk = by_id[key]
        budget = row.get('native_duration')
        if budget is None:
            budget = row['native_frames'] / row['rate']
        guarded = row['source_duration']
        tempo = native_tempo(guarded, budget)
        if not math.isclose(tempo, row['tempo'], rel_tol=1e-9, abs_tol=1e-9):
            raise ValueError('fitter/receipt timing contradiction: ' + str(key))
        is_timing = tempo > MAX_DEPLOYMENT_TEMPO
        is_codec = 'encoded native PCM budget mismatch' in row.get('reason', '')
        if not is_timing and not is_codec:
            raise ValueError('unclassified retained take: ' + str(key))
        # Invert the exact fitter formula. Do NOT subtract the 80 ms guards
        # a second time: guarded already contains them, clipped at file edges.
        max_guarded = MAX_DEPLOYMENT_TEMPO * (budget - .02)
        entry = dict(row, speaker=lk.get('speaker_guess', 'unknown'),
                     profile=lk.get('profile', 'unknown'),
                     text_en=lk.get('text_en', ''), text_pl=lk.get('text_pl', ''),
                     tts_text=lk.get('tts_text', ''), native_duration=budget,
                     source_duration_kind='trim_guarded_including_existing_guards',
                     max_allowed_source_duration=max_guarded,
                     duration_overshoot_s=max(0., guarded - max_guarded),
                     needs_shorter_take=is_timing, codec_also_pending=is_codec)
        if is_timing:
            timing.append(entry)
        if is_codec:
            codec.append(entry)
            if not is_timing:
                codec_only.append(entry)
    groups = Counter('under_1s' if r['duration_overshoot_s'] <= 1. else
                     '1_to_2s' if r['duration_overshoot_s'] <= 2. else 'over_2s'
                     for r in timing)
    return dict(schema='singularityvr-dubbing-timing-plan-v2',
                evidence_level='OFFLINE_ONLY_NOT_DEPLOYED_NOT_AUDITIONED_BY_MAIN',
                summary=dict(total_retained=len(seen), timing_cases=len(timing),
                             codec_cases=len(codec),
                             timing_and_codec_cases=len(codec)-len(codec_only),
                             codec_only_cases=len(codec_only),
                             tempo_over_185=sum(r['tempo'] > 1.85 for r in timing),
                             tempo_135_to_185=sum(r['tempo'] <= 1.85 for r in timing),
                             overshoot_under_1s=groups['under_1s'],
                             overshoot_1_to_2s=groups['1_to_2s'],
                             overshoot_over_2s=groups['over_2s']),
                budget=dict(max_tempo=MAX_DEPLOYMENT_TEMPO,
                            native_tail_reserve_s=.02, trim_guard_per_side_s=.04,
                            guarded_limit_formula='1.35 * (native_duration - 0.02)',
                            guards_already_in_source_duration=True,
                            silence_cause_proven=False),
                timing_cases=timing, codec_cases=codec, codec_only_cases=codec_only)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=Path, required=True)
    ap.add_argument('--lookup', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    build = json.loads(args.build.read_text(encoding='utf-8-sig'))
    lookup = [json.loads(s) for s in args.lookup.read_text(encoding='utf-8-sig').splitlines() if s.strip()]
    result = reconcile(build, lookup)
    result['input_sha256'] = dict(build=hashlib.sha256(args.build.read_bytes()).hexdigest(),
                                  lookup=hashlib.sha256(args.lookup.read_bytes()).hexdigest())
    args.out.write_text(json.dumps(result, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(json.dumps(result['summary']))


if __name__ == '__main__':
    main()
