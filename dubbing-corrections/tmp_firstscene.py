import json
rows = [json.loads(l) for l in open(
    r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\localization\pl\polish_story_voice.jsonl',
    encoding='utf-8') if l.strip()]
want = [r for r in rows if r['package'].lower().startswith(
    ('sp_rt1', 'sp_freight_script_train', 'sp_vi_intro'))]
from collections import Counter
print('n=%d' % len(want))
print(Counter(r['package'] for r in want))
for r in want:
    print('%s | %s | %s | %s' % (
        r['package'], r['asset'], r.get('speaker_guess'), r['text_en'][:110]))
