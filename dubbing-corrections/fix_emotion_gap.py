"""Fix po odsluchu uzytkownika (2026-10-16):
1) SD_GRD1/ESP2_RPT_PINNED1: 'Pomocy!' zostaje (aprobata), 'Przygwozdzili mnie!'
   z dawca-KRZYKIEM tego samego aktora (jego wlasny okrzyk bojowy MOVE_IN4
   'Go, go, go!' zamiast placzliwego oryginalu tej kwestii) — bez zmiany glosu.
2) Przerwa miedzy zdaniami 0.12 s -> 0.04 s (attempt2 + dotkniete pliki).
3) temp 0.5 (stabilnosc z rund repair) dla dotknietych plikow.
Backup v2 do _backup_v2/ (porownanie A/B). Manifest: attempt=3 + notka."""
import json, os, re, sys, time, hashlib, shutil
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
WAVDIR = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth130'
EN_BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
REFS = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual\_refs_en'

sys.path.insert(0, REPO + r'\dubbing-corrections')
from resynth_corrected import clean, ogg_to_wav_mono24

rows = [json.loads(l) for l in open(REPO + r'\localization\pl\polish_story_voice.jsonl',
                                   encoding='utf-8') if l.strip()]
idx = {}
for r in rows:
    idx.setdefault(r['file'], []).append(r)

def asset_donor_wav(asset):
    """Zdekodowany wlasny ogg EN danego assetu (ten sam aktor); z _donors_en
    (jesli juz zdekodowany przy resyntezie) albo dekoduje na biezaco."""
    dst = os.path.join(WAVDIR, '_donors_en', asset + '_en.wav')
    if os.path.exists(dst):
        return dst
    for key, rs in idx.items():
        for r in rs:
            if r['asset'] == asset and r['package'] == PKG:
                src = os.path.join(EN_BASE, r['source'])
                assert os.path.exists(src), (asset, src)
                return ogg_to_wav_mono24(src, dst)
    raise AssertionError(('no EN ogg for', asset))

PKG = 'SP_VI5_kismet03_LOC_INT'
# (plik wyjsciowy, [(tekst_part, donor_asset)], gap_s, temp)
JOBS = [
    ('SD_GRD1_RPT_PINNED1.wav',
     [('Pomocy!', 'SD_GRD1_RPT_PINNED1'), ('Przygwoździli mnie!', 'SD_GRD1_MOVE_IN4')],
     0.05, 0.5),
    ('SD_ESP2_RPT_PINNED1.wav',
     [('Pomocy!', 'SD_ESP2_RPT_PINNED1'), ('Przygwoździli mnie!', 'SD_ESP2_MOVE_IN4')],
     0.05, 0.5),
]
# reszta attempt2 + 2 single z zalacznikow: te same dawcy co w manifeście, gap 0.04
man = [json.loads(l) for l in open(REPO + r'\dubbing-corrections\resynth_manifest.jsonl',
                                   encoding='utf-8') if l.strip()]
midx = {m['file']: m for m in man if 'FAIL' not in m}
for f in ['1fca8cc6e8f67644.wav', 'fc18985d1ee4277a.wav', '960111000a1afdbf.wav',
          'd46aa5cf683fce12.wav', '14fb7c852b38c25d.wav',
          'bc0ee3caae5d223b.wav', '4fa0f0deff1c1704.wav']:
    m = midx[f]
    sents = [p for p in re.split(r'(?<=[.!?…])\s+', m['text_pl']) if p]
    donor = os.path.join(REFS, f.replace('.wav', '_en.wav'))
    if not os.path.exists(donor):
        donor = os.path.join(WAVDIR, '_donors_en', f.replace('.wav', '_en.wav'))
    JOBS.append((f, [(s, donor) for s in sents], 0.04, 0.5))

bak = os.path.join(WAVDIR, '_backup_v2')
os.makedirs(bak, exist_ok=True)
for f, _, _, _ in JOBS:
    shutil.copyfile(os.path.join(WAVDIR, f), os.path.join(bak, f))
print('backup v2: %d' % len(JOBS), flush=True)

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading model...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                            temp=0.5, quantize=True)
SR = model.sample_rate
assert SR == 24000

log = []
for f, parts, gap_s, temp in JOBS:
    donor_of = {}
    aud = []
    donors_used = []
    for text, d in parts:
        if d in donor_of:
            donor = donor_of[d]
        elif os.path.isfile(d):
            donor = d
        else:
            donor = asset_donor_wav(d)
        donor_of[d] = donor
        donors_used.append(os.path.basename(donor))
        st = model.get_state_for_audio_prompt(donor)
        au = model.generate_audio(st, clean(text)).numpy().squeeze()
        aud.append(np.atleast_1d(au).astype(np.float32))
    full = aud[0]
    gap = np.zeros(int(SR * gap_s), dtype=np.float32)
    for p in aud[1:]:
        full = np.concatenate([full, gap, p])
    peak = float(np.max(np.abs(full))) or 1e-6
    g = (10 ** (-1.0 / 20)) / peak
    outp = os.path.join(WAVDIR, f)
    wavfile.write(outp, SR, (np.clip(full * g, -1, 1) * 32767).astype(np.int16))
    h = hashlib.sha256(open(outp, 'rb').read()).hexdigest()[:16]
    log.append({'file': f, 'parts': [t for t, _ in parts], 'donors': donors_used,
                'gap_s': gap_s, 'temp': temp, 'dur_s': round(len(full) / SR, 2),
                'peak_gain_db': round(20 * np.log10(g), 2), 'sha16': h})
    print('FIX %s %dpart %.1fs donors=%s' % (f, len(parts), len(full) / SR,
                                            ','.join(x[:14] for x in donors_used)), flush=True)

with open(REPO + r'\dubbing-corrections\resynth_fix_log.jsonl', 'w', encoding='utf-8') as fh:
    for r in log:
        fh.write(json.dumps(r, ensure_ascii=False) + '\n')
print('DONE %d' % len(log), flush=True)
