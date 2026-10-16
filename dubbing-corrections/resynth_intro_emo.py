"""Intro: dawcy o podobnych emocjach (czyste ogg EN z gry, nie kokpit).
1005 NIM stanowcze wezwanie -> REF645 (spokojny autorytet, precedens regen7)
1006 CP spokojne potwierdzenie -> REF645 (precedens regen7)
1011 GST szok/zdziwienie -> bark SEE (okrzyk spostrzezenia, alarm)
1012 NIM pilne ostrzezenie radiowe -> bark ADVANCING (wysoka pilka)
1014 GST panika+krzyk -> bark FIRE (krzyk bojowy, wysokie pobudzenie)
1016 CP panika mayday -> bark FIRE (jw.)
Temp 0.65 (receptura story), zdanie=chunk, gap 0.04, PCM16/24k/-1dBFS. Tylko temp."""
import json, os, re, sys, time, hashlib
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections')
from resynth_corrected import clean, ogg_to_wav_mono24

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
REFS = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual\_refs_en'
ENB = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
OUT = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth_intro_emo'
os.makedirs(OUT, exist_ok=True)
DON = os.path.join(OUT, '_donors_en')
os.makedirs(DON, exist_ok=True)

REF645 = os.path.join(REFS, '645b6de719763bc4_en.wav')
BARK = {
    'SEE': r'SP_VI5_kismet03_LOC_INT/SoundNodeWave/SD_SPT2_SEE_TARGET4.ogg',
    'ADVANCING': r'SP_VI5_kismet03_LOC_INT/SoundNodeWave/SD_GST_ADVANCING2.ogg',
    'FIRE': r'SP_VI6_kismet05_LOC_INT/SoundNodeWave/SD_KAT_FIRE_SUPPRESS3.ogg',
}
JOBS = [
    ('INTRO_1005_NIM', 'REF645'),
    ('INTRO_1006_CP', 'REF645'),
    ('INTRO_1011_GST', 'SEE'),
    ('INTRO_1012_NIM', 'ADVANCING'),
    ('INTRO_1014_GST', 'FIRE'),
    ('INTRO_1016_CP', 'FIRE'),
]

pl = {r['asset']: r['pl'] for r in
      json.load(open(REPO + r'\remaining-voices\intro-pl.json', encoding='utf-8'))}

def donor_wav(key):
    if key == 'REF645':
        assert os.path.exists(REF645)
        return REF645, 'REF645-calm'
    src = os.path.join(ENB, BARK[key].replace('/', os.sep))
    assert os.path.exists(src), src
    dst = os.path.join(DON, key + '_donor_en.wav')
    if not os.path.exists(dst):
        ogg_to_wav_mono24(src, dst)
    return dst, 'bark-%s' % key

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.65...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                            temp=0.65, quantize=True)
SR = model.sample_rate
assert SR == 24000
man = []
for n, (asset, dk) in enumerate(JOBS, 1):
    t0 = time.time()
    try:
        dw, kind = donor_wav(dk)
        text = clean(pl[asset])
        sents = [p for p in re.split(r'(?<=[.!?…])\s+', text) if p]
        parts = []
        for ch in sents:
            st = model.get_state_for_audio_prompt(dw)
            au = model.generate_audio(st, ch).numpy().squeeze()
            parts.append(np.atleast_1d(au).astype(np.float32))
        full = parts[0]
        gap = np.zeros(int(SR * 0.04), dtype=np.float32)
        for p in parts[1:]:
            full = np.concatenate([full, gap, p])
        peak = float(np.max(np.abs(full))) or 1e-6
        g = (10 ** (-1.0 / 20)) / peak
        outp = os.path.join(OUT, asset + '.pl.wav')
        wavfile.write(outp, SR, (np.clip(full * g, -1, 1) * 32767).astype(np.int16))
        man.append({'file': asset + '.pl.wav', 'asset': asset, 'donor': kind,
                    'donor_file': os.path.basename(dw), 'text_pl': text,
                    'chunks': len(sents), 'dur_s': round(len(full) / SR, 2),
                    'peak_gain_db': round(20 * np.log10(g), 2),
                    'sha16': hashlib.sha256(open(outp, 'rb').read()).hexdigest()[:16],
                    'gen_s': round(time.time() - t0, 1)})
        print('SYN %d/%d %s %s %.1fs' % (n, len(JOBS), asset, kind, len(full) / SR),
              flush=True)
    except Exception as ex:
        man.append({'file': asset + '.pl.wav', 'asset': asset,
                    'FAIL': '%s: %s' % (type(ex).__name__, str(ex)[:160])})
        print('FAIL', asset, type(ex).__name__, flush=True)
with open(REPO + r'\dubbing-corrections\resynth_manifest_intro_emo.jsonl', 'w', encoding='utf-8') as fh:
    for m in man:
        fh.write(json.dumps(m, ensure_ascii=False) + '\n')
print('DONE ok=%d/%d' % (sum(1 for m in man if 'FAIL' not in m), len(man)), flush=True)
