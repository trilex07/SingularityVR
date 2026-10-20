"""Generate Game-Accurate Voice Clones (V3):
Cloned directly from the original English in-game voice actors of Singularity!
Characters:
1. Devlin (Renko's partner / US Marine)
2. Dr. Victor Barisov (Soviet scientist)
3. Nikolai Demichev (Soviet General / Main antagonist)
4. Kathryn (Mir-12 agent)
5. Commander Nimitz (Carrier Commander)

Loudness & Dynamics:
- Target: -11.5 LUFS (punchy, loud, broadcast game dialogue)
- True-Peak ceiling: <= -1.0 dBFS (soft-knee analog tanh saturation, zero digital clipping)
- Micro-cuts fix: split_sentences=False + temperature=0.25
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
OUT_DIR = r'C:\Dubbing-aktualny\samples_game_voices_v3'
TEMP_DIR = r'C:\Users\Trilex\AppData\Local\Temp\xtts_v3_game'
os.makedirs(OUT_DIR, exist_ok=True)
os.makedirs(TEMP_DIR, exist_ok=True)

VOICES = [
    {
        'id': 'v3_01_devlin',
        'character': 'Devlin (Partner Renko)',
        'donor': os.path.join(REFS_DIR, 'd4b361c6a632d1f5_en.wav'),
        'text': 'Wywiad twierdzi, że ta dziura jest opuszczona od pięćdziesięciu lat, ale wiecie, co mówią o wywiadzie wojskowym. Miejcie oczy dookoła głowy.',
        'has_radio': True
    },
    {
        'id': 'v3_02_barisov',
        'character': 'Dr Victor Barisov',
        'donor': os.path.join(REFS_DIR, '5fbf4d3b1d830376_en.wav'),
        'text': 'Widzisz Renko, linia czasowa została zmieniona, ale można ją skorygować, a Osobliwość jest kluczem. Musimy powstrzymać Demiczewa.',
        'has_radio': False
    },
    {
        'id': 'v3_03_demichev',
        'character': 'Gen. Nikolai Demichev',
        'donor': os.path.join(REFS_DIR, '425e388a18d457a5_en.wav'),
        'text': 'Kapitanie Renko, nie bądź taki zaskoczony. Twój towarzysz był niezwykle chętny do współpracy. A teraz oddaj mi TMD.',
        'has_radio': False
    },
    {
        'id': 'v3_04_kathryn',
        'character': 'Kathryn (Mir-12)',
        'donor': os.path.join(REFS_DIR, '72400162a8a1f767_en.wav'), # Approved Wariant B
        'text': 'Doktorze Barisow, słyszy mnie pan? Proszę się trzymać, jesteśmy już w drodze do doków.',
        'has_radio': True
    },
    {
        'id': 'v3_05_nimitz',
        'character': 'Dowódca Nimitz',
        'donor': os.path.join(REFS_DIR, 'a118757c4ec96996_en.wav'),
        'text': 'Zespół Spartan, tu dowódca Nimitz. Wkraczacie w rosyjską przestrzeń powietrzną. Zachować szczególną ostrożność.',
        'has_radio': True
    }
]

def master_loud_and_safe(src_wav, dst_wav, is_radio=False, target_rms_db=-11.0, ceiling=0.88):
    # 1. Band filtering via ffmpeg
    temp_flt = dst_wav + '.flt.wav'
    if is_radio:
        filt = 'highpass=f=350,lowpass=f=3600'
    else:
        filt = 'highpass=f=80'
    cmd = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
           '-i', src_wav, '-filter:a', filt, '-ar', '24000', '-ac', '1', temp_flt]
    subprocess.run(cmd, check=True)
    
    # 2. Python soft-knee saturation limiter to target RMS and strict peak ceiling
    with wave.open(temp_flt, 'rb') as w:
        frames = w.readframes(w.getnframes())
        rate = w.getframerate()
        data = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0
    
    try:
        os.remove(temp_flt)
    except Exception:
        pass
        
    rms = np.sqrt(np.mean(data**2))
    target_rms = 10 ** (target_rms_db / 20.0)
    gain = target_rms / max(rms, 1e-6)
    
    amplified = data * gain
    # Soft saturation with tanh ensures smooth compression on transients without square clipping
    scaled = ceiling * np.tanh(amplified / ceiling)
    
    out_int16 = (scaled * 32767).astype(np.int16)
    with wave.open(dst_wav, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(out_int16.tobytes())

def check_lufs(path):
    import re
    cmd = ['ffmpeg', '-nostats', '-i', path, '-filter_complex', 'ebur128=peak=true', '-f', 'null', '-']
    p = subprocess.run(cmd, stderr=subprocess.PIPE, text=True, encoding='utf-8')
    m_i = re.search(r'Integrated loudness:\s+I:\s+([-\d.]+)\s+LUFS', p.stderr)
    m_tp = re.search(r'True peak:\s+Peak:\s+([-\d.]+)\s+dBFS', p.stderr)
    return (m_i.group(1) if m_i else 'N/A', m_tp.group(1) if m_tp else 'N/A')

def main():
    print("Loading Coqui XTTS-v2...", flush=True)
    tts = TTS('tts_models/multilingual/multi-dataset/xtts_v2', gpu=False)
    print("Model ready. Generating authentic game voice clones...", flush=True)
    
    for v in VOICES:
        print(f"\n=== Synthesizing {v['character']} ===", flush=True)
        print(f"Donor: {os.path.basename(v['donor'])}", flush=True)
        raw_wav = os.path.join(TEMP_DIR, f"{v['id']}_raw.wav")
        
        t0 = time.time()
        tts.tts_to_file(
            text=v['text'],
            speaker_wav=v['donor'],
            language='pl',
            file_path=raw_wav,
            temperature=0.25,
            split_sentences=False
        )
        print(f"Generated raw speech in {time.time()-t0:.1f}s.", flush=True)
        
        # 1. Mastered Studio Version
        out_studio = os.path.join(OUT_DIR, f"{v['id']}_studio.wav")
        master_loud_and_safe(raw_wav, out_studio, is_radio=False)
        i_s, tp_s = check_lufs(out_studio)
        print(f"  Studio -> {os.path.basename(out_studio)} | I: {i_s} LUFS | TP: {tp_s} dBFS", flush=True)
        
        # 2. Mastered Radio Version if applicable
        if v['has_radio']:
            out_radio = os.path.join(OUT_DIR, f"{v['id']}_radio.wav")
            master_loud_and_safe(raw_wav, out_radio, is_radio=True)
            i_r, tp_r = check_lufs(out_radio)
            print(f"  Radio  -> {os.path.basename(out_radio)} | I: {i_r} LUFS | TP: {tp_r} dBFS", flush=True)

    print("\nAll V3 game voice samples saved to:", OUT_DIR, flush=True)

if __name__ == '__main__':
    main()
