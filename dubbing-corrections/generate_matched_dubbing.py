"""Generator dubbingu z klonowaniem glosu od angielskich oryginalow (Pocket-TTS Polish 6L).
Dopasowanie emocji (temperatura zalezna od dynamiki i roli) oraz dopasowanie glosnosci (RMS matching z soft-limiterem).
Format wyjsciowy: PCM16 mono 24 kHz (UE3 standard), peak <= -1.0 dBFS.
Pakiety objete: intro (11), vignette (6), skipped (97), barks (268).
"""
import os, sys, time, math, miniaudio, array as pyarray, numpy as np, wave, json, shutil, hashlib

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
from pocket_tts import TTSModel

REPO_ROOT = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
CORR_DIR = os.path.join(REPO_ROOT, 'dubbing-corrections')
EN_BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
DUB_ROOT = r'C:\Dubbing-aktualny'
BACKUP_DIR = os.path.join(DUB_ROOT, '_backup_pre_regen')
LOG_PATH = os.path.join(CORR_DIR, 'matched_generation_log.jsonl')
TEMP_REF = r'C:\Users\Trilex\AppData\Local\Temp\matched_ref.wav'

os.makedirs(BACKUP_DIR, exist_ok=True)
os.makedirs(os.path.dirname(TEMP_REF), exist_ok=True)

# 1. Build index of all English OGG reference files
print('Indexing English reference OGG files...', flush=True)
en_index = {}
for root, dirs, files in os.walk(EN_BASE):
    for f in files:
        if f.lower().endswith('.ogg'):
            asset = os.path.splitext(f)[0]
            en_index[asset.lower()] = os.path.join(root, f)
print(f'Total English reference files indexed: {len(en_index)}', flush=True)

# 2. Helpers
def ogg_to_mono24(src):
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float32) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != 24000:
        idx = np.linspace(0, len(x) - 1, int(len(x) * 24000 / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    return x

def write_temp_ref(x):
    with wave.open(TEMP_REF, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(24000)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())
    return TEMP_REF

def soft_limit(x, threshold_db=-3.0, ceiling_db=-1.0):
    thresh = 10 ** (threshold_db / 20)
    ceiling = 10 ** (ceiling_db / 20)
    y = np.copy(x)
    abs_y = np.abs(y)
    mask = abs_y > thresh
    if np.any(mask):
        excess = (abs_y[mask] - thresh) / (1.0 - thresh + 1e-9)
        compressed = thresh + (ceiling - thresh) * np.tanh(excess * 1.5)
        y[mask] = np.sign(y[mask]) * compressed
    peak = np.max(np.abs(y))
    if peak > ceiling:
        y = y * (ceiling / peak)
    return y

def save_pcm16_mono24(dst_path, x):
    os.makedirs(os.path.dirname(dst_path), exist_ok=True)
    # Clip and quantize to int16
    clipped = np.clip(x, -1.0, 1.0)
    samples_i16 = (clipped * 32767).astype(np.int16)
    with wave.open(dst_path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(24000)
        w.writeframes(samples_i16.tobytes())
    return dst_path

def split_sentences(text):
    import re
    parts = [p.strip() for p in re.split(r'(?<=[.!?…])\s+', text.strip()) if p.strip()]
    return parts if parts else [text.strip()]

# 3. Load models
print('Loading Pocket-TTS models for adaptive emotion...', flush=True)
# High-energy / combat shout model (barks)
model_high = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.78, quantize=True)
# Dialogue / narrative model (vignette, calm skipped)
model_med = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.68, quantize=True)
print('Pocket-TTS models loaded successfully!', flush=True)

# 4. Synthesize with English donor and Loudness Matching
def generate_matched_line(text, en_ogg, temp_mode='high'):
    x_en = ogg_to_mono24(en_ogg)
    dur_en = len(x_en) / 24000.0
    rms_en = float(np.sqrt(np.mean(x_en ** 2))) or 1e-6
    peak_en = float(np.max(np.abs(x_en))) or 1e-6
    
    write_temp_ref(x_en)
    model = model_high if temp_mode == 'high' else model_med
    
    chunks = split_sentences(text)
    parts = []
    for ch in chunks:
        state = model.get_state_for_audio_prompt(TEMP_REF)
        audio = model.generate_audio(state, ch).numpy().squeeze()
        parts.append(np.atleast_1d(audio).astype(np.float32))
        
    gap = np.zeros(int(24000 * 0.04), dtype=np.float32)
    full = parts[0]
    for p in parts[1:]:
        full = np.concatenate([full, gap, p])
        
    dur_pl = len(full) / 24000.0
    rms_raw = float(np.sqrt(np.mean(full ** 2))) or 1e-6
    
    # Loudness matching
    gain = rms_en / rms_raw
    # Bound gain between -12 dB and +18 dB to avoid noise explosions
    gain = max(0.25, min(8.0, gain))
    
    gained = full * gain
    limited = soft_limit(gained, threshold_db=-3.0, ceiling_db=-1.0)
    
    rms_final = float(np.sqrt(np.mean(limited ** 2))) or 1e-6
    peak_final = float(np.max(np.abs(limited))) or 1e-6
    
    return limited, dur_en, dur_pl, 20*math.log10(rms_en), 20*math.log10(rms_final), 20*math.log10(peak_final)

def run():
    log_file = open(LOG_PATH, 'w', encoding='utf-8')
    t_start = time.time()
    
    # Load curated datasets
    intro_curated = json.load(open(os.path.join(CORR_DIR, 'intro_pl_curated.json'), encoding='utf-8'))
    vignette_curated = json.load(open(os.path.join(CORR_DIR, 'vignette_pl_curated.json'), encoding='utf-8'))
    skipped_curated = json.load(open(os.path.join(CORR_DIR, 'skipped_pl_curated.json'), encoding='utf-8'))
    barks_curated = json.load(open(os.path.join(CORR_DIR, 'barks_pl_curated.json'), encoding='utf-8'))
    barks_list = json.load(open(os.path.join(REPO_ROOT, 'remaining-voices', 'bark-pl.json'), encoding='utf-8'))
    
    tasks = []
    
    # PACK: INTRO
    for asset, pl_text in intro_curated.items():
        base_asset = asset.replace('_alt', '')
        ogg = en_index.get(base_asset.lower())
        if ogg:
            dst = os.path.join(DUB_ROOT, 'intro', asset + '.pl.wav')
            tasks.append(('intro', asset, pl_text, ogg, dst, 'high'))
            
    # PACK: VIGNETTE
    for asset, pl_text in vignette_curated.items():
        ogg = en_index.get(asset.lower())
        if ogg:
            dst = os.path.join(DUB_ROOT, 'vignette', asset + '.pl.wav')
            tasks.append(('vignette', asset, pl_text, ogg, dst, 'med'))
            
    # PACK: SKIPPED
    for asset, info in skipped_curated.items():
        ogg = en_index.get(asset.lower())
        if ogg:
            dst = os.path.join(DUB_ROOT, 'skipped', asset + '.pl.wav')
            tasks.append(('skipped', asset, info['pl'], ogg, dst, 'med'))
            
    # PACK: BARKS
    for b in barks_list:
        asset = b['asset']
        en_text = b['en']
        pl_text = barks_curated.get(en_text, b['pl'])
        ogg = en_index.get(asset.lower())
        if ogg:
            dst = os.path.join(DUB_ROOT, 'barks', asset + '.pl.wav')
            tasks.append(('barks', asset, pl_text, ogg, dst, 'high'))
            
    print(f'Ready to generate {len(tasks)} files across all target packs!', flush=True)
    
    success = 0
    errors = 0
    
    for i, (pack, asset, text, ogg, dst, temp_mode) in enumerate(tasks, 1):
        t0 = time.time()
        try:
            # Backup original if exists
            if os.path.exists(dst):
                bak_path = os.path.join(BACKUP_DIR, pack, os.path.basename(dst))
                if not os.path.exists(bak_path):
                    os.makedirs(os.path.dirname(bak_path), exist_ok=True)
                    shutil.copyfile(dst, bak_path)
                    
            audio, dur_en, dur_pl, rms_en, rms_pl, peak_pl = generate_matched_line(text, ogg, temp_mode=temp_mode)
            save_pcm16_mono24(dst, audio)
            
            sha16 = hashlib.sha256(open(dst, 'rb').read()).hexdigest()[:16]
            delta = rms_pl - rms_en
            
            record = {
                'pack': pack,
                'asset': asset,
                'file': os.path.basename(dst),
                'text_pl': text,
                'dur_en': round(dur_en, 2),
                'dur_pl': round(dur_pl, 2),
                'rms_en_db': round(rms_en, 1),
                'rms_pl_db': round(rms_pl, 1),
                'delta_db': round(delta, 1),
                'peak_pl_db': round(peak_pl, 1),
                'sha16': sha16,
                'gen_sec': round(time.time() - t0, 2)
            }
            log_file.write(json.dumps(record, ensure_ascii=False) + '\n')
            log_file.flush()
            success += 1
            
            if i % 10 == 0 or i == len(tasks) or pack in ('intro', 'vignette'):
                elapsed = (time.time() - t_start) / 60.0
                print(f'[{i}/{len(tasks)}] {pack:8s} {asset:22s} | EN: {rms_en:5.1f}dB | PL: {rms_pl:5.1f}dB | Delta: {delta:4.1f}dB | Peak: {peak_pl:4.1f}dB | {elapsed:.1f}m', flush=True)
        except Exception as ex:
            errors += 1
            print(f'ERROR on {pack} {asset}: {ex}', flush=True)
            log_file.write(json.dumps({'pack': pack, 'asset': asset, 'ERROR': str(ex)}, ensure_ascii=False) + '\n')
            log_file.flush()
            
    log_file.close()
    total_time = (time.time() - t_start) / 60.0
    print(f'=== GENERATION COMPLETE ===', flush=True)
    print(f'Total: {len(tasks)} | Success: {success} | Errors: {errors} | Total time: {total_time:.2f} min', flush=True)

if __name__ == '__main__':
    run()
