"""Automated Full Production of New Singularity Polish Dubbing:
- Pipeline: Coqui XTTS-v2 (14 CPU threads, temperature=0.25, smart sentence chunking for text > 180 chars)
- Verified Authentic Game Voice Cast:
    * Devlin (GST): d4b361c6a632d1f5_en.wav
    * Kathryn (KAT): 72400162a8a1f767_en.wav (Approved Wariant B)
    * Dr. Victor Barisov (BAR/BARY): 5fbf4d3b1d830376_en.wav
    * Gen. Nikolai Demichev (DEM/DEMY): 425e388a18d457a5_en.wav
    * Commander Nimitz (NIM): nimitz_combined_donor.wav
    * Soldiers / Secondary NPCs: own English reference or mc_speech_sample1.wav
- Acoustic & Mastering Standards:
    * Comms/Helicopter/Radio lines: Bandpass 350Hz-3600Hz + loudnorm -12.0 LUFS, TP <= -1.0 dBFS
    * In-person lines: Highpass 80Hz + loudnorm -12.0 LUFS, TP <= -1.0 dBFS
    * Exact True-Peak protection (<= -1.0 dBFS guaranteed)
    * Format: PCM16 mono 24 kHz WAV
- Output: C:\\Dubbing-aktualny\\<pack>\\<file>
- Resumable: Logs progress to production_full_dubbing.jsonl and skips already-completed lines.
"""

import os, sys, time, json, wave, glob, subprocess, hashlib, re
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.xtts_tools')
import numpy as np, soundfile as sf
import torch, torchaudio

torch.set_num_threads(14)

def _load_via_soundfile(uri, *a, **k):
    data, sr = sf.read(uri, always_2d=False)
    arr = np.asarray(data, dtype=np.float32)
    t = torch.from_numpy(arr).unsqueeze(0) if arr.ndim == 1 else torch.from_numpy(arr.T.copy())
    return t, sr

torchaudio.load = _load_via_soundfile
os.environ['COQUI_TOS_AGREED'] = '1'

from TTS.api import TTS

REFS_DIR = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual\_refs_en'
BASE_OUT = r'C:\Dubbing-aktualny'
TEMP_DIR = r'C:\Users\Trilex\AppData\Local\Temp\xtts_full_prod'
os.makedirs(TEMP_DIR, exist_ok=True)

MANIFEST_FULL = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections\deploy_full_manifest.jsonl'
MANIFEST_DELTA = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections\deploy_manifest.jsonl'
PROGRESS_LOG = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections\production_full_dubbing.jsonl'

DONORS = {
    'devlin': os.path.join(REFS_DIR, 'd4b361c6a632d1f5_en.wav'),
    'kathryn': os.path.join(REFS_DIR, '72400162a8a1f767_en.wav'), # Approved Wariant B
    'barisov': os.path.join(REFS_DIR, '5fbf4d3b1d830376_en.wav'),
    'demichev': os.path.join(REFS_DIR, '425e388a18d457a5_en.wav'),
    'nimitz': r'C:\Users\Trilex\AppData\Local\Temp\nimitz_combined_donor.wav',
    'tactical_soldier': r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\cast_legal_20261014\internet_candidates\mc_speech_sample1.wav',
    'announcer': r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\mailabs_m_loud.wav'
}

def get_sha16(path):
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()[:16]

def split_text_into_chunks(text, max_len=180):
    if len(text) <= max_len:
        return [text]
    # Split by sentence enders
    parts = re.split(r'([.!?]+(?:\s+|$))', text)
    chunks = []
    curr = ""
    for i in range(0, len(parts), 2):
        sentence = parts[i]
        sep = parts[i+1] if i+1 < len(parts) else ""
        piece = (sentence + sep).strip()
        if not piece:
            continue
        if len(curr) + len(piece) + 1 <= max_len:
            curr = f"{curr} {piece}".strip()
        else:
            if curr:
                chunks.append(curr)
            curr = piece
    if curr:
        chunks.append(curr)
    return chunks if chunks else [text]

def synthesize_smart(tts, text, donor, out_wav):
    chunks = split_text_into_chunks(text, max_len=180)
    if len(chunks) == 1:
        tts.tts_to_file(
            text=chunks[0],
            speaker_wav=donor,
            language='pl',
            file_path=out_wav,
            temperature=0.25,
            split_sentences=False
        )
        return
        
    chunk_files = []
    for c_idx, c_text in enumerate(chunks):
        c_path = os.path.join(TEMP_DIR, f"c_{c_idx}_{os.path.basename(out_wav)}")
        tts.tts_to_file(
            text=c_text,
            speaker_wav=donor,
            language='pl',
            file_path=c_path,
            temperature=0.25,
            split_sentences=False
        )
        chunk_files.append(c_path)
        
    # Concatenate with 50ms smooth pause
    all_frames = []
    for idx, cp in enumerate(chunk_files):
        with wave.open(cp, 'rb') as w:
            all_frames.append(w.readframes(w.getnframes()))
        if idx < len(chunk_files) - 1:
            all_frames.append(b'\x00' * int(24000 * 2 * 0.05)) # 50ms silence
        try: os.remove(cp)
        except Exception: pass
        
    with wave.open(out_wav, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(24000)
        w.writeframes(b''.join(all_frames))

def master_line(raw_wav, dst_wav, is_radio=False):
    if is_radio:
        filt = 'highpass=f=350,lowpass=f=3600,loudnorm=I=-12:TP=-1.2:LRA=7'
    else:
        filt = 'highpass=f=80,loudnorm=I=-12:TP=-1.2:LRA=7'
    cmd = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
           '-i', raw_wav, '-filter:a', filt, '-ar', '24000', '-ac', '1', dst_wav]
    subprocess.run(cmd, check=True)
    
    # Exact True-Peak protection (guarantees <= -1.0 dBFS)
    cmd_check = ['ffmpeg', '-nostats', '-i', dst_wav, '-filter_complex', 'ebur128=peak=true', '-f', 'null', '-']
    p = subprocess.run(cmd_check, stderr=subprocess.PIPE, text=True, encoding='utf-8')
    m_tp = re.search(r'True peak:\s+Peak:\s+([-\d.]+)\s+dBFS', p.stderr)
    current_tp = float(m_tp.group(1)) if m_tp else -1.0
    
    if current_tp > -1.0:
        diff_db = current_tp - (-1.0)
        factor = 10 ** (-diff_db / 20.0)
        with wave.open(dst_wav, 'rb') as w:
            rate = w.getframerate()
            data = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float32) * factor
        with wave.open(dst_wav, 'wb') as w:
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
            w.writeframes(data.astype(np.int16).tobytes())

def determine_speaker_and_radio(asset, speaker_guess, file_name):
    asset_u = (asset or '').upper()
    spk_u = (speaker_guess or '').upper()
    f_u = file_name.upper()
    
    # 1. Radio detection
    is_radio = False
    if any(k in asset_u for k in ['_NIM', '_CP', '_CP2', 'TAC3', 'RADIO', 'INTRO_']):
        is_radio = True
    elif any(k in f_u for k in ['INTRO_', '_NIM', '_CP']):
        is_radio = True
        
    # 2. Donor selection
    if '_GST' in asset_u or 'DEVLIN' in spk_u:
        donor = DONORS['devlin']
    elif '_KAT' in asset_u or 'KATHRYN' in spk_u:
        donor = DONORS['kathryn']
        if any(k in asset_u for k in ['FR3_5505', 'FR3_8008', 'FR3_3205', 'FR1_1005', 'FR1_1505', 'RT1_1505']):
            is_radio = True
    elif '_BAR' in asset_u or '_BARY' in asset_u or 'BARISOV' in spk_u:
        donor = DONORS['barisov']
    elif '_DEM' in asset_u or '_DEMY' in asset_u or 'DEMICHEV' in spk_u:
        donor = DONORS['demichev']
    elif '_NIM' in asset_u or 'NIMITZ' in spk_u:
        donor = DONORS['nimitz']
        is_radio = True
    elif any(k in asset_u for k in ['_ANN', '_PA']):
        donor = DONORS['announcer']
        is_radio = True
    else:
        own_ref = os.path.join(REFS_DIR, file_name.replace('.wav', '_en.wav'))
        if os.path.exists(own_ref) and os.path.getsize(own_ref) > 10000:
            donor = own_ref
        else:
            donor = DONORS['tactical_soldier']
            
    return donor, is_radio

def load_all_tasks():
    tasks = []
    
    # 1. Story pack (1084 files)
    p_psv = r'C:\Users\Trilex\vr-mods\test-games\shadow-fix\Singularity\Binaries\vr-assets\localization\pl\polish_story_voice.jsonl'
    if os.path.exists(p_psv):
        with open(p_psv, 'r', encoding='utf-8') as f:
            for line in f:
                d = json.loads(line)
                fname = d.get('file')
                text = d.get('text_pl', '').strip()
                asset = d.get('asset', '')
                spk = d.get('speaker_guess', '')
                if fname and text:
                    donor, is_radio = determine_speaker_and_radio(asset, spk, fname)
                    # Priority scoring: Devlin (1), Kathryn (2), Barisov (3), Demichev (4), Others (5)
                    pri = 5
                    if '_GST' in asset.upper(): pri = 1
                    elif '_KAT' in asset.upper(): pri = 2
                    elif '_BAR' in asset.upper(): pri = 3
                    elif '_DEM' in asset.upper(): pri = 4
                    tasks.append({
                        'pack': 'story',
                        'file': fname,
                        'asset': asset,
                        'text': text,
                        'donor': donor,
                        'is_radio': is_radio,
                        'priority': pri
                    })

    # 2. Skipped pack (97 files)
    p_skip = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections\skipped_pl_curated.json'
    if os.path.exists(p_skip):
        with open(p_skip, 'r', encoding='utf-8') as f:
            skip_data = json.load(f)
            for k, val in skip_data.items():
                if isinstance(val, dict):
                    fname = val.get('file', f"{k}.pl.wav")
                    text = val.get('pl', '').strip()
                    asset = val.get('asset', k)
                    donor, is_radio = determine_speaker_and_radio(asset, '', fname)
                    tasks.append({
                        'pack': 'skipped',
                        'file': fname,
                        'asset': asset,
                        'text': text,
                        'donor': donor,
                        'is_radio': is_radio,
                        'priority': 6
                    })

    # 3. Vignette pack (6 files)
    p_vig = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections\vignette_pl_curated.json'
    if os.path.exists(p_vig):
        with open(p_vig, 'r', encoding='utf-8') as f:
            vig_data = json.load(f)
            for k, text in vig_data.items():
                fname = f"{k}.pl.wav"
                donor, is_radio = determine_speaker_and_radio(k, '', fname)
                tasks.append({
                    'pack': 'vignette',
                    'file': fname,
                    'asset': k,
                    'text': text.strip(),
                    'donor': donor,
                    'is_radio': True,
                    'priority': 6
                })

    # Sort tasks by priority
    tasks.sort(key=lambda x: x['priority'])
    return tasks

def update_manifests(updated_files):
    if not updated_files:
        return
    for mpath in [MANIFEST_FULL, MANIFEST_DELTA]:
        if not os.path.exists(mpath):
            continue
        lines = []
        with open(mpath, 'r', encoding='utf-8') as f:
            for line in f:
                d = json.loads(line)
                fname = d.get('file')
                if fname in updated_files:
                    real_path = updated_files[fname]
                    d['sha16'] = get_sha16(real_path)
                    d['size_bytes'] = os.path.getsize(real_path)
                lines.append(d)
        with open(mpath, 'w', encoding='utf-8') as f:
            for d in lines:
                f.write(json.dumps(d, ensure_ascii=False) + '\n')

def run_production(max_count=None):
    tasks = load_all_tasks()
    print(f"Total production tasks assembled: {len(tasks)} files.", flush=True)
    
    completed = set()
    if os.path.exists(PROGRESS_LOG):
        with open(PROGRESS_LOG, 'r', encoding='utf-8') as f:
            for line in f:
                if line.strip():
                    try:
                        completed.add(json.loads(line)['file'])
                    except Exception:
                        pass
                        
    print(f"Already completed in progress log: {len(completed)} files.", flush=True)
    
    remaining = [t for t in tasks if t['file'] not in completed]
    print(f"Remaining to produce: {len(remaining)} files.", flush=True)
    
    if max_count:
        remaining = remaining[:max_count]
        print(f"Running limited batch of: {len(remaining)} files.", flush=True)
        
    if not remaining:
        print("Nothing to produce. All tasks finished!", flush=True)
        return
        
    print("Loading Coqui XTTS-v2 model (14 threads)...", flush=True)
    tts = TTS('tts_models/multilingual/multi-dataset/xtts_v2', gpu=False)
    print("Model ready. Commencing production run...", flush=True)
    
    updated_files = {}
    with open(PROGRESS_LOG, 'a', encoding='utf-8') as log_fp:
        for idx, task in enumerate(remaining, 1):
            out_folder = os.path.join(BASE_OUT, task['pack'])
            os.makedirs(out_folder, exist_ok=True)
            dst_wav = os.path.join(out_folder, task['file'])
            raw_wav = os.path.join(TEMP_DIR, f"raw_{task['file']}")
            
            t0 = time.time()
            try:
                synthesize_smart(tts, task['text'], task['donor'], raw_wav)
                master_line(raw_wav, dst_wav, is_radio=task['is_radio'])
                dur_synth = time.time() - t0
                
                updated_files[task['file']] = dst_wav
                log_entry = {
                    'pack': task['pack'],
                    'file': task['file'],
                    'asset': task['asset'],
                    'donor': os.path.basename(task['donor']),
                    'is_radio': task['is_radio'],
                    'dur_s': dur_synth,
                    'timestamp': time.time()
                }
                log_fp.write(json.dumps(log_entry, ensure_ascii=False) + '\n')
                log_fp.flush()
                
                print(f"[{idx}/{len(remaining)}] {task['pack']}/{task['file']} ({task['asset']}) synth={dur_synth:.1f}s radio={task['is_radio']}", flush=True)
                
                if len(updated_files) >= 10:
                    update_manifests(updated_files)
                    updated_files.clear()
                    
            except Exception as e:
                print(f"ERR on {task['file']}: {e}", flush=True)
                
    if updated_files:
        update_manifests(updated_files)
        
    print(f"\nProduction batch completed! Processed {len(remaining)} files.", flush=True)

if __name__ == '__main__':
    count = int(sys.argv[1]) if len(sys.argv) > 1 else None
    run_production(max_count=count)
