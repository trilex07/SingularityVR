"""NLLB EN->PL: okrzyki werbalne (268) + MP (897). Uruchamiac PO zakonczeniu gen_skipped (CPU).
Wyjscie: bark-pl.json / mp-pl.json (asset + en + pl + głosariusz)."""
import os, json, re, sys
os.environ['HF_HUB_OFFLINE'] = '1'
os.environ['TOKENIZERS_PARALLELISM'] = 'false'

BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices'
mode = sys.argv[1] if len(sys.argv) > 1 else 'barks'
assert mode in ('barks', 'mp')

GLOSSARY = {'Beacon': 'nadajnik', 'beacon': 'nadajnik', 'Hive': 'rój', 'hive': 'rój',
            'TMD': 'TMD', 'E99': 'E 99', 'Renko': 'Renko', 'Katorga-12': 'Katorga-12',
            'Katorga': 'Katorga', 'Motherland': 'ojczyzna', 'motherland': 'ojczyzna'}

if mode == 'barks':
    inv = json.load(open(BASE + '/bark-speech.json', encoding='utf-8'))
    verb = set(json.load(open(BASE + '/bark-class.json', encoding='utf-8'))['verbal'])
    items = [{'asset': n, 'en': inv[n]['spoken'], 'rate': inv[n]['rate'],
              'ch': inv[n]['ch'], 'dur': inv[n]['dur'], 'pkg': inv[n]['package']} for n in sorted(verb)]
    outp = BASE + '/bark-pl.json'
else:
    inv = json.load(open(BASE + '/mp-speech.json', encoding='utf-8'))
    items = [{'asset': r['asset'], 'en': r['spoken'], 'rate': r['rate'],
              'ch': r['ch'], 'dur': r['dur'], 'pkg': r['package']} for r in inv
             if len((r['spoken'] or '').split()) >= 2]
    outp = BASE + '/mp-pl.json'
print(mode, 'zadan:', len(items), flush=True)

from transformers import AutoTokenizer, AutoModelForSeq2SeqLM
import torch
tok = AutoTokenizer.from_pretrained('facebook/nllb-200-distilled-600M', local_files_only=True)
model = AutoModelForSeq2SeqLM.from_pretrained('facebook/nllb-200-distilled-600M', local_files_only=True)
model.eval()
print('model ready', flush=True)

def apply_glossary(t):
    for en, pl in GLOSSARY.items():
        t = re.sub(r'\b' + re.escape(en) + r'\b', pl, t)
    return t

done = set()
if os.path.exists(outp):
    try:
        for r in json.load(open(outp, encoding='utf-8')):
            done.add(r['asset'] + '|' + r.get('pkg', ''))
    except Exception:
        pass
out = []
if os.path.exists(outp):
    try:
        out = json.load(open(outp, encoding='utf-8'))
    except Exception:
        out = []
with torch.no_grad():
    for i, it in enumerate(items):
        if it['asset'] + '|' + it.get('pkg', '') in done:
            continue
        inp = tok(it['en'], return_tensors='pt', truncation=True, max_length=128)
        gen = model.generate(**inp, forced_bos_token_id=tok.convert_tokens_to_ids('pol_Latn'),
                             max_new_tokens=128, num_beams=3)
        pl = apply_glossary(tok.batch_decode(gen, skip_special_tokens=True)[0])
        out.append({'asset': it['asset'], 'en': it['en'], 'pl': pl, 'pkg': it.get('pkg'),
                    'rate': it['rate'], 'ch': it['ch'], 'dur': it['dur'], 'status': 'machine_draft'})
        if (i + 1) % 50 == 0:
            json.dump(out, open(outp, 'w'), ensure_ascii=False, indent=1)
            print('postep %d/%d' % (i + 1, len(items)), flush=True)
json.dump(out, open(outp, 'w'), ensure_ascii=False, indent=1)
print('DONE', len(out), flush=True)
