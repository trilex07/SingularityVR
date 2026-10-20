"""Comprehensive audio audit of C:\\Dubbing-aktualny:
Checks duration, loudness, peak, runaway files, and runs ASR on suspicious files.
"""
import os, sys, time, json, math, wave
import numpy as np, miniaudio

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.whisper_tools')
from faster_whisper import WhisperModel

DUB_ROOT = r'C:\Dubbing-aktualny'
EN_BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
REPO_ROOT = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
OUT_REPORT = os.path.join(REPO_ROOT, 'dubbing-corrections', 'global_audio_audit_report.json')

# Build English reference map: filename lower -> full path
print('Indexing English reference audio...', flush=True)
en_map = {}
for root, dirs, files in os.walk(EN_BASE):
    for f in files:
        if f.lower().endswith('.ogg'):
            stem = os.path.splitext(f)[0].lower()
            en_map[stem] = os.path.join(root, f)
print(f'Indexed {len(en_map)} English reference files.', flush=True)

# Build story metadata map from localization polish_story_voice.jsonl
story_meta = {}
loc_meta_path = os.path.join(REPO_ROOT, 'localization', 'pl', 'polish_story_voice.jsonl')
if os.path.exists(loc_meta_path):
    with open(loc_meta_path, 'r', encoding='utf-8') as f:
        for line in f:
            if line.strip():
                try:
                    d = json.loads(line)
                    story_meta[d['file']] = d
                except Exception:
                    pass
print(f'Loaded {len(story_meta)} metadata records for story.', flush=True)

packs = ['intro', 'vignette', 'skipped', 'barks', 'story', 'mp']
all_results = {}
suspicious_files = []

for pack in packs:
    folder = os.path.join(DUB_ROOT, pack)
    if not os.path.exists(folder):
        continue
    wav_files = [f for f in sorted(os.listdir(folder)) if f.lower().endswith('.wav')]
    print(f'Auditing pack: {pack} ({len(wav_files)} files)...', flush=True)
    
    pack_stats = {
        'total': len(wav_files),
        'flagged': 0,
        'files': []
    }
    
    for f in wav_files:
        path = os.path.join(folder, f)
        stem = f.replace('.pl.wav', '').replace('.wav', '').lower()
        
        # Audio properties
        try:
            with wave.open(path, 'rb') as w:
                frames = w.getnframes()
                rate = w.getframerate()
                dur = frames / rate
                raw = w.readframes(frames)
                s = np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768.0
                peak = float(np.max(np.abs(s))) if len(s) else 0.0
                rms = float(np.sqrt(np.mean(s**2))) if len(s) else 0.0
                peak_db = 20 * math.log10(peak) if peak > 1e-9 else -99.0
                rms_db = 20 * math.log10(rms) if rms > 1e-9 else -99.0
        except Exception as e:
            pack_stats['files'].append({'file': f, 'error': str(e)})
            continue
            
        # Check against English duration if available
        dur_en = None
        rms_en_db = None
        en_path = en_map.get(stem)
        if not en_path and f in story_meta:
            src = story_meta[f].get('source')
            if src:
                p = os.path.join(EN_BASE, src)
                if os.path.exists(p):
                    en_path = p
                    
        if en_path and os.path.exists(en_path):
            try:
                dec = miniaudio.decode_file(en_path)
                en_samples = np.array(dec.samples, dtype=np.float32) / 32768.0
                dur_en = len(en_samples) / dec.sample_rate / dec.nchannels
                rms_en = float(np.sqrt(np.mean(en_samples**2)))
                rms_en_db = 20 * math.log10(rms_en) if rms_en > 1e-9 else -99.0
            except Exception:
                pass
                
        # Flagging logic
        flags = []
        if peak_db > -0.8:
            flags.append('clipping')
        if rms_db < -35.0:
            flags.append('too_quiet')
            
        # Runaway duration detection
        if pack in ('barks', 'mp') and dur > 8.0:
            flags.append(f'excessive_bark_dur_{dur:.1f}s')
        elif pack in ('intro', 'vignette') and dur > 15.0:
            flags.append(f'excessive_intro_dur_{dur:.1f}s')
        elif dur_en and dur > max(4.0, dur_en * 1.6):
            flags.append(f'dur_ratio_{dur/dur_en:.2f}x')
            
        if rms_en_db is not None and abs(rms_db - rms_en_db) > 4.5:
            flags.append(f'rms_delta_{rms_db - rms_en_db:+.1f}dB')
            
        item = {
            'pack': pack,
            'file': f,
            'path': path,
            'dur_pl': round(dur, 2),
            'dur_en': round(dur_en, 2) if dur_en else None,
            'peak_db': round(peak_db, 1),
            'rms_db': round(rms_db, 1),
            'rms_en_db': round(rms_en_db, 1) if rms_en_db is not None else None,
            'flags': flags
        }
        
        if flags:
            pack_stats['flagged'] += 1
            suspicious_files.append(item)
        pack_stats['files'].append(item)
        
    all_results[pack] = pack_stats
    print(f'Pack {pack}: {pack_stats["flagged"]}/{pack_stats["total"]} flagged.', flush=True)

print(f'\nTotal suspicious files across all packs: {len(suspicious_files)}', flush=True)

# Run Whisper ASR on suspicious files to detect hallucinations
print('\nLoading Faster-Whisper small model to verify suspicious files...', flush=True)
asr = WhisperModel('small', device='cpu', compute_type='int8')

asr_checked = 0
asr_hallucinations = []

for item in suspicious_files:
    # Only run ASR if flagged for duration or loudness
    wav_path = item['path']
    try:
        dec = miniaudio.decode_file(wav_path, output_format=miniaudio.SampleFormat.FLOAT32)
        s = np.array(dec.samples, dtype=np.float32)
        idx = np.linspace(0, len(s) - 1, int(len(s) * 16000 / dec.sample_rate))
        x16 = np.interp(idx, np.arange(len(s)), s)
        segs, _ = asr.transcribe(x16, language='pl')
        transcription = ' '.join(seg.text for seg in segs).strip()
        item['asr_text'] = transcription
        asr_checked += 1
        
        # Check for known hallucination patterns
        words = transcription.split()
        is_loop = False
        if len(words) >= 4:
            for w in set(words):
                if words.count(w) >= 5:
                    is_loop = True
                    break
                    
        is_amara = 'amara' in transcription.lower()
        is_empty = len(transcription) == 0
        
        if is_loop or is_amara or is_empty:
            item['hallucination'] = True
            item['hallucination_reason'] = 'loop' if is_loop else ('amara' if is_amara else 'empty')
            asr_hallucinations.append(item)
            print(f'HALLUCINATION in {item["pack"]}/{item["file"]}: [{item["hallucination_reason"]}] "{transcription[:60]}"', flush=True)
    except Exception as e:
        item['asr_error'] = str(e)

print(f'\nASR verification finished: {len(asr_hallucinations)} confirmed hallucinations detected.', flush=True)

with open(OUT_REPORT, 'w', encoding='utf-8') as f:
    json.dump({
        'summary': {pack: {'total': all_results[pack]['total'], 'flagged': all_results[pack]['flagged']} for pack in packs},
        'total_files': sum(all_results[p]['total'] for p in packs),
        'total_flagged': len(suspicious_files),
        'total_hallucinations': len(asr_hallucinations),
        'hallucinations': asr_hallucinations,
        'all_suspicious': suspicious_files
    }, f, ensure_ascii=False, indent=2)

print(f'Full audit report saved to: {OUT_REPORT}', flush=True)
