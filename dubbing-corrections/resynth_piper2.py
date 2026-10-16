import hashlib
import json
import os
import subprocess

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
VOICE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\models\piper\pl_PL-darkman-medium\pl_PL-darkman-medium.onnx'
OUT = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth_intro_piper'
os.makedirs(OUT, exist_ok=True)
PIPER = r'C:\Users\Trilex\vr-mods\.piper_tools\bin\piper.exe'

pl = {r['asset']: r for r in
      json.load(open(os.path.join(REPO, 'remaining-voices', 'intro-pl.json'),
                     encoding='utf-8'))}
ASSETS = ['INTRO_1005_NIM', 'INTRO_1006_CP', 'INTRO_1011_GST',
          'INTRO_1012_NIM', 'INTRO_1014_GST', 'INTRO_1016_CP']
man = []
for n, a in enumerate(ASSETS, 1):
    text = pl[a]['pl']
    txt = os.path.join(OUT, a + '.txt')
    with open(txt, 'w', encoding='utf-8') as fh:
        fh.write(text + '\n')
    raw = os.path.join(OUT, a + '.raw.wav')
    fin = os.path.join(OUT, a + '.pl.wav')
    if os.path.exists(raw):
        os.remove(raw)
    r1 = subprocess.run([PIPER, '-m', VOICE, '-i', txt, '-f', raw],
                        capture_output=True, text=True, errors='replace')
    if r1.returncode != 0 or not os.path.exists(raw):
        man.append({'file': a + '.pl.wav', 'asset': a,
                    'FAIL': 'rc=%d %s' % (r1.returncode, (r1.stderr or '')[-500:])})
        print('FAIL', a, flush=True)
        continue
    r2 = subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y',
                         '-i', raw, '-ar', '24000', '-ac', '1', fin],
                        capture_output=True, text=True)
    assert r2.returncode == 0, r2.stderr[-300:]
    import struct
    import wave
    with wave.open(fin, 'rb') as w:
        fr = w.getnframes()
        s = struct.unpack('<%dh' % fr, w.readframes(fr))
    peak = max(abs(x) for x in s) / 32768.0
    if peak > 0.891250938:
        g = 0.891250938 / peak
        s2 = [max(-32768, min(32767, int(round(x * g)))) for x in s]
        import array
        with wave.open(fin, 'wb') as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(24000)
            w.writeframes(array.array('h', s2).tobytes())
    man.append({'file': a + '.pl.wav', 'asset': a, 'voice': 'piper-darkman-fallback',
                'text_pl': text, 'dur_s': round(fr / 24000, 2),
                'budget_s': pl[a].get('dur'),
                'sha16': hashlib.sha256(open(fin, 'rb').read()).hexdigest()[:16]})
    print('SYN %d/%d %s %.1fs' % (n, len(ASSETS), a, fr / 24000), flush=True)

with open(os.path.join(REPO, 'dubbing-corrections',
                       'resynth_manifest_intro_piper.jsonl'), 'w', encoding='utf-8') as fh:
    for m in man:
        fh.write(json.dumps(m, ensure_ascii=False) + '\n')
print('DONE ok=%d/%d' % (sum(1 for m in man if 'FAIL' not in m), len(man)), flush=True)
