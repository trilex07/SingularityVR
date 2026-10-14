"""NLLB EN->PL dla 268 werbalnych okrzykow + glosariusz militarny.
Wejscie: remaining-voices/bark-speech.json. Wyjscie: bark-pl.json.
Uruchomic DOPIERO po zakonczeniu gen_skipped.py (oba na CPU)."""
import os, json, re
os.environ['HF_HUB_OFFLINE'] = '1'
os.environ['TOKENIZERS_PARALLELISM'] = 'false'

BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices'
inv = json.load(open(BASE + '/bark-speech.json', encoding='utf-8'))
with open(BASE + '/bark-class.json', encoding='utf-8') as f:
    verbal = set(json.load(f)['verbal'])

GLOSSARY = {  # post-produkcja na surowym NLLB
    'Beacon': 'nadajnik', 'beacon': 'nadajnik',
    'Hive': 'rój', 'hive': 'rój',
    'TMD': 'TMD', 'E99': 'E 99', 'Renko': 'Renko',
    'Katorga-12': 'Katorga-12', 'Katorga': 'Katorga',
}

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

out = []
items = [(n, inv[n]['spoken']) for n in sorted(verbal)]
with torch.no_grad():
    for i, (name, en) in enumerate(items):
        inp = tok(en, return_tensors='pt', truncation=True, max_length=128)
        gen = model.generate(**inp, forced_bos_token_id=tok.convert_tokens_to_ids('pol_Latn'),
                             max_new_tokens=128, num_beams=3)
        pl = apply_glossary(tok.batch_decode(gen, skip_special_tokens=True)[0])
        out.append({'asset': name, 'en': en, 'pl': pl,
                    'rate': inv[name]['rate'], 'ch': inv[name]['ch'], 'dur': inv[name]['dur'],
                    'package_hint': inv[name]['package'], 'status': 'machine_draft'})
        if (i + 1) % 50 == 0:
            print('postep %d/%d' % (i + 1, len(items)), flush=True)
json.dump(out, open(BASE + '/bark-pl.json', 'w'), ensure_ascii=False, indent=1)
print('DONE', len(out), flush=True)
