"""Generate high-quality comparison samples using Coqui XTTS-v2:
1. Nimitz (Commander) - INTRO_1005_NIM
2. Devlin (USMC soldier) - INTRO_1008_GST
3. Kathryn (Female agent) - FR3_8008_KAT
Outputs both clean broadcast-mastered WAV and tactical Radio-FX WAV into C:\\Dubbing-aktualny\\samples_xtts\\
"""

import os, sys, time, wave, subprocess
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.xtts_tools')
import numpy as np, soundfile as sf
import torch, torchaudio

# Monkey-patch torchaudio.load to use soundfile (avoids Windows FFmpeg DLL issue)
def _load_via_soundfile(uri, *a, **k):
    data, sr = sf.read(uri, always_2d=False)
    arr = np.asarray(data, dtype=np.float32)
    t = torch.from_numpy(arr).unsqueeze(0) if arr.ndim == 1 else torch.from_numpy(arr.T.copy())
    return t, sr

torchaudio.load = _load_via_soundfile
os.environ['COQUI_TOS_AGREED'] = '1'

from TTS.api import TTS

EN_BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
OUT_DIR = r'C:\Dubbing-aktualny\samples_xtts'
os.makedirs(OUT_DIR, exist_ok=True)
TEMP_DIR = r'C:\Users\Trilex\AppData\Local\Temp\xtts_samples'
os.makedirs(TEMP_DIR, exist_ok=True)

SAMPLES = [
    {
        'id': 'sample1_nimitz_INTRO_1005',
        'character': 'Dowódca Nimitz',
        'text': 'Zespół Spartan, tu dowódca Nimitz. Wkraczacie w rosyjską przestrzeń powietrzną. Zachować szczególną ostrożność.',
        'donor_ogg': os.path.join(EN_BASE, 'SP_VI_intro_Kismet_LOC_INT', 'SoundNodeWave', 'INTRO_1005_NIM.ogg'),
        'radio': True
    },
    {
        'id': 'sample2_devlin_INTRO_1008',
        'character': 'Devlin (Żołnierz)',
        'text': 'Wywiad twierdzi, że ta dziura jest opuszczona od pięćdziesięciu lat, ale wiecie, co mówią o wywiadzie wojskowym. Miejcie oczy dookoła głowy.',
        'donor_ogg': os.path.join(EN_BASE, 'SP_VI_intro_Kismet_LOC_INT', 'SoundNodeWave', 'INTRO_1008_GST.ogg'),
        'radio': False
    },
    {
        'id': 'sample3_kathryn_FR3_8008',
        'character': 'Kathryn (Agentka)',
        'text': 'Doktorze Barisow, słyszy mnie pan? Proszę się trzymać, jesteśmy już w drodze.',
        'donor_ogg': os.path.join(EN_BASE, 'SP_Freight_Script_StreamCargo2_LOC_INT', 'SoundNodeWave', 'FR3_8008_KAT.ogg'),
        'radio': True
    }
]

def prepare_donor_wav(ogg_path, out_wav):
    cmd = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
           '-i', ogg_path, '-t', '8.0', '-ar', '24000', '-ac', '1', out_wav]
    subprocess.run(cmd, check=True)
    return out_wav

def master_audio(src_wav, dst_wav, is_radio=False):
    # Professional broadcast mastering chain:
    # Highpass 80Hz + Vocal Compressor (ratio 3:1, makeup +5dB) + Peak limiter at -0.5 dBFS
    if is_radio:
        # Military radio / helmet comms EQ: bandpass 350-3600Hz + light saturation
        filt = (
            'highpass=f=350,lowpass=f=3600,'
            'compand=attacks=0.005:decays=0.05:points=-80/-80|-30/-15|-12/-4|0/-0.5:gain=4.5'
        )
    else:
        filt = (
            'highpass=f=80,'
            'compand=attacks=0.005:decays=0.06:points=-80/-80|-30/-18|-15/-6|0/-0.5:gain=5.0'
        )
    cmd = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
           '-i', src_wav, '-filter_complex', filt, '-ar', '24000', '-ac', '1', dst_wav]
    subprocess.run(cmd, check=True)

def main():
    print('Loading Coqui XTTS-v2 model (cached local weights)...', flush=True)
    t0 = time.time()
    tts = TTS('tts_models/multilingual/multi-dataset/xtts_v2', gpu=False)
    print(f'XTTS-v2 loaded in {time.time() - t0:.1f}s.', flush=True)
    
    for s in SAMPLES:
        print(f"\n--- Synthesizing: {s['character']} ({s['id']}) ---", flush=True)
        donor_wav = os.path.join(TEMP_DIR, f"{s['id']}_donor.wav")
        prepare_donor_wav(s['donor_ogg'], donor_wav)
        
        raw_xtts = os.path.join(TEMP_DIR, f"{s['id']}_raw.wav")
        t_gen = time.time()
        tts.tts_to_file(text=s['text'], speaker_wav=donor_wav, language='pl', file_path=raw_xtts)
        print(f"Generated XTTS-v2 in {time.time() - t_gen:.1f}s.", flush=True)
        
        # 1. Clean Mastered Version
        out_clean = os.path.join(OUT_DIR, f"{s['id']}_mastered.wav")
        master_audio(raw_xtts, out_clean, is_radio=False)
        
        # 2. Radio Comms Version
        out_radio = os.path.join(OUT_DIR, f"{s['id']}_radio_fx.wav")
        master_audio(raw_xtts, out_radio, is_radio=True)
        
        # Measure
        for path, tag in [(out_clean, 'Clean'), (out_radio, 'Radio')]:
            with wave.open(path, 'rb') as w:
                n = w.getnframes()
                raw = w.readframes(n)
                arr = np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768.0
                peak = float(np.max(np.abs(arr)))
                rms = float(np.sqrt(np.mean(arr**2)))
                print(f"  {tag:6s}: {os.path.basename(path)} | dur={n/24000:.2f}s | peak={20*np.log10(peak):.1f} dBFS | RMS={20*np.log10(rms):.1f} dBFS", flush=True)

    print(f"\nAll XTTS-v2 samples ready in: {OUT_DIR}", flush=True)

if __name__ == '__main__':
    main()
