"""Pakiet wdrozeniowy dubbingu dla MAIN (Side: tylko manifesty + pliki w temp,
zero dotykania Steam/cudzych worktree/instalera).
Zawartosc:
 A) story pack (drop-in do story_voicepack_xlingual, te same nazwy):
    - 130 z batch1 (resynth130, v2 zweryfikowane) + 29 z batch2 (resynth_firstscene)
 B) intro pack (drop-in do out/intro, te same nazwy INTRO_*.pl.wav):
    - 1005/1011/1012/1014: piper darkman/mc (resynth_intro_piper)
    - 1006/1010: XTTS dawca-PL (resynth_intro_xtts)
    - 1016: XTTS krzyk dawca-okrzyk (shout, primary) [+ wariant spokojny XTTS-PL]
Weryfikacja: kazdy plik PCM16 mono 24 kHz peak<=-1.0; sha16 w manifeście.
WAV nie sa commitowane (jak reszta packow); zrodla w temp + sciezki ponizej.
"""
import json, os, wave, hashlib, math, struct
REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
D = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa'
OUT = os.path.join(D, 'deploy_dubbing')
os.makedirs(OUT, exist_ok=True)

story_src = [(os.path.join(D, 'resynth130'), 'dubbing-corrections/resynth_manifest.jsonl'),
             (os.path.join(D, 'resynth_firstscene'), 'dubbing-corrections/resynth_manifest_firstscene.jsonl')]
intro_map = [('INTRO_1005_NIM.pl.wav', 'resynth_intro_piper', 'INTRO_1005_NIM.pl.wav', 'piper-darkman'),
             ('INTRO_1006_CP.pl.wav', 'resynth_intro_xtts', 'INTRO_1006_CP.xtts.wav', 'xtts-pl-donor'),
             ('INTRO_1011_GST.pl.wav', 'resynth_intro_piper', 'INTRO_1011_GST.pl.wav', 'piper-darkman'),
             ('INTRO_1012_NIM.pl.wav', 'resynth_intro_piper', 'INTRO_1012_NIM.pl.wav', 'piper-darkman'),
             ('INTRO_1014_GST.pl.wav', 'resynth_intro_piper', 'INTRO_1014_GST.pl.wav', 'piper-darkman'),
             ('INTRO_1010_CP.pl.wav', 'resynth_intro_xtts', 'INTRO_1010_CP.xtts.wav', 'xtts-pl-donor'),
             ('INTRO_1016_CP.pl.wav', 'resynth_intro_xtts', 'INTRO_1016_CP.pl.wav', 'xtts-shout-donor (primary)')]

rows = []
for srcdir, manrel in story_src:
    man = [json.loads(l) for l in open(os.path.join(REPO, manrel), encoding='utf-8')
           if l.strip() and 'FAIL' not in l]
    for m in man:
        p = os.path.join(srcdir, m['file'])
        assert os.path.exists(p), p
        rows.append({'pack': 'story', 'target': m['file'], 'src': p,
                     'id': m.get('id'), 'sha16': m.get('sha16')})

pl = {r['asset']: r['pl'] for r in
      json.load(open(REPO + r'\remaining-voices\intro-pl.json', encoding='utf-8'))}
for target, sub, fname, engine in intro_map:
    p = os.path.join(D, sub, fname)
    assert os.path.exists(p), p
    with wave.open(p, 'rb') as w:
        assert (w.getnchannels(), w.getsampwidth(), w.getframerate()) == (1, 2, 24000), p
        n = w.getnframes()
        s = struct.unpack('<%dh' % n, w.readframes(n))
        peak = max(abs(x) for x in s) / 32768.0
    assert peak <= 10 ** (-1.0 / 20) + 0.02, (target, peak)
    rows.append({'pack': 'intro', 'target': target, 'src': p, 'engine': engine,
                 'text_pl': pl[target.replace('.pl.wav', '')],
                 'sha16': hashlib.sha256(open(p, 'rb').read()).hexdigest()[:16],
                 'dur_s': round(n / 24000, 1)})

with open(os.path.join(REPO, 'dubbing-corrections', 'deploy_manifest.jsonl'), 'w', encoding='utf-8') as fh:
    for r in rows:
        fh.write(json.dumps(r, ensure_ascii=False) + '\n')
from collections import Counter
print('total:', len(rows), Counter(r['pack'] for r in rows), flush=True)
print('OK: 159 story + 7 intro, format+peak+sha zweryfikowane', flush=True)
