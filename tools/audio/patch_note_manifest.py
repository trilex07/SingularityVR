"""Offline exact-key/text-hash patch of first-area notes. Never writes Steam.

Translations contain no original asset; English text must come from the user's
own manifest and match its pinned UTF8 SHA. Other entries stay byte-identical.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path


def patch(source, translations):
    patches = {r['key']: r for r in translations}
    if len(patches) != len(translations): raise ValueError('duplicate patch key')
    seen = set()
    out = []
    for line in source.splitlines(keepends=True):
        if not line.strip(): out.append(line); continue
        row = json.loads(line)
        key = row.get('key')
        if row.get('section') != 'Notes' or key not in patches:
            out.append(line); continue
        if key in seen: raise ValueError('duplicate source Notes key')
        p = patches[key]
        if hashlib.sha256(row['en'].encode('utf-8')).hexdigest() != p['source_sha256']:
            raise ValueError('source text changed: '+key)
        if not p['pl'] or p['pl'] == row['en'] or len(p['pl']) > 4095:
            raise ValueError('invalid bounded translation')
        token = lambda s: re.findall(r'\[[^\]]+\]', s)
        if token(row['en']) != token(p['pl']): raise ValueError('input tokens changed')
        if row['en'].count('\n\n') != p['pl'].count('\n\n'):
            raise ValueError('paragraph boundaries changed: '+key)
        row.update(pl=p['pl'], status='translated', note='First-area complete human text review; native delivery not yet physically accepted.')
        out.append(json.dumps(row, ensure_ascii=False)+'\n')
        seen.add(key)
    if seen != set(patches): raise ValueError('missing source Notes keys')
    return ''.join(out)


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--source', type=Path, required=True)
    ap.add_argument('--translations', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    if args.out.exists(): raise ValueError('refuse existing output')
    data = patch(args.source.read_text(encoding='utf-8-sig'), json.loads(args.translations.read_text(encoding='utf-8')))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(data, encoding='utf-8')
    print('PASS: pinned first-area Notes manifest; no Steam writes')
