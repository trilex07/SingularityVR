"""Generate silky-smooth, broadcast-quality XTTS-v2 samples:
Fixes:
1. split_sentences=False (eliminates all comma/period micro-cuts and concatenation clicks).
2. temperature=0.25 (eliminates vocoder jitter, phase buzzing, and crackles).
3. Pristine studio reference audio (zero helicopter/static bleed).
4. Smooth EBU R128 loudnorm (integrated -14 LUFS, TP -1.0 dBFS, zero harsh compressor pumping).
Outputs to C:\\Dubbing-aktualny\\samples_xtts_v2\\
"""

import os, sys, time, wave, subprocess
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.xtts_tools')
import numpy as np, soundfile as sf
import torch, torchaudio

def _load_via_soundfile(uri, *a, **k):
    data, sr = sf.read(uri, always_2d=False)
    arr = np.asarray(data, dtype=np.float32)
    t = torch.from_numpy(arr).unsqueeze(0) if arr.ndim == 1 else torch.from_numpy(arr.T.copy())
    return t, sr

torchaudio.load = _load_via_soundfile
os.environ['COQUI_TOS_AGREED'] = '1'

from TTS.api import TTS

REPO = r'C:\Users\Trilex\vr-mods'
OUT_DIR = r'C:\Dubbing-aktualny\samples_xtts_v2'
os.makedirs(OUT_DIR, exist_ok=True)
TEMP_DIR = r'C:\Users\Trilex\AppData\Local\Temp\xtts_v2_smooth'
os.makedirs(TEMP_DIR, exist_ok=True)

SAMPLES = [
    {
        'id': 'sample1_nimitz_smooth',
        'character': 'Dowódca Nimitz',
        'text': 'Zespół Spartan, tu dowódca Nimitz. Wkraczacie w rosyjską przestrzeń powietrzną. Zachować szczególną ostrożność.',
        'donor': r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\mailabs_m_loud.wav',
    },
    {
        'id': 'sample2_devlin_smooth',
        'character': 'Devlin (Żołnierz)',
        'text': 'Wywiad twierdzi, że ta dziura jest opuszczona od pięćdziesięciu lat, ale wiecie, co mówią o wywiadzie wojskowym. Miejcie oczy dookoła głowy.',
        'donor': os.path.join(REPO, r'SingularityVR-cutscene\audio\polish\cast_legal_20261014\internet_candidates\mc_speech_sample1.wav'),
    },
    {
        'id': 'sample3_kathryn_smooth',
        'character': 'Kathryn (Agentka)',
        'text': 'Doktorze Barisow, słyszy mnie pan? Proszę się trzymać, jesteśmy już w drodze.',
        'donor': os.path.join(REPO, r'SingularityVR-cutscene\audio\polish\cast_legal_20261014\internet_candidates\commons_anna_bugajska.mp3'),
    }
]

def prepare_donor(src, dst):
    cmd = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
           '-i', src, '-t', '10.0', '-ar', '24000', '-ac', '1', dst]
    subprocess.run(cmd, check=True)
    return dst

def master_loudnorm(src_wav, dst_wav, is_radio=False):
    if is_radio:
        filt = 'highpass=f=350,lowpass=f=3600,loudnorm=I=-14:TP=-1.0:LRA=7'
    else:
        filt = 'highpass=f=80,loudnorm=I=-14:TP=-1.0:LRA=7'
    cmd = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
           '-i', src_wav, '-filter:a', filt, '-ar', '24000', '-ac', '1', dst_wav]
    subprocess.run(cmd, check=True)

def main():
    print('Loading Coqui XTTS-v2...', flush=True)
    tts = TTS('tts_models/multilingual/multi-dataset/xtts_v2', gpu=False)
    print('Model ready.', flush=True)
    
    for s in SAMPLES:
        print(f"\n--- Synthesizing smooth: {s['character']} ---", flush=True)
        donor_wav = os.path.join(TEMP_DIR, f"{s['id']}_donor.wav")
        prepare_donor(s['donor'], donor_wav)
        
        raw_wav = os.path.join(TEMP_DIR, f"{s['id']}_raw.wav")
        t0 = time.time()
        # Crucial parameters: temperature=0.25 and split_sentences=False
        tts.tts_to_file(
            text=s['text'],
            speaker_wav=donor_wav,
            language='pl',
            file_path=raw_wav,
            temperature=0.25,
            split_sentences=False
        )
        print(f"Synthesized in {time.time()-t0:.1f}s.", flush=True)
        
        # 1. Clean Studio Master
        out_clean = os.path.join(OUT_DIR, f"{s['id']}_studio.wav")
        master_loudnorm(raw_wav, out_clean, is_radio=False)
        
        # 2. Radio Comms Master
        out_radio = os.path.join(OUT_DIR, f"{s['id']}_radio.wav")
        master_loudnorm(raw_wav, out_radio, is_radio=True)
        
        for path, tag in [(out_clean, 'Studio'), (out_radio, 'Radio')]:
            with wave.open(path, 'rb') as w:
                n = w.getnframes()
                raw = w.readframes(n)
                arr = np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768.0
                peak = float(np.max(np.abs(arr)))
                rms = float(np.sqrt(np.mean(arr**2)))
                diff = np.max(np.abs(np.diff(arr)))
                print(f"  {tag:6s}: {os.path.basename(path)} | dur={n/24000:.2f}s | RMS={20*np.log10(rms):.1f}dB | Peak={20*np.log10(peak):.1f}dB | max_step={diff:.3f}", flush=True)

    print(f"\nAll v2 smooth samples saved to: {OUT_DIR}", flush=True)

if __name__ == '__main__':
    main()
