import json, os, re, sys
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np
from pocket_tts import TTSModel
from scipy.io import wavfile
from faster_whisper import WhisperModel
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections')
from resynth_corrected import clean, ogg_to_wav_mono24

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
OUT = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth_1016micro'
os.makedirs(OUT, exist_ok=True)
ENB = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference\SP_VI_intro_Kismet_LOC_INT\SoundNodeWave'
REFS = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual\_refs_en'
TEXT = 'Dostaliśmy! Dostaliśmy! Mejday, mejday, mejday! Spadamy!'

model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                            temp=0.65, quantize=True)
asr = WhisperModel('small', device='cpu', compute_type='int8')

def wav_to_16k_mono(path):
    sr, s = wavfile.read(path)
    s = np.asarray(s, dtype=np.float64)
    idx = np.linspace(0, len(s) - 1, int(len(s) * 16000 / sr))
    return np.interp(idx, np.arange(len(s)), s).astype(np.float32)

def toks(s):
    return re.findall(r'[a-ząćęłńóśźż]+', s.lower())

def attempt(name, donor):
    st = model.get_state_for_audio_prompt(donor)
    au = model.generate_audio(st, clean(TEXT)).numpy().squeeze()
    full = np.atleast_1d(au).astype(np.float32)
    peak = float(np.max(np.abs(full))) or 1e-6
    g = (10 ** (-1.0 / 20)) / peak
    p = os.path.join(OUT, 'INTRO_1016_CP.%s.wav' % name)
    wavfile.write(p, 24000, (np.clip(full * g, -1, 1) * 32767).astype(np.int16))
    recs, txts = [], []
    for _ in range(2):
        segs, _ = asr.transcribe(wav_to_16k_mono(p), language='pl', beam_size=5)
        txt = ' '.join(x.text for x in segs).strip()
        at, et = toks(txt), toks(TEXT)
        recs.append(round(len(set(at) & set(et)) / max(1, len(set(et))), 2))
        txts.append(txt[:100])
    ok = sum(1 for r in recs if r >= 0.5) >= 1
    print('%s dur=%.1f draws=%s %s' % (name, len(full) / 24000, recs,
                                      'PASS' if ok else 'FAIL'), flush=True)
    for t in txts:
        print('   ', t[:100], flush=True)
    return ok, p

own = os.path.join(OUT, '_own_en.wav')
ogg_to_wav_mono24(os.path.join(ENB, 'INTRO_1016_CP.ogg'), own)
ok1, _ = attempt('own', own)
if not ok1:
    attempt('ref645', os.path.join(REFS, '645b6de719763bc4_en.wav'))
print('DONE', flush=True)
