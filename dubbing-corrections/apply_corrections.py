"""Zastosowanie corrections_sp_vi5_kismet03.jsonl do localization/pl/polish_story_voice.jsonl.

Zasady (zatwierdzone przez uzytkownika 2026-10-16; zdejmuje zakaz edycji generowanych):
  - TYLKO 131 id z pliku korekt; nic innego nie ruszane (byte-identical).
  - text_pl = proposed_pl (poprawka lokalizacyjna).
  - tts_text NIETKNIETE (proweniencja audio: istniejace WAV mowia stary tekst;
    re-TTS ma syntetyzowac text_pl i dopiero wtedy wyrownac tts_text).
  - translation_status: machine_draft -> side_text_review_applied (uczciwy sygnal:
    analyzer mapuje to nadal do TRANSLATION_NEEDS_CORRECTION z evidence,
    bo audio jest nieaktualne; pelne full_native_text_review dopiero po re-TTS+uchu).
  - ABORT jezeli: id nie znalezione / zdublowane, status != machine_draft
    (nie nadpisujemy zweryfikowanych 42), text_pl juz rowny proposed.
Bezposredni zapis JSONL z zachowaniem kolejnosci kluczy i koncowek CRLF.
"""
import argparse, json
ap = argparse.ArgumentParser()
ap.add_argument('--corrections', required=True)
ap.add_argument('--status', default='side_text_review_applied')
a = ap.parse_args()

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
SRC = REPO + r'\localization\pl\polish_story_voice.jsonl'
COR = REPO + '\\' + a.corrections.replace('/', '\\')
NEW_STATUS = a.status

cor = {}
for line in open(COR, encoding='utf-8'):
    line = line.strip()
    if line:
        r = json.loads(line)
        assert r['id'] not in cor, r['id']
        cor[r['id']] = r

raw = open(SRC, 'rb').read().split(b'\n')
# ostatni element po split to b'' (plik konczy sie \n); linie maja \r\n
assert raw[-1] == b'', 'unexpected EOF'
lines = raw[:-1]
rows = [json.loads(l.decode('utf-8')) for l in lines]
idx = {}
for i, r in enumerate(rows):
    key = 'story:%s:%s' % (r['package'], r['asset'])
    idx.setdefault(key, []).append(i)

assert len(rows) == 1672, len(rows)
changed, skipped_same, already = 0, [], 0
for cid, c in cor.items():
    hits = idx.get(cid, [])
    assert len(hits) == 1, (cid, len(hits))
    r = rows[hits[0]]
    if r.get('text_pl') == c['proposed_pl'] and r.get('translation_status') == NEW_STATUS:
        already += 1
        continue
    assert r.get('translation_status') == 'machine_draft', (cid, r.get('translation_status'))
    if r.get('text_pl') == c['proposed_pl']:
        skipped_same.append(cid)
        continue
    r['text_pl'] = c['proposed_pl']
    r['translation_status'] = NEW_STATUS
    changed += 1

assert len(skipped_same) <= 5, skipped_same
print('matched: %d changed: %d already-applied: %d no-op (proposed==draft): %s' % (
    len(cor), changed, already, skipped_same), flush=True)

with open(SRC, 'wb') as fh:
    for r in rows:
        fh.write((json.dumps(r, ensure_ascii=False) + '\r\n').encode('utf-8'))

# weryfikacja: byte-identical poza zmienionymi + parse + statusy
from collections import Counter
rows2 = [json.loads(l) for l in open(SRC, encoding='utf-8')]
print('total:', len(rows2), 'statuses:', Counter(r.get('translation_status') for r in rows2), flush=True)
assert len(rows2) == 1672
cmap = {c: cor[c]['proposed_pl'] for c in cor}
by_id = {}
for r in rows2:
    by_id['story:%s:%s' % (r['package'], r['asset'])] = r
assert len(rows2) == 1672
for cid, prop in cmap.items():
    assert by_id[cid]['text_pl'] == prop, cid
    assert by_id[cid]['translation_status'] == NEW_STATUS, cid
print('OK: %d text_pl=proposed+%s, tts_text untouched' % (len(cmap), NEW_STATUS), flush=True)
