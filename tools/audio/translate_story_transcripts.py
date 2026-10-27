from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.translate_tools')
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
from transformers import MarianMTModel, MarianTokenizer

ROOT = Path(r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene')
IN = ROOT / 'audio' / 'story_en_reference' / 'story_transcripts.jsonl'
OUT = ROOT / 'audio' / 'story_en_reference' / 'story_transcripts_pl.jsonl'
MODEL_NAME = 'Helsinki-NLP/opus-mt-en-ine'


def main() -> None:
    rows = [json.loads(x) for x in IN.read_text(encoding='utf-8').splitlines() if x.strip()]
    rows = [r for r in rows if r.get('text_en')]
    done = set()
    if OUT.exists():
        for line in OUT.read_text(encoding='utf-8').splitlines():
            if line.strip():
                item = json.loads(line)
                done.add(item['path'])
    rows = [r for r in rows if r['path'] not in done]
    print('loading', MODEL_NAME, flush=True)
    tokenizer = MarianTokenizer.from_pretrained(MODEL_NAME)
    model = MarianMTModel.from_pretrained(MODEL_NAME)
    model.eval()

    with OUT.open('a', encoding='utf-8') as out:
        for start in range(0, len(rows), 8):
            batch = rows[start:start + 8]
            inputs = ['>>pol<< ' + r['text_en'] for r in batch]
            encoded = tokenizer(inputs, return_tensors='pt', padding=True, truncation=True, max_length=384)
            generated = model.generate(**encoded, max_new_tokens=160, num_beams=4)
            translated = tokenizer.batch_decode(generated, skip_special_tokens=True)
            for r, pl in zip(batch, translated):
                item = dict(r)
                item['text_pl_machine'] = pl.strip()
                item['translation_model'] = MODEL_NAME
                item['translation_status'] = 'machine_draft'
                out.write(json.dumps(item, ensure_ascii=False) + '\n')
            out.flush()
            print(f'translated {min(start + len(batch), len(rows))}/{len(rows)}', flush=True)
    print('complete', flush=True)


if __name__ == '__main__':
    main()
