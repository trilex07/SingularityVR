import miniaudio, numpy as np, wave, os, array

RATE = 24000
OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014/casting_final/role_radio_demo'
os.makedirs(OUT, exist_ok=True)
BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014'

def load_any(p, max_s=25):
    if p.lower().endswith('.mp3') or p.lower().endswith('.m4a'):
        dec = miniaudio.decode_file(p, output_format=miniaudio.SampleFormat.SIGNED16)
        x = np.array(array.array('h', dec.samples), dtype=np.float64) / 32768.0
        if dec.nchannels == 2:
            x = x.reshape(-1, 2).mean(axis=1)
        sr = dec.sample_rate
    else:
        with wave.open(p, 'rb') as w:
            sr = w.getframerate(); ch = w.getnchannels()
            n = w.getnframes()
            x = np.array(array.array('h', w.readframes(n)), dtype=np.float64) / 32768.0
            if ch == 2:
                x = x.reshape(-1, 2).mean(axis=1)
    if sr != RATE:
        idx = np.linspace(0, len(x) - 1, int(len(x) * RATE / sr))
        x = np.interp(idx, np.arange(len(x)), x)
    # trim leading silence, take max_s seconds
    a = np.where(np.abs(x) > 0.02)[0]
    if len(a):
        x = x[a[0]:]
    return x[:RATE * max_s]

def radio_fx(x):
    # bandpass 300-3400 przez FFT
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / RATE)
    g = 1 / (1 + (300 / np.maximum(f, 1)) ** 4) * 1 / (1 + (f / 3400) ** 4)
    y = np.fft.irfft(X * g, len(x))
    # kompresor 4:1 prog -18dB
    env = np.abs(y)
    at = np.exp(-1 / (RATE * 0.005)); rel = np.exp(-1 / (RATE * 0.08))
    e = np.zeros_like(env); cur = 0.0
    for i, v in enumerate(env):
        cur = at * cur + (1 - at) * v if v > cur else rel * cur + (1 - rel) * v
        e[i] = cur
    thr = 10 ** (-18 / 20)
    gain = np.where(e > thr, (thr + (e - thr) / 4) / np.maximum(e, 1e-6), 1.0)
    y = y * gain
    # 15ms fade
    n = int(RATE * 0.015)
    ramp = np.linspace(0, 1, n)
    y[:n] *= ramp; y[-n:] *= ramp[::-1]
    # normalizacja -3dBFS
    y = y * (10 ** (-3 / 20) / max(1e-6, np.max(np.abs(y))))
    return np.clip(y, -1, 1)

def save(name, x):
    with wave.open(os.path.join(OUT, name), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes((x * 32767).astype(np.int16).tobytes())
    print('saved', name, '%.1fs' % (len(x) / RATE))

targets = [
    ('RADIO_kathryn_anna_bugajska_20s.wav', BASE + '/internet_candidates/commons_anna_bugajska.mp3', 20),
    ('RADIO_patrol_mc_speech.wav', BASE + '/internet_candidates/mc_speech_sample1.wav', 25),
    ('RADIO_kathryn_piper_gosia.wav', BASE + '/references/piper_gosia_reference.wav', 25),
    ('RADIO_kathryn_magda_rhvoice.wav', BASE + '/references/rhvoice_magda_reference.wav', 25),
]
for name, path, maxs in targets:
    try:
        x = load_any(path, maxs)
        save(name, radio_fx(x))
    except Exception as ex:
        print('SKIP', name, type(ex).__name__, ex)
print('DONE ->', OUT)
