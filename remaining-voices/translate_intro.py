"""NLLB EN->PL dla 11 linii narracji intra (INTRO_*)."""
import os, json
os.environ['HF_HUB_OFFLINE'] = '1'
os.environ['TOKENIZERS_PARALLELISM'] = 'false'
BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices'
inv = json.load(open(BASE + '/intro-speech.json', encoding='utf-8'))
items = [r for r in inv if r['asset'].startswith('INTRO_')]
print('linii:', len(items), flush=True)
from transformers import AutoTokenizer, AutoModelForSeq2SeqLM
import torch
tok = AutoTokenizer.from_pretrained('facebook/nllb-200-distilled-600M', local_files_only=True)
model = AutoModelForSeq2SeqLM.from_pretrained('facebook/nllb-200-distilled-600M', local_files_only=True)
model.eval()
out = []
with torch.no_grad():
    for r in items:
        inp = tok(r['spoken'], return_tensors='pt', truncation=True, max_length=256)
        gen = model.generate(**inp, forced_bos_token_id=tok.convert_tokens_to_ids('pol_Latn'),
                             max_new_tokens=256, num_beams=3)
        pl = tok.batch_decode(gen, skip_special_tokens=True)[0]
        out.append({'asset': r['asset'], 'en': r['spoken'], 'pl': pl,
                    'rate': r['rate'], 'ch': r['ch'], 'dur': r['dur'], 'status': 'machine_draft'})
        print(r['asset'], '->', pl[:80], flush=True)
json.dump(out, open(BASE + '/intro-pl.json', 'w'), ensure_ascii=False, indent=1)
print('DONE', flush=True)
