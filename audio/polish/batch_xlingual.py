"""PRODUKCJA: dubbing EN->PL klonowaniem glosu (Pocket-TTS Polish 6L, temp 0.65).
Wejscie: story_voicepack_nllb/manifest.jsonl (992 linie: zrodlo EN ogg + tekst PL).
Wyjscie: story_voicepack_xlingual/<ten sam filename>.wav (PCM16 24kHz mono) + manifest_xlingual.jsonl.
Stare pliki NIETYKANE. Wznawialne (skip istniejacych)."""
import sys, os, time, json
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'

AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio'
EN_BASE = AUDIO + '/story_en_reference'
MANIFEST = AUDIO + '/polish/story_voicepack_nllb/manifest.jsonl'
OUTDIR = AUDIO + '/polish/story_voicepack_xlingual'
REFDIR = os.path.join(OUTDIR, '_refs_en')
os.makedirs(OUTDIR, exist_ok=True)
os.makedirs(REFDIR, exist_ok=True)
OUT_MANIFEST = os.path.join(OUTDIR, 'manifest_xlingual.jsonl')

import miniaudio, array as pyarray
import numpy as np

def ogg_to_wav_mono24(src, dst):
    if os.path.exists(dst):
        return dst
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != 24000:
        idx = np.linspace(0, len(x) - 1, int(len(x) * 24000 / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    import wave
    with wave.open(dst, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())
    return dst

entries = []
with open(MANIFEST, 'r', encoding='utf-8') as f:
    for line in f:
        if line.strip():
            entries.append(json.loads(line))
print('entries:', len(entries), flush=True)

done = set()
if os.path.exists(OUT_MANIFEST):
    with open(OUT_MANIFEST, 'r', encoding='utf-8') as f:
        for line in f:
            if line.strip():
                try:
                    done.add(json.loads(line)['file'])
                except Exception:
                    pass
print('already done:', len(done), flush=True)

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading model...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.65, quantize=True)
print('model ready', flush=True)

t_all = time.time()
made = 0
with open(OUT_MANIFEST, 'a', encoding='utf-8') as out:
    for i, e in enumerate(entries):
        fname = e['file']
        if fname in done or os.path.exists(os.path.join(OUTDIR, fname)):
            continue
        src_ogg = os.path.join(EN_BASE, e['source'])
        if not os.path.exists(src_ogg):
            print('MISSING EN:', e['source'], flush=True)
            continue
        ref = ogg_to_wav_mono24(src_ogg, os.path.join(REFDIR, fname.replace('.wav', '_en.wav')))
        text = e.get('text_pl_machine', '')
        if not text:
            print('EMPTY PL:', fname, flush=True)
            continue
        t0 = time.time()
        try:
            state = model.get_state_for_audio_prompt(ref)
            audio = model.generate_audio(state, text)
            wavfile.write(os.path.join(OUTDIR, fname), model.sample_rate, audio.numpy().squeeze())
        except Exception as ex:
            print('FAIL', fname, type(ex).__name__, str(ex)[:200], flush=True)
            continue
        rec = dict(e)
        rec['pack'] = 'story_xlingual'
        rec['voice_engine'] = 'Pocket-TTS Polish 6L temp=0.65 (EN donor voice)'
        rec['gen_seconds'] = round(time.time() - t0, 1)
        out.write(json.dumps(rec, ensure_ascii=False) + '\n')
        out.flush()
        made += 1
        if made % 25 == 0:
            print('progress: %d/%d (%.1f min)' % (made, len(entries), (time.time() - t_all) / 60), flush=True)
print('DONE made=%d total_time=%.1f min' % (made, (time.time() - t_all) / 60), flush=True)
