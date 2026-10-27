import json, os, re
os.environ['HF_HUB_OFFLINE'] = '1'
os.environ['TOKENIZERS_PARALLELISM'] = 'false'
import ftfy

BASE = 'C:/Users/Trilex/vr-mods'
with open(BASE + '/SingularityVR-polish-integration/localization/pl/master_inventory.jsonl', 'rb') as f:
    content = f.read()
if content.startswith(b'\xff\xfe'):
    content = content[2:].decode('utf-16')
elif content.startswith(b'\xef\xbb\xbf'):
    content = content[3:].decode('utf-8')
else:
    content = content.decode('utf-8')

missing = []
for line in content.splitlines():
    if not line.strip():
        continue
    d = json.loads(line)
    if d.get('section') == 'Notes' and not ((d.get('pl') or '') and d.get('pl') != d.get('en')):
        missing.append(d)
print('notatek do tlumaczenia:', len(missing), flush=True)

from transformers import AutoTokenizer, AutoModelForSeq2SeqLM
model_id = 'facebook/nllb-200-distilled-600M'
tok = AutoTokenizer.from_pretrained(model_id, local_files_only=True)
model = AutoModelForSeq2SeqLM.from_pretrained(model_id, local_files_only=True)
model.eval()
print('model ready', flush=True)

def split_paras(text, max_chars=800):
    parts, cur = [], ''
    for para in re.split(r'\n{2,}', text):
        if len(cur) + len(para) + 2 <= max_chars:
            cur = (cur + '\n\n' + para).strip()
        else:
            if cur:
                parts.append(cur)
            if len(para) <= max_chars:
                cur = para
            else:
                # tnij po zdaniach
                for sent in re.split(r'(?<=[.!?])\s+', para):
                    if len(cur) + len(sent) + 1 <= max_chars:
                        cur = (cur + ' ' + sent).strip()
                    else:
                        if cur:
                            parts.append(cur)
                        cur = sent
    if cur:
        parts.append(cur)
    return [p for p in parts if p]

import torch
out = []
with torch.no_grad():
    for i, d in enumerate(missing):
        en = ftfy.fix_text(d.get('en') or '')
        chunks = split_paras(en)
        pls = []
        for ch in chunks:
            inp = tok(ch, return_tensors='pt', truncation=True, max_length=512)
            gen = model.generate(**inp, forced_bos_token_id=tok.convert_tokens_to_ids('pol_Latn'),
                                 max_new_tokens=512, num_beams=3)
            pls.append(tok.batch_decode(gen, skip_special_tokens=True)[0])
        rec = {'id': d['id'], 'section': d.get('section'), 'key': d.get('key'),
               'en': en, 'pl': '\n\n'.join(pls), 'status': 'machine_draft',
               'translation_model': 'facebook/nllb-200-distilled-600M'}
        out.append(rec)
        if (i + 1) % 8 == 0:
            print('postep %d/%d' % (i + 1, len(missing)), flush=True)

with open(BASE + '/SingularityVR-cutscene/audio/polish/notes_pl_patch.jsonl', 'w', encoding='utf-8') as f:
    for r in out:
        f.write(json.dumps(r, ensure_ascii=False) + '\n')
print('DONE zapisano notes_pl_patch.jsonl', flush=True)
