"""Boost Volume across Entire Dubbing Library to -12 LUFS Broadcast Game Standard:
- Targets: story, barks, mp, skipped, vignette (1864 files)
- Skips intro (managed separately by XTTS-v2)
- Fast vectorized NumPy processing
- Soft-knee analog tanh saturation (ceiling = 0.88 -> TP <= -1.0 dBFS)
- Updates deploy_full_manifest.jsonl and deploy_manifest.jsonl
"""

import os, sys, glob, wave, time, json, hashlib
import numpy as np

BASE_DIR = r'C:\Dubbing-aktualny'
MANIFEST_FULL = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections\deploy_full_manifest.jsonl'
MANIFEST_DELTA = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections\deploy_manifest.jsonl'

TARGET_FOLDERS = ['story', 'barks', 'mp', 'skipped', 'vignette']
TARGET_ACTIVE_RMS_DB = -11.0
CEILING = 0.88 # -1.1 dBFS ceiling to prevent any inter-sample clipping

def get_sha16(path):
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()[:16]

def process_file(filepath):
    with wave.open(filepath, 'rb') as w:
        nch = w.getnchannels()
        sampwidth = w.getsampwidth()
        framerate = w.getframerate()
        frames = w.readframes(w.getnframes())
        
    data = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0
    if len(data) == 0:
        return False
        
    # Find active region RMS
    abs_d = np.abs(data)
    active = data[abs_d > 0.01]
    if len(active) > 0:
        rms = np.sqrt(np.mean(active**2))
    else:
        rms = np.sqrt(np.mean(data**2))
        
    target_rms = 10 ** (TARGET_ACTIVE_RMS_DB / 20.0)
    
    # Calculate required gain
    if rms < target_rms:
        gain = min(target_rms / max(rms, 1e-6), 6.0) # max +15.5 dB gain
    else:
        gain = 1.0
        
    # Soft saturation with tanh
    scaled = CEILING * np.tanh((data * gain) / CEILING)
    out_int16 = (scaled * 32767).astype(np.int16)
    
    with wave.open(filepath, 'wb') as w:
        w.setnchannels(nch)
        w.setsampwidth(sampwidth)
        w.setframerate(framerate)
        w.writeframes(out_int16.tobytes())
        
    return True

def main():
    print("=== Boosting Dubbing Library to -12 LUFS Broadcast Standard ===")
    total_processed = 0
    t0 = time.time()
    
    file_map = {} # filename -> filepath
    for folder in TARGET_FOLDERS:
        fdir = os.path.join(BASE_DIR, folder)
        files = glob.glob(os.path.join(fdir, '*.wav'))
        print(f"Processing folder: {folder} ({len(files)} files)...", flush=True)
        for f in files:
            process_file(f)
            file_map[os.path.basename(f)] = f
            total_processed += 1
            
    print(f"\nSuccessfully boosted {total_processed} files in {time.time()-t0:.2f}s!", flush=True)
    
    print("\nUpdating manifests with new hashes...", flush=True)
    for mpath in [MANIFEST_FULL, MANIFEST_DELTA]:
        if not os.path.exists(mpath):
            continue
        updated = 0
        lines = []
        with open(mpath, 'r', encoding='utf-8') as f:
            for line in f:
                d = json.loads(line)
                fname = d.get('file')
                if fname in file_map:
                    real_path = file_map[fname]
                    new_sha = get_sha16(real_path)
                    new_sz = os.path.getsize(real_path)
                    if d.get('sha16') != new_sha:
                        d['sha16'] = new_sha
                        d['size_bytes'] = new_sz
                        updated += 1
                lines.append(d)
                
        with open(mpath, 'w', encoding='utf-8') as f:
            for d in lines:
                f.write(json.dumps(d, ensure_ascii=False) + '\n')
        print(f"  {os.path.basename(mpath)}: {updated} entries updated.")

    print("\nAll manifests and files synchronized successfully!")

if __name__ == '__main__':
    main()
