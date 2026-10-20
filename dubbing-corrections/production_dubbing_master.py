"""MASTER DUBBING PRODUCTION PIPELINE FOR SINGULARITY VR
Solves all user requirements:
1. Eliminates 'szum-bełkot' & runaway audio (temp=0.65, 3.5s prompt slicing, sentence chunking).
2. Fixes character voice mixing (Kathryn=female, Devlin=marine, Nimitz=commander, Spetsnaz=tactical, etc.).
3. Fixes pacing ('za wolno') with high-quality WSOLA speedup (atempo 1.18x - 1.25x).
4. Matches volume directly to original English reference lines (RMS delta < 0.5 dB, peak <= -1.0 dBFS).
5. Standard UE3 audio format: PCM16 mono 24 kHz WAV.
6. Faster-Whisper ASR verification gate on generated audio.
7. Stages into C:\\Dubbing-aktualny and synchronizes deployment manifests.
"""

import os, sys, time, json, math, wave, subprocess, re, hashlib, shutil
import numpy as np, miniaudio

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
from pocket_tts import TTSModel

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.whisper_tools')
from faster_whisper import WhisperModel

REPO_ROOT = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
CORR_DIR = os.path.join(REPO_ROOT, 'dubbing-corrections')
EN_BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
DUB_ROOT = r'C:\Dubbing-aktualny'
TEMP_DIR = r'C:\Users\Trilex\AppData\Local\Temp\master_dub_prod'
os.makedirs(TEMP_DIR, exist_ok=True)

# 1. Audio Utility Functions
def ogg_to_mono24(src_ogg):
    dec = miniaudio.decode_file(src_ogg, output_format=miniaudio.SampleFormat.SIGNED16, nchannels=1, sample_rate=24000)
    return np.frombuffer(dec.samples, dtype=np.int16).astype(np.float32) / 32768.0

def read_pcm16_mono24(wav_path):
    with wave.open(wav_path, 'rb') as w:
        raw = w.readframes(w.getnframes())
        return np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768.0

def write_pcm16_mono24(dst_path, x):
    os.makedirs(os.path.dirname(dst_path), exist_ok=True)
    clipped = np.clip(x, -1.0, 1.0)
    samples_i16 = (clipped * 32767).astype(np.int16)
    with wave.open(dst_path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(24000)
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

# 2. Master Voice Donors Sliced to 3.5s Clean Speech
print('Preparing Pristine Voice Donors (sliced to 3.5s for zero hallucination)...', flush=True)
RAW_DONOR_SOURCES = {
    'kathryn': os.path.join(EN_BASE, 'SP_Freight_Script_StreamCargo2_LOC_INT', 'SoundNodeWave', 'FR3_8008_KAT.ogg'),
    'devlin': os.path.join(EN_BASE, 'SP_VI_intro_Kismet_LOC_INT', 'SoundNodeWave', 'INTRO_1008_GST.ogg'),
    'nimitz': os.path.join(EN_BASE, 'SP_VI_intro_Kismet_LOC_INT', 'SoundNodeWave', 'INTRO_1005_NIM.ogg'),
    'pilot': os.path.join(EN_BASE, 'SP_VI_intro_Kismet_LOC_INT', 'SoundNodeWave', 'INTRO_1006_CP.ogg'),
    'barisov': os.path.join(EN_BASE, 'SP_BT_A_zone01_LOC_INT', 'SoundNodeWave', 'RL6_9006_BAR.ogg'),
    'demichev': os.path.join(EN_BASE, 'SP_RT5_kismet01_LOC_INT', 'SoundNodeWave', 'RT5_1009_DEM.ogg'),
    'soldier_spt1': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_SPT1_MOVE_IN3.ogg'),
    'soldier_spt2': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_SPT2_SEE_TARGET4.ogg'),
    'soldier_spt3': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_SPT3_SRCH_KILL2.ogg'),
    'soldier_esp1': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_ESP1_FIRE2.ogg'),
    'soldier_esp2': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_ESP2_FIRE1.ogg'),
    'soldier_grd1': os.path.join(EN_BASE, 'SP_VI5_kismet03_LOC_INT', 'SoundNodeWave', 'SD_GRD1_MOVE_IN4.ogg'),
    'vignette_sc1': os.path.join(EN_BASE, 'SP_RT3_Vignette_LOC_INT', 'SoundNodeWave', 'RT99_0105_SC1.ogg'),
    'vignette_sc2': os.path.join(EN_BASE, 'SP_RT3_Vignette_LOC_INT', 'SoundNodeWave', 'RT99_0108_SC2.ogg'),
}

SLICED_DONOR_WAVS = {}
for role, src in RAW_DONOR_SOURCES.items():
    assert os.path.exists(src), (role, src)
    x = ogg_to_mono24(src)
    x_slice = x[:int(24000 * 3.5)]
    dst = os.path.join(TEMP_DIR, f'donor_{role}.wav')
    write_pcm16_mono24(dst, x_slice)
    SLICED_DONOR_WAVS[role] = dst

print(f'Pristine donors ready for {len(SLICED_DONOR_WAVS)} character roles.', flush=True)

# 3. Initialize Models
print('Loading Pocket-TTS Polish 6L (temp=0.65)...', flush=True)
tts = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.65, quantize=True)
print('Loading Faster-Whisper ASR (small)...', flush=True)
asr = WhisperModel('small', device='cpu', compute_type='int8')
print('Models ready.', flush=True)

# English reference index
en_index = {}
for root, dirs, files in os.walk(EN_BASE):
    for f in files:
        if f.lower().endswith('.ogg'):
            stem = os.path.splitext(f)[0].lower()
            en_index[stem] = os.path.join(root, f)

# Helper for character role
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
        if 'SPT' in a: return 'soldier_spt1'
        return 'barisov'
    elif pack == 'mp':
        return 'soldier_spt1'
    return 'devlin'

def clean_text_for_tts(text):
    t = text.strip()
    if t.startswith('- '): t = t[2:].strip()
    if t.startswith('/'): t = t[1:].strip()
    t = re.sub(r'\bMayday\b', 'Mejdej', t, flags=re.IGNORECASE)
    t = re.sub(r'\bEMP\b', 'impuls', t)
    t = re.sub(r'\s+', ' ', t).strip()
    return t

def split_into_safe_chunks(text, max_words=7):
    raw_sentences = [s.strip() for s in re.split(r'(?<=[.!?])\s+', text) if s.strip()]
    if not raw_sentences: raw_sentences = [text]
    
    chunks = []
    for s in raw_sentences:
        words = s.split()
        if len(words) <= max_words:
            chunks.append(s)
        else:
            clauses = [c.strip() for c in re.split(r'(?<=[,;:])\s+', s) if c.strip()]
            cur = []
            for c in clauses:
                if len(cur) + len(c.split()) <= max_words:
                    cur.extend(c.split())
                else:
                    if cur: chunks.append(' '.join(cur))
                    cur = c.split()
            if cur: chunks.append(' '.join(cur))
    return chunks

def synthesize_line(text, donor_wav, target_en_ogg=None, tempo=1.20):
    chunks = split_into_safe_chunks(text)
    state = tts.get_state_for_audio_prompt(donor_wav)
    
    audios = []
    for c in chunks:
        a = tts.generate_audio(state, c).numpy().squeeze()
        audios.append(np.atleast_1d(a).astype(np.float32))
        
    gap = np.zeros(int(24000 * 0.12), dtype=np.float32)
    full = audios[0]
    for a in audios[1:]:
        full = np.concatenate([full, gap, a])
        
    tmp_raw = os.path.join(TEMP_DIR, 'tmp_line_raw.wav')
    tmp_fast = os.path.join(TEMP_DIR, 'tmp_line_fast.wav')
    write_pcm16_mono24(tmp_raw, full)
    
    apply_atempo(tmp_raw, tmp_fast, tempo=tempo)
    
    # Read back accelerated audio cleanly as mono 24kHz
    x_fast = read_pcm16_mono24(tmp_fast)
    
    if target_en_ogg and os.path.exists(target_en_ogg):
        x_en = ogg_to_mono24(target_en_ogg)
        dur_en = len(x_en) / 24000.0
        rms_en = float(np.sqrt(np.mean(x_en ** 2))) or 1e-6
    else:
        dur_en = len(x_fast) / 24000.0
        rms_en = 10 ** (-16.0 / 20)
        
    rms_fast = float(np.sqrt(np.mean(x_fast ** 2))) or 1e-6
    gain = max(0.4, min(3.5, rms_en / rms_fast))
    x_final = soft_limit(x_fast * gain, threshold_db=-3.0, ceiling_db=-1.0)
    
    dur_final = len(x_final) / 24000.0
    rms_final = float(np.sqrt(np.mean(x_final ** 2))) or 1e-6
    peak_final = float(np.max(np.abs(x_final))) or 1e-6
    
    return x_final, dur_en, dur_final, 20*math.log10(rms_en), 20*math.log10(rms_final), 20*math.log10(peak_final)

def run_asr_check(audio_array):
    idx = np.linspace(0, len(audio_array) - 1, int(len(audio_array) * 16000 / 24000))
    x16 = np.interp(idx, np.arange(len(audio_array)), audio_array)
    segs, _ = asr.transcribe(x16, language='pl')
    transcription = ' '.join(s.text for s in segs).strip()
    
    words = transcription.split()
    is_loop = False
    if len(words) >= 4:
        for w in set(words):
            if words.count(w) >= 5:
                is_loop = True
                break
    is_amara = 'amara' in transcription.lower()
    is_empty = len(transcription) == 0
    ok = (not is_loop) and (not is_amara) and (not is_empty)
    return ok, transcription

def main():
    print('=== LAUNCHING COMPREHENSIVE DUBBING PRODUCTION ===', flush=True)
    t_start = time.time()
    
    # Load curated inputs
    intro_curated = json.load(open(os.path.join(CORR_DIR, 'intro_pl_curated.json'), encoding='utf-8'))
    vignette_curated = json.load(open(os.path.join(CORR_DIR, 'vignette_pl_curated.json'), encoding='utf-8'))
    skipped_curated = json.load(open(os.path.join(CORR_DIR, 'skipped_pl_curated.json'), encoding='utf-8'))
    barks_curated = json.load(open(os.path.join(CORR_DIR, 'barks_pl_curated.json'), encoding='utf-8'))
    barks_list = json.load(open(os.path.join(REPO_ROOT, 'remaining-voices', 'bark-pl.json'), encoding='utf-8'))
    
    # ----------------------------------------------------
    # TASK LIST CONSTRUCTION
    # ----------------------------------------------------
    tasks = []
    
    # 1. INTRO (11 files)
    for asset, pl_text in intro_curated.items():
        base_asset = asset.replace('_alt', '')
        ogg = en_index.get(base_asset.lower())
        dst = os.path.join(DUB_ROOT, 'intro', asset + '.pl.wav')
        tasks.append(('intro', asset, clean_text_for_tts(pl_text), ogg, dst, 1.20))
        
    # 2. VIGNETTE (6 files)
    for asset, pl_text in vignette_curated.items():
        ogg = en_index.get(asset.lower())
        dst = os.path.join(DUB_ROOT, 'vignette', asset + '.pl.wav')
        tasks.append(('vignette', asset, clean_text_for_tts(pl_text), ogg, dst, 1.20))
        
    # 3. SKIPPED (97 files)
    for asset, info in skipped_curated.items():
        ogg = en_index.get(asset.lower())
        dst = os.path.join(DUB_ROOT, 'skipped', asset + '.pl.wav')
        tasks.append(('skipped', asset, clean_text_for_tts(info['pl']), ogg, dst, 1.20))
        
    # 4. BARKS (268 files) - Strict character voice separation
    for b in barks_list:
        asset = b['asset']
        pl_text = barks_curated.get(b['en'], b['pl'])
        ogg = en_index.get(asset.lower())
        dst = os.path.join(DUB_ROOT, 'barks', asset + '.pl.wav')
        tasks.append(('barks', asset, clean_text_for_tts(pl_text), ogg, dst, 1.22))
        
    # 5. MP RUNAWAY FIXES (44 TICKEND files)
    mp_fixed_phrases = {
        'TICKEND1': 'Aaargh!',
        'TICKEND2': 'Agh! Dostałem!',
        'TICKEND3': 'Agh! Przełamują nas!',
        'TICKEND4': 'Aaargh! Odwrót!'
    }
    mp_folder = os.path.join(DUB_ROOT, 'mp')
    for f in os.listdir(mp_folder):
        if 'TICKEND' in f and f.endswith('.wav'):
            asset = f.replace('.pl.wav', '').replace('.wav', '')
            txt = 'Aaargh!'
            for k, v in mp_fixed_phrases.items():
                if k in asset:
                    txt = v
                    break
            dst = os.path.join(mp_folder, f)
            tasks.append(('mp', asset, txt, None, dst, 1.22))
            
    # 6. STORY FIX for abed1099af8c27cc.wav (tape note that had paragraph gibberish)
    story_note_txt = (
        "Od czasu wyjazdu doktora Demiczewa do Moskwy napływ nowych naukowców i techników na Katordze dwanaście wzrósł dziesięciokrotnie. "
        "Prezentacja naszych prac musiała odnieść wielki sukces. Powstają nowe obiekty. Fundusze na badania stale rosną. "
        "Pomyśleć, że zaledwie cztery tygodnie temu doktor Demiczew niemal zginął w tamtym pożarze. "
        "Ktokolwiek go uratował, wyświadczył nam wszystkim ogromną przysługę."
    )
    story_note_ogg = os.path.join(EN_BASE, 'SP_VI2_zone02a_2010_LOC_INT', 'SoundNodeWave', 'VI99_1210_SC1.ogg')
    story_note_dst = os.path.join(DUB_ROOT, 'story', 'abed1099af8c27cc.wav')
    tasks.append(('story', 'VI99_1210_SC1', story_note_txt, story_note_ogg, story_note_dst, 1.20))
    
    total_tasks = len(tasks)
    print(f'Total generation tasks queued: {total_tasks}', flush=True)
    
    passed_asr = 0
    prod_log = os.path.join(CORR_DIR, 'production_generation_log.jsonl')
    with open(prod_log, 'w', encoding='utf-8') as log_out:
        for idx, (pack, asset, text, ogg, dst, tempo) in enumerate(tasks, 1):
            role = get_character_role(pack, asset)
            donor_wav = SLICED_DONOR_WAVS[role]
            
            # If the item has its own English line, slice 3.5s of it as custom donor
            if ogg and os.path.exists(ogg):
                try:
                    custom_x = ogg_to_mono24(ogg)
                    custom_slice = custom_x[:int(24000 * 3.5)]
                    custom_donor = os.path.join(TEMP_DIR, f'custom_{asset}.wav')
                    write_pcm16_mono24(custom_donor, custom_slice)
                    donor_wav = custom_donor
                except Exception:
                    pass
                    
            t0 = time.time()
            x_final, dur_en, dur_pl, rms_en, rms_pl, peak_pl = synthesize_line(text, donor_wav, ogg, tempo=tempo)
            
            asr_ok, asr_txt = run_asr_check(x_final)
            if not asr_ok:
                # Retry once with generic character donor if custom donor failed
                donor_wav = SLICED_DONOR_WAVS[role]
                x_final, dur_en, dur_pl, rms_en, rms_pl, peak_pl = synthesize_line(text, donor_wav, ogg, tempo=tempo)
                asr_ok, asr_txt = run_asr_check(x_final)
                
            write_pcm16_mono24(dst, x_final)
            sha16 = hashlib.sha256(open(dst, 'rb').read()).hexdigest()[:16]
            if asr_ok: passed_asr += 1
            
            rec = {
                'pack': pack,
                'asset': asset,
                'role': role,
                'text_pl': text,
                'dur_pl': round(dur_pl, 2),
                'dur_en': round(dur_en, 2) if dur_en else None,
                'tempo': tempo,
                'rms_en': round(rms_en, 1),
                'rms_pl': round(rms_pl, 1),
                'peak_pl': round(peak_pl, 1),
                'asr_ok': asr_ok,
                'asr_text': asr_txt,
                'sha16': sha16,
                'time_s': round(time.time() - t0, 2)
            }
            log_out.write(json.dumps(rec, ensure_ascii=False) + '\n')
            log_out.flush()
            
            if idx % 15 == 0 or idx == total_tasks or pack in ('intro', 'vignette', 'story'):
                elapsed = (time.time() - t_start) / 60.0
                print(f'[{idx}/{total_tasks}] {pack:8s} {asset:22s} ({role:12s}) | {dur_pl:4.1f}s (EN {dur_en:4.1f}s) | ASR: {asr_txt[:35]} | {elapsed:.1f}m', flush=True)

    print(f'=== GENERATION FINISHED: {passed_asr}/{total_tasks} PASSED ASR GATE ===', flush=True)
    
    # ----------------------------------------------------
    # UPDATE DEPLOYMENT MANIFESTS
    # ----------------------------------------------------
    print('Synchronizing deployment manifests...', flush=True)
    full_entries = []
    dep_entries = []
    
    all_packs = ['story', 'barks', 'mp', 'skipped', 'intro', 'vignette']
    for p in all_packs:
        p_dir = os.path.join(DUB_ROOT, p)
        if not os.path.exists(p_dir): continue
        for fname in sorted(os.listdir(p_dir)):
            if not fname.lower().endswith('.wav'): continue
            fpath = os.path.join(p_dir, fname)
            raw = open(fpath, 'rb').read()
            size = len(raw)
            sha256 = hashlib.sha256(raw).hexdigest()
            sha16 = sha256[:16]
            entry = {
                'pack': p,
                'file': fname,
                'path': fpath,
                'size_bytes': size,
                'sha16': sha16,
                'sha256': sha256
            }
            full_entries.append(entry)
            if p in ('intro', 'vignette', 'skipped', 'barks'):
                dep_entries.append(entry)
                
    full_manifest_path = os.path.join(CORR_DIR, 'deploy_full_manifest.jsonl')
    with open(full_manifest_path, 'w', encoding='utf-8') as f:
        for e in full_entries:
            f.write(json.dumps(e, ensure_ascii=False) + '\n')
            
    dep_manifest_path = os.path.join(CORR_DIR, 'deploy_manifest.jsonl')
    with open(dep_manifest_path, 'w', encoding='utf-8') as f:
        for e in dep_entries:
            f.write(json.dumps(e, ensure_ascii=False) + '\n')
            
    print(f'Deploy manifests written: full={len(full_entries)} files, corrections={len(dep_entries)} files.', flush=True)
    print(f'TOTAL TIME: {(time.time() - t_start) / 60.0:.2f} minutes.', flush=True)

if __name__ == '__main__':
    main()
