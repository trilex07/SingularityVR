import json, statistics
from collections import defaultdict

V = [json.loads(l) for l in open('dubbing-corrections/measurements/levels_volumedetect.jsonl', encoding='utf-8')]
E = [json.loads(l) for l in open('dubbing-corrections/measurements/levels_ebur128.jsonl', encoding='utf-8')]

def num(x):
    try:
        return float(x)
    except (TypeError, ValueError):
        return None

by = defaultdict(list)
me = defaultdict(list)
for r in V:
    p, m = num(r['max_volume']), num(r['mean_volume'])
    if p is not None:
        by[r['pack']].append(p)
    if m is not None:
        me[r['pack']].append(m)

print('pack         n peak_mean peak_min peak_max rms_mean rms_min rms_max')
for p in ('story', 'barks', 'mp', 'skipped', 'intro', 'vignette', 'nllb_control'):
    a, b = by[p], me[p]
    print('%-12s %3d %9.2f %8.2f %8.2f %8.2f %8.2f %8.2f' % (
        p, len(a), statistics.mean(a), min(a), max(a),
        statistics.mean(b), min(b), max(b)))

print()
print('pack         n_ebu lufs_mean lufs_min lufs_max tp_max_over_-1')
lu = defaultdict(list)
tp = defaultdict(list)
for r in E:
    if r['integrated_lufs'] is not None:
        lu[r['pack']].append(r['integrated_lufs'])
    if r['true_peak_dbtp'] is not None:
        tp[r['pack']].append(r['true_peak_dbtp'])
for p in ('story', 'barks', 'mp', 'skipped', 'intro', 'vignette', 'nllb_control'):
    a = lu[p]
    over = sum(1 for x in tp[p] if x > -1.0)
    print('%-12s %3d %9.2f %8.2f %8.2f %3d/%d' % (
        p, len(a), statistics.mean(a) if a else float('nan'),
        min(a) if a else float('nan'), max(a) if a else float('nan'),
        over, len(tp[p])))

print()
print('--- quietest 15 files overall (volumedetect peak) ---')
rows = [(num(r['max_volume']), num(r['mean_volume']), r['pack'], r['key']) for r in V]
rows = [x for x in rows if x[0] is not None]
for p, m, pack, key in sorted(rows)[:15]:
    print('%.1f / %.1f  %s  %s' % (p, m if m else float('nan'), pack, key))
