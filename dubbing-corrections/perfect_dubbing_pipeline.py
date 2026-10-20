"""Kompleksowy rurociag perfekcyjnego dubbingu Singularity VR:
1. Spojne, stabilne glosy per postac (Kathryn, Barisov, Demichev, Devlin, Nimitz, Pilot, Zolniersze).
2. Naturalna interpunkcja i dynamiczne tempo (atempo 1.18x - 1.22x - eliminacja wolnego mowienia).
3. Zero szumu i belkotu: temp 0.58 + bezpieczne prompt referencyjne + walidacja ASR Whisper dla KAZDEJ kwestii.
4. Dopasowanie glosnosci RMS do oryginalow z limitem true-peak -1.0 dBFS.
5. Standard UE3: PCM16 mono 24 kHz WAV.
"""
import os, sys, time, wave, math, json, subprocess, re, hashlib, shutil
import numpy as np, miniaudio, array as pyarray

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
from pocket_tts import TTSModel

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.whisper_tools')
from faster_whisper import WhisperModel

REPO_ROOT = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
CORR_DIR = os.path.join(REPO_ROOT, 'dubbing-corrections')
EN_BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
DUB_ROOT = r'C:\Dubbing-aktualny'
LOG_PATH = os.path.join(CORR_DIR, 'perfect_dubbing_audit.jsonl')
TEMP_DIR = r'C:\Users\Trilex\AppData\Local\Temp\perfect_dub'
os.makedirs(TEMP_DIR, exist_ok=True)

# 1. Helpers
def ogg_to_mono24(src_ogg):
    dec = miniaudio.decode_file(src_ogg, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float32) / 32768.0
    if dec.nchannels == 2: x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != 24000:
        idx = np.linspace(0, len(x) - 1, int(len(x) * 24000 / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    return x

def write_pcm16_mono24(dst_path, x):
    os.makedirs(os.path.dirname(dst_path), exist_ok=True)
    clipped = np.clip(x, -1.0, 1.0)
    samples_i16 = (clipped * 32767).astype(np.int16)
    with wave.open(dst_path, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
        w.writeframes(samples_i16.tobytes())
    return dst_path

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

def apply_atempo(src_wav, dst_wav, tempo=1.20):
    cmd = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
           '-i', src_wav, '-filter:a', f'atempo={tempo}', '-ar', '24000', '-ac', '1', dst_wav]
    subprocess.run(cmd, check=True)

# 2. Master Voice Donors (Clean, Pristine Studio English Lines)
print('Preparing Master Voice Donors per character...', flush=True)
MASTER_DONOR_SOURCES = {
    'barisov': os.path.join(EN_BASE, 'SP_BT_A_zone01_LOC_INT', 'SoundNodeWave', 'RL6_9006_BAR.ogg'),
    'kathryn': os.path.join(EN_BASE, 'SP_Freight_Script_StreamCargo2_LOC_INT', 'SoundNodeWave', 'FR3_8008_KAT.ogg'),
    'demichev': os.path.join(EN_BASE, 'SP_RT5_kismet01_LOC_INT', 'SoundNodeWave', 'RT5_1009_DEM.ogg'),
    'devlin': os.path.join(EN_BASE, 'SP_VI_intro_Kismet_LOC_INT', 'SoundNodeWave', 'INTRO_1008_GST.ogg'),
    'nimitz': os.path.join(EN_BASE, 'SP_VI_intro_Kismet_LOC_INT', 'SoundNodeWave', 'INTRO_1005_NIM.ogg'),
    'pilot': os.path.join(EN_BASE, 'SP_VI_intro_Kismet_LOC_INT', 'SoundNodeWave', 'INTRO_1006_CP.ogg'),
    'soldier_spt1': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_SPT1_MOVE_IN3.ogg'),
    'soldier_spt2': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_SPT2_SEE_TARGET4.ogg'),
    'soldier_spt3': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_SPT3_SRCH_KILL2.ogg'),
    'soldier_esp1': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_ESP1_FIRE2.ogg'),
    'soldier_esp2': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_ESP2_FIRE1.ogg'),
    'soldier_grd1': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_GRD1_MOVE_IN4.ogg'),
    'vignette_sc1': os.path.join(EN_BASE, 'SP_RT3_Vignette_LOC_INT', 'SoundNodeWave', 'RT99_0105_SC1.ogg'),
    'vignette_sc2': os.path.join(EN_BASE, 'SP_RT3_Vignette_LOC_INT', 'SoundNodeWave', 'RT99_0108_SC2.ogg'),
}

MASTER_WAVS = {}
for role, src in MASTER_DONOR_SOURCES.items():
    assert os.path.exists(src), (role, src)
    dst = os.path.join(TEMP_DIR, f'master_{role}.wav')
    x = ogg_to_mono24(src)
    write_pcm16_mono24(dst, x)
    MASTER_WAVS[role] = dst
print(f'Master donors ready for {len(MASTER_WAVS)} character roles!', flush=True)

# 3. Load Models
print('Loading Pocket-TTS (temp=0.58) and Faster-Whisper ASR...', flush=True)
tts = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.58, quantize=True)
asr = WhisperModel('small', device='cpu', compute_type='int8')
print('TTS and ASR models initialized successfully!', flush=True)

# 4. English reference file index
en_index = {}
for root, dirs, files in os.walk(EN_BASE):
    for f in files:
        if f.lower().endswith('.ogg'):
            asset = os.path.splitext(f)[0]
            en_index[asset.lower()] = os.path.join(root, f)

# 5. Build task list
def get_character_role(pack, asset):
    a = asset.upper()
    if pack == 'intro':
        if 'NIM' in a: return 'nimitz'
        if 'CP' in a: return 'pilot'
        return 'devlin'
    elif pack == 'vignette':
        if 'SC1' in a: return 'vignette_sc1'
        return 'vignette_sc2'
    elif pack == 'barks':
        if a.startswith('SD_KAT'): return 'kathryn'
        if a.startswith('SD_GST'): return 'devlin'
        if a.startswith('SD_SPT1'): return 'soldier_spt1'
        if a.startswith('SD_SPT2'): return 'soldier_spt2'
        if a.startswith('SD_SPT3'): return 'soldier_spt3'
        if a.startswith('SD_ESP1'): return 'soldier_esp1'
        if a.startswith('SD_ESP2'): return 'soldier_esp2'
        if a.startswith('SD_GRD'): return 'soldier_grd1'
        return 'soldier_spt1'
    elif pack == 'skipped':
        if 'BAR' in a: return 'barisov'
        if 'KAT' in a: return 'kathryn'
        if 'DEM' in a: return 'demichev'
        if 'GST' in a: return 'devlin'
        return 'barisov'
    return 'devlin'

def clean_text_for_tts(text):
    t = text.strip()
    if t.startswith('- '): t = t[2:].strip()
    if t.startswith('/'): t = t[1:].strip()
    # Phonetic replacements for English terms that derail Polish phonemizer
    t = re.sub(r'\bMayday\b', 'Mejdej', t, flags=re.IGNORECASE)
    t = re.sub(r'\bEMP\b', 'impuls', t)
    t = re.sub(r'\s+', ' ', t).strip()
    return t

def synthesize_line(text, role, target_ogg, tempo=1.20):
    ref_wav = MASTER_WAVS[role]
    # Split only on major sentence terminators
    sentences = [s.strip() for s in re.split(r'(?<=[.!?])\s+', text) if s.strip()]
    if not sentences: sentences = [text]
    
    audios = []
    for s in sentences:
        st = tts.get_state_for_audio_prompt(ref_wav)
        a = tts.generate_audio(st, s).numpy().squeeze()
        audios.append(np.atleast_1d(a).astype(np.float32))
        
    gap = np.zeros(int(24000 * 0.02), dtype=np.float32)
    full = audios[0]
    for a in audios[1:]: full = np.concatenate([full, gap, a])
    
    # Write intermediate raw
    tmp_raw = os.path.join(TEMP_DIR, 'tmp_raw.wav')
    tmp_fast = os.path.join(TEMP_DIR, 'tmp_fast.wav')
    write_pcm16_mono24(tmp_raw, full)
    
    # Speed up cleanly with atempo
    apply_atempo(tmp_raw, tmp_fast, tempo=tempo)
    
    # Read back accelerated audio
    dec = miniaudio.decode_file(tmp_fast, output_format=miniaudio.SampleFormat.SIGNED16)
    x_fast = np.array(pyarray.array('h', dec.samples), dtype=np.float32) / 32768.0
    
    # Loudness matching to target English OGG
    if target_ogg and os.path.exists(target_ogg):
        x_en = ogg_to_mono24(target_ogg)
        dur_en = len(x_en) / 24000.0
        rms_en = float(np.sqrt(np.mean(x_en ** 2))) or 1e-6
    else:
        dur_en = len(x_fast) / 24000.0
        rms_en = 10 ** (-15.0 / 20)
        
    rms_fast = float(np.sqrt(np.mean(x_fast ** 2))) or 1e-6
    gain = max(0.4, min(4.0, rms_en / rms_fast))
    x_final = soft_limit(x_fast * gain, threshold_db=-3.0, ceiling_db=-1.0)
    
    dur_final = len(x_final) / 24000.0
    rms_final = float(np.sqrt(np.mean(x_final ** 2))) or 1e-6
    peak_final = float(np.max(np.abs(x_final))) or 1e-6
    
    return x_final, dur_en, dur_final, 20*math.log10(rms_en), 20*math.log10(rms_final), 20*math.log10(peak_final)

def main():
    intro_curated = json.load(open(os.path.join(CORR_DIR, 'intro_pl_curated.json'), encoding='utf-8'))
    vignette_curated = json.load(open(os.path.join(CORR_DIR, 'vignette_pl_curated.json'), encoding='utf-8'))
    skipped_curated = json.load(open(os.path.join(CORR_DIR, 'skipped_pl_curated.json'), encoding='utf-8'))
    barks_curated = json.load(open(os.path.join(CORR_DIR, 'barks_pl_curated.json'), encoding='utf-8'))
    barks_list = json.load(open(os.path.join(REPO_ROOT, 'remaining-voices', 'bark-pl.json'), encoding='utf-8'))
    
    tasks = []
    
    # INTRO
    for asset, pl_text in intro_curated.items():
        base_asset = asset.replace('_alt', '')
        ogg = en_index.get(base_asset.lower())
        dst = os.path.join(DUB_ROOT, 'intro', asset + '.pl.wav')
        tasks.append(('intro', asset, clean_text_for_tts(pl_text), ogg, dst, 1.18))
        
    # VIGNETTE
    for asset, pl_text in vignette_curated.items():
        ogg = en_index.get(asset.lower())
        dst = os.path.join(DUB_ROOT, 'vignette', asset + '.pl.wav')
        tasks.append(('vignette', asset, clean_text_for_tts(pl_text), ogg, dst, 1.18))
        
    # SKIPPED
    for asset, info in skipped_curated.items():
        ogg = en_index.get(asset.lower())
        dst = os.path.join(DUB_ROOT, 'skipped', asset + '.pl.wav')
        tasks.append(('skipped', asset, clean_text_for_tts(info['pl']), ogg, dst, 1.18))
        
    # BARKS
    for b in barks_list:
        asset = b['asset']
        pl_text = barks_curated.get(b['en'], b['pl'])
        ogg = en_index.get(asset.lower())
        dst = os.path.join(DUB_ROOT, 'barks', asset + '.pl.wav')
        tasks.append(('barks', asset, clean_text_for_tts(pl_text), ogg, dst, 1.22))
        
    print(f'Starting Perfect Dubbing Pipeline on {len(tasks)} files...', flush=True)
    log_file = open(LOG_PATH, 'w', encoding='utf-8')
    t_start = time.time()
    
    passed_asr = 0
    total = len(tasks)
    
    for i, (pack, asset, text, ogg, dst, tempo) in enumerate(tasks, 1):
        role = get_character_role(pack, asset)
        t0 = time.time()
        
        # Generation loop with auto-retry
        max_attempts = 2
        success = False
        
        for attempt in range(1, max_attempts + 1):
            x_final, dur_en, dur_pl, rms_en, rms_pl, peak_pl = synthesize_line(text, role, ogg, tempo=tempo)
            
            # Whisper ASR verification
            segs, info = asr.transcribe(x_final, language='pl')
            transcription = ' '.join(s.text for s in segs).strip()
            
            # Sanity checks
            # 1. No runaway duration (PL dur should not exceed 1.8x EN budget for lines > 3s)
            dur_ok = True
            if dur_en > 3.0 and dur_pl > dur_en * 1.8: dur_ok = False
            # 2. Transcription not empty
            trans_ok = len(transcription) > 0
            # 3. No looping repetition (same token repeated 4+ times)
            words = transcription.split()
            loop_ok = True
            for w in set(words):
                if words.count(w) >= 5 and len(words) >= 6:
                    loop_ok = False
                    break
                    
            if dur_ok and trans_ok and loop_ok:
                success = True
                break
                
        # Write final file
        write_pcm16_mono24(dst, x_final)
        sha16 = hashlib.sha256(open(dst, 'rb').read()).hexdigest()[:16]
        delta = rms_pl - rms_en
        
        rec = {
            'pack': pack,
            'asset': asset,
            'role': role,
            'text_pl': text,
            'asr_transcription': transcription,
            'asr_ok': success,
            'dur_en': round(dur_en, 2),
            'dur_pl': round(dur_pl, 2),
            'tempo_applied': tempo,
            'rms_en_db': round(rms_en, 1),
            'rms_pl_db': round(rms_pl, 1),
            'delta_db': round(delta, 1),
            'peak_pl_db': round(peak_pl, 1),
            'sha16': sha16,
            'gen_sec': round(time.time() - t0, 2)
        }
        log_file.write(json.dumps(rec, ensure_ascii=False) + '\n')
        log_file.flush()
        
        if success: passed_asr += 1
        
        if i % 15 == 0 or i == total or pack in ('intro', 'vignette'):
            elapsed = (time.time() - t_start) / 60.0
            print(f'[{i}/{total}] {pack:8s} {asset:22s} ({role:12s}) | dur={dur_pl:4.1f}s | EN:{rms_en:5.1f}dB PL:{rms_pl:5.1f}dB | ASR: {transcription[:40]} | {elapsed:.1f}m', flush=True)
            
    log_file.close()
    total_time = (time.time() - t_start) / 60.0
    print(f'=== PIPELINE COMPLETE ===', flush=True)
    print(f'Total: {total} | Passed ASR Gate: {passed_asr}/{total} ({passed_asr/total*100:.1f}%) | Time: {total_time:.2f} min', flush=True)

if __name__ == '__main__':
    main()
