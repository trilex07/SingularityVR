"""Intro innym silnikiem: Piper pl_PL-darkman-medium (lokalny ONNX, VITS - stabilny,
bez halucynacji autoregresji). Jeden spojny glos zastepczy dla 6 kwestii
(NIM/CP/GST - wszyscy mezczyzni; zmiana glosu vs pack oznaczona w manifeście).
Teksty = poprawione pl z intro-pl.json. Konwersja do PCM16 mono 24 kHz peak -1 dBFS.
Wyjscie TYLKO temp. Bez dotykania packa."""
import json, os, subprocess, hashlib
REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
VOICE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\models\piper\pl_PL-darkman-medium'
OUT = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth_intro_piper'
os.makedirs(OUT, exist_ok=True)
PIPER = r'C:\Users\Trilex\vr-mods\.piper_tools\bin\piper.exe'
FF = 'ffmpeg'

pl = {r['asset']: r for r in
      json.load(open(REPO + r'\remaining-voices\intro-pl.json', encoding='utf-8'))}
ASSETS = ['INTRO_1005_NIM', 'INTRO_1006_CP', 'INTRO_1011_GST',
          'INTRO_1012_NIM', 'INTRO_1014_GST', 'INTRO_1016_CP']
man = []
for n, a in enumerate(ASSETS, 1):
    text = pl[a]['pl']
    raw = os.path.join(OUT, a + '.raw.wav')
    fin = os.path.join(OUT, a + '.pl.wav')
    with open(os.path.join(OUT, a + '.txt'), 'w', encoding='utf-8') as fh:
        fh.write(text)
    r1 = subprocess.run([PIPER, '-m', VOICE + '.onnx',
                         '-i', os.path.join(OUT, a + '.txt'), '-f', raw],
                        capture_output=True, text=True, errors='replace')
    if r1.returncode != 0 or not os.path.exists(raw):
        man.append({'file': a + '.pl.wav', 'asset': a,
                    'FAIL': 'piper rc=%d: %s' % (r1.returncode, (r1.stderr or '')[-800:])})
        print('FAIL', a, (r1.stderr or '')[:120], flush=True)
        continue
    r2 = subprocess.run([FF, '-hide_banner', '-loglevel', 'error', '-y', '-i', raw,
                         '-ar', '24000', '-ac', '1', '-af',
                         'loudnorm=I=-16:TP=-1.0:LRA=11:print_format=none',
                         fin], capture_output=True, text=True)
    # twardy sufit peak -1 dBFS (niezaleznie od loudnorm)
    import struct, wave, math
    with wave.open(fin, 'rb') as w:
        fr = w.getnframes()
        s = struct.unpack('<%dh' % fr, w.readframes(fr))
    peak = max(abs(x) for x in s) / 32768.0
    if peak > 10 ** (-1.0 / 20):
        g = (10 ** (-1.0 / 20)) / peak
        s2 = [max(-32768, min(32767, int(round(x * g)))) for x in s]
        import array
        with wave.open(fin, 'wb') as w:
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
            w.writeframes(array.array('h', s2).tobytes())
    h = hashlib.sha256(open(fin, 'rb').read()).hexdigest()[:16]
    with wave.open(fin, 'rb') as w:
        dur = w.getnframes() / w.getframerate()
    man.append({'file': a + '.pl.wav', 'asset': a, 'voice': 'piper-darkman-fallback',
                'text_pl': text, 'dur_s': round(dur, 2), 'budget_s': pl[a].get('dur'),
                'sha16': h})
    print('SYN %d/%d %s %.1fs' % (n, len(ASSETS), a, dur), flush=True)

with open(REPO + r'\dubbing-corrections\resynth_manifest_intro_piper.jsonl', 'w', encoding='utf-8') as fh:
    for m in man:
        fh.write(json.dumps(m, ensure_ascii=False) + '\n')
print('DONE ok=%d/%d' % (sum(1 for m in man if 'FAIL' not in m), len(man)), flush=True)
