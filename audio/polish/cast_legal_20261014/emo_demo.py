import numpy as np, wave, os, shutil

RATE = 24000
RDIR = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014/casting_final/role_radio_demo'
BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014'

# 1. A_V4 do zestawu radiowego (bylo pominecie)
shutil.copy2(BASE + '/casting_final/role_lektor/A_rework/A_V4_RADIO.wav',
             RDIR + '/RADIO_lektor_A_V4.wav')
print('copied A_V4')

def load_wav(p):
    with wave.open(p, 'rb') as w:
        assert w.getframerate() == RATE and w.getnchannels() == 1
        n = w.getnframes()
        import array
        return np.array(array.array('h', w.readframes(n)), dtype=np.float64) / 32768.0

def save_wav(p, x):
    x = np.clip(x, -1, 1)
    with wave.open(p, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes((x * 32767).astype(np.int16).tobytes())

def shorten_pauses(x, thr=0.03, max_keep_ms=120):
    # znajdz cisze >250ms, zostaw 120ms
    win = RATE // 100
    is_sil = np.array([np.sqrt(np.mean(x[i:i+win]**2)) < thr
                       for i in range(0, len(x), win)])
    keep = np.ones(len(x), dtype=bool)
    i = 0
    while i < len(is_sil):
        if is_sil[i]:
            j = i
            while j < len(is_sil) and is_sil[j]:
                j += 1
            if (j - i) * 10 > 250:  # dluzsze niz 250ms
                cut_from = i * win + int(max_keep_ms * RATE / 1000)
                cut_to = j * win
                if cut_to > cut_from:
                    keep[cut_from:cut_to] = False
            i = j
        else:
            i += 1
    return x[keep]

def resample(x, factor):
    idx = np.linspace(0, len(x) - 1, int(len(x) / factor))
    return np.interp(idx, np.arange(len(x)), x)

def presence(x):
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / RATE)
    g = 1 + (10 ** (3 / 20) - 1) * np.exp(-((f - 3500) / 1500) ** 2)
    return np.fft.irfft(X * g, len(x))

def emo_chain(x):
    x = shorten_pauses(x)          # mniej powietrza = pilnosc
    x = resample(x, 1.06)          # +6% tempo i wysokosc = napiecie
    x = presence(x)                # klarownosc krzyku
    x = np.tanh(1.6 * x) / np.tanh(1.6)  # gestosc, krzyk bez clippingu
    n = int(RATE * 0.015)
    ramp = np.linspace(0, 1, n)
    x[:n] *= ramp; x[-n:] *= ramp[::-1]
    x = x * (10 ** (-3 / 20) / max(1e-6, np.max(np.abs(x))))
    return x

for f in sorted(os.listdir(RDIR)):
    if f.startswith('RADIO_') and f.endswith('.wav'):
        x = load_wav(os.path.join(RDIR, f))
        save_wav(os.path.join(RDIR, f.replace('RADIO_', 'EMO_')), emo_chain(x))
        print('EMO <-', f)
print('DONE')
