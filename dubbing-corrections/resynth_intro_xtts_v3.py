"""Synthesize 11 Intro Helicopter lines using XTTS-v2:
Authentic Cast:
- Nimitz: nimitz_combined_donor.wav
- Devlin: d4b361c6a632d1f5_en.wav
- Pilot: mc_speech_sample1.wav (clean military pilot tone)

Mastering:
- Military Radio Filter (highpass=350, lowpass=3600)
- Loudness: -12.0 to -12.5 LUFS
- True-Peak: <= -1.0 dBFS
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

REFS_DIR = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual\_refs_en'
OUT_DIR = r'C:\Dubbing-aktualny\intro'
TEMP_DIR = r'C:\Users\Trilex\AppData\Local\Temp\intro_xtts_v3'
os.makedirs(TEMP_DIR, exist_ok=True)

DONOR_DEVLIN = os.path.join(REFS_DIR, 'd4b361c6a632d1f5_en.wav')
DONOR_NIMITZ = r'C:\Users\Trilex\AppData\Local\Temp\nimitz_combined_donor.wav'
DONOR_PILOT = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\cast_legal_20261014\internet_candidates\mc_speech_sample1.wav'

INTRO_LINES = [
    {
        'file': 'INTRO_1005_NIM.pl.wav',
        'donor': DONOR_NIMITZ,
        'text': 'Zespół Spartan, tu dowódca Nimitz. Wkraczacie w rosyjską przestrzeń powietrzną. Zachować szczególną ostrożność.'
    },
    {
        'file': 'INTRO_1005_NIM_alt.pl.wav',
        'donor': DONOR_NIMITZ,
        'text': 'Zespół Spartan, tu baza. Wkraczacie w rosyjską przestrzeń powietrzną. Zachować szczególną ostrożność.'
    },
    {
        'file': 'INTRO_1006_CP.pl.wav',
        'donor': DONOR_PILOT,
        'text': 'Zrozumiałem, Nimitz. Utrzymujemy kurs. Do celu trzy minuty.'
    },
    {
        'file': 'INTRO_1007_GST.pl.wav',
        'donor': DONOR_DEVLIN,
        'text': 'Dowództwo musi mieć pełne gacie, skoro ryzykują międzynarodowy incydent, wysyłając nas w ten sposób.'
    },
    {
        'file': 'INTRO_1008_GST.pl.wav',
        'donor': DONOR_DEVLIN,
        'text': 'Wywiad twierdzi, że ta dziura jest opuszczona od pięćdziesięciu lat, ale wiecie, co mówią o wywiadzie wojskowym. Miejcie oczy dookoła głowy.'
    },
    {
        'file': 'INTRO_1009_NIM.pl.wav',
        'donor': DONOR_NIMITZ,
        'text': 'Zespół Spartan, utrzymać obecną prędkość i kurs.'
    },
    {
        'file': 'INTRO_1010_CP.pl.wav',
        'donor': DONOR_PILOT,
        'text': 'Przyjąłem, Nimitz.'
    },
    {
        'file': 'INTRO_1011_GST.pl.wav',
        'donor': DONOR_DEVLIN,
        'text': 'Cholera... Widzicie to?'
    },
    {
        'file': 'INTRO_1012_NIM.pl.wav',
        'donor': DONOR_NIMITZ,
        'text': 'Spartan dwa-jeden, Spartan dwa-jeden! Mamy potężny skok energii na waszym wektorze!'
    },
    {
        'file': 'INTRO_1014_GST.pl.wav',
        'donor': DONOR_DEVLIN,
        'text': 'To wygląda jak cholerny impuls EMP! Trzymaj się, Renko! Zaraz rąbniemy!'
    },
    {
        'file': 'INTRO_1016_CP.pl.wav',
        'donor': DONOR_PILOT,
        'text': 'Dostaliśmy! Dostaliśmy! Mayday, mayday, mayday! Spadamy!'
    }
]

def apply_radio_loudnorm(raw_wav, dst_wav):
    # Two-stage: bandpass + loudnorm to -12 LUFS with TP <= -1.0 dBFS
    filt = 'highpass=f=350,lowpass=f=3600,volume=2.0dB,loudnorm=I=-12:TP=-1.0:LRA=7'
    cmd = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
           '-i', raw_wav, '-filter:a', filt, '-ar', '24000', '-ac', '1', dst_wav]
    subprocess.run(cmd, check=True)

def main():
    print("Loading XTTS-v2 model for Intro Helicopter resynthesis...", flush=True)
    tts = TTS('tts_models/multilingual/multi-dataset/xtts_v2', gpu=False)
    print("Model loaded. Starting synthesis of 11 intro lines...", flush=True)
    
    for idx, item in enumerate(INTRO_LINES, 1):
        raw = os.path.join(TEMP_DIR, f"raw_{item['file']}")
        dst = os.path.join(OUT_DIR, item['file'])
        
        print(f"[{idx}/11] Synthesizing {item['file']}...", flush=True)
        t0 = time.time()
        tts.tts_to_file(
            text=item['text'],
            speaker_wav=item['donor'],
            language='pl',
            file_path=raw,
            temperature=0.25,
            split_sentences=False
        )
        print(f"   Generated in {time.time()-t0:.1f}s. Applying military radio filter...", flush=True)
        apply_radio_loudnorm(raw, dst)
        print(f"   Saved to {dst}", flush=True)

    print("\nAll 11 intro helicopter lines synthesized and mastered!", flush=True)

if __name__ == '__main__':
    main()
