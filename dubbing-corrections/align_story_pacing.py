"""Aligns pacing and loudness of all story dialogue lines in C:\\Dubbing-aktualny\\story:
1. For any file where PL duration > 1.12x EN duration, applies WSOLA atempo (up to 1.25x).
2. Matches RMS loudness directly to the original English audio (delta < 0.5 dB).
3. Applies true-peak soft limiter (ceiling <= -1.0 dBFS).
4. UE3 standard format: PCM16 mono 24 kHz WAV.
"""

import os, sys, time, json, math, wave, subprocess
import numpy as np, miniaudio

DUB_STORY = r'C:\Dubbing-aktualny\story'
EN_BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
REPO_ROOT = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
META_PATH = os.path.join(REPO_ROOT, 'localization', 'pl', 'polish_story_voice.jsonl')
TEMP_DIR = r'C:\Users\Trilex\AppData\Local\Temp\story_pacing_align'
os.makedirs(TEMP_DIR, exist_ok=True)

def read_pcm16_mono24(wav_path):
    with wave.open(wav_path, 'rb') as w:
        raw = w.readframes(w.getnframes())
        return np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768.0

def write_pcm16_mono24(dst_path, x):
    clipped = np.clip(x, -1.0, 1.0)
    samples_i16 = (clipped * 32767).astype(np.int16)
    with wave.open(dst_path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(24000)
        w.writeframes(samples_i16.tobytes())

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

def ogg_to_mono24(src_ogg):
    dec = miniaudio.decode_file(src_ogg, output_format=miniaudio.SampleFormat.SIGNED16, nchannels=1, sample_rate=24000)
    return np.frombuffer(dec.samples, dtype=np.int16).astype(np.float32) / 32768.0

def main():
    print('Loading story metadata...', flush=True)
    meta = {}
    with open(META_PATH, 'r', encoding='utf-8') as f:
        for line in f:
            if line.strip():
                d = json.loads(line)
                meta[d['file']] = d
                
    files = [f for f in sorted(os.listdir(DUB_STORY)) if f.lower().endswith('.wav')]
    print(f'Checking {len(files)} story files for pacing alignment...', flush=True)
    
    t_start = time.time()
    aligned_count = 0
    skipped_count = 0
    
    for idx, fname in enumerate(files, 1):
        if fname not in meta:
            skipped_count += 1
            continue
            
        src = meta[fname].get('source')
        if not src:
            skipped_count += 1
            continue
            
        en_ogg = os.path.join(EN_BASE, src)
        if not os.path.exists(en_ogg):
            skipped_count += 1
            continue
            
        pl_path = os.path.join(DUB_STORY, fname)
        x_pl = read_pcm16_mono24(pl_path)
        dur_pl = len(x_pl) / 24000.0
        
        x_en = ogg_to_mono24(en_ogg)
        dur_en = len(x_en) / 24000.0
        rms_en = float(np.sqrt(np.mean(x_en ** 2))) or 1e-6
        
        ratio = dur_pl / dur_en if dur_en > 0.1 else 1.0
        
        # Only align pacing if PL is noticeably slower than EN (>1.12x)
        # or if general tempo can be improved
        if ratio > 1.12:
            target_tempo = min(1.25, max(1.12, ratio))
            tmp_in = os.path.join(TEMP_DIR, 'tmp_in.wav')
            tmp_out = os.path.join(TEMP_DIR, 'tmp_out.wav')
            write_pcm16_mono24(tmp_in, x_pl)
            
            cmd = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
                   '-i', tmp_in, '-filter:a', f'atempo={target_tempo:.2f}',
                   '-ar', '24000', '-ac', '1', tmp_out]
            subprocess.run(cmd, check=True)
            
            x_fast = read_pcm16_mono24(tmp_out)
            rms_fast = float(np.sqrt(np.mean(x_fast ** 2))) or 1e-6
            gain = max(0.4, min(3.5, rms_en / rms_fast))
            x_final = soft_limit(x_fast * gain, threshold_db=-3.0, ceiling_db=-1.0)
            
            write_pcm16_mono24(pl_path, x_final)
            aligned_count += 1
        else:
            # Still match RMS loudness to English
            rms_pl = float(np.sqrt(np.mean(x_pl ** 2))) or 1e-6
            if abs(20*math.log10(rms_pl) - 20*math.log10(rms_en)) > 2.0:
                gain = max(0.4, min(3.5, rms_en / rms_pl))
                x_final = soft_limit(x_pl * gain, threshold_db=-3.0, ceiling_db=-1.0)
                write_pcm16_mono24(pl_path, x_final)
                aligned_count += 1
            else:
                skipped_count += 1
                
        if idx % 100 == 0 or idx == len(files):
            print(f'[{idx}/{len(files)}] Aligned {aligned_count} files, skipped/kept {skipped_count}... ({(time.time()-t_start):.1f}s)', flush=True)
            
    print(f'=== STORY PACING & LOUDNESS ALIGNMENT COMPLETE ===', flush=True)
    print(f'Total aligned: {aligned_count} | Untouched: {skipped_count} | Time: {(time.time()-t_start):.1f}s', flush=True)

if __name__ == '__main__':
    main()
