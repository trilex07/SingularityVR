import miniaudio, numpy as np, wave, os

SRC = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/full_voicepack/0069_da72bdf8ca31.wav'
OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014/casting_final/role_lektor/A_rework'
os.makedirs(OUT, exist_ok=True)
RATE = 24000

# decode mp3 -> float mono @24k
dec = miniaudio.decode_file(SRC, output_format=miniaudio.SampleFormat.SIGNED16)
import array
pcm = np.array(array.array('h', dec.samples), dtype=np.float64) / 32768.0
if dec.nchannels == 2:
    pcm = pcm.reshape(-1, 2).mean(axis=1)
if dec.sample_rate != RATE:
    # linear resample
    old_idx = np.linspace(0, len(pcm) - 1, int(len(pcm) * RATE / dec.sample_rate))
    pcm = np.interp(old_idx, np.arange(len(pcm)), pcm)
print('src: %.2fs @%dHz ch=%d' % (len(pcm) / RATE, dec.sample_rate, dec.nchannels))

def trim(x, thr=0.02):
    a = np.where(np.abs(x) > thr)[0]
    if len(a) == 0:
        return x
    return x[max(0, a[0] - int(RATE * 0.03)):a[-1] + int(RATE * 0.15)]

def fade(x, ms=15):
    n = int(RATE * ms / 1000)
    if len(x) > 2 * n:
        ramp = np.linspace(0, 1, n)
        x = x.copy()
        x[:n] *= ramp
        x[-n:] *= ramp[::-1]
    return x

def norm(x, peak_db=-3.0):
    p = np.max(np.abs(x))
    if p > 0:
        x = x * (10 ** (peak_db / 20) / p)
    return x

def fft_eq(x, curve_fn):
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / RATE)
    X = X * curve_fn(f)
    return np.fft.irfft(X, len(x))

def db(x):
    return 10 ** (x / 20)

def save(name, x):
    x = norm(fade(trim(x)))
    x = np.clip(x, -1, 1)
    with wave.open(os.path.join(OUT, name), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes((x * 32767).astype(np.int16).tobytes())
    print('saved', name, '%.2fs' % (len(x) / RATE))

base = trim(pcm)

# V1 CLEAN
save('A_V1_CLEAN.wav', base)

# V2 CIEPLY: +4dB <300Hz, -2dB >8kHz
def warm(f):
    g = np.ones_like(f)
    g += (db(4) - 1) * np.exp(-(f / 300) ** 2)
    g = np.where(f > 8000, db(-2), g)
    return g
save('A_V2_CIEPLY.wav', fft_eq(base, warm))

# V3 PREZENCJA: +3dB bell 2-5kHz
def pres(f):
    g = np.ones_like(f)
    g += (db(3) - 1) * np.exp(-((f - 3500) / 1500) ** 2)
    return g
save('A_V3_PREZENCJA.wav', fft_eq(base, pres))

# V4 RADIO: bandpass 300-3400 + kompresja
def bp(f):
    g = 1 / (1 + (300 / np.maximum(f, 1)) ** 4) * 1 / (1 + (f / 3400) ** 4)
    return g
r = fft_eq(base, bp)
# prosty kompresor: prog -18dB, ratio 4:1
env = np.abs(r)
a = np.exp(-1 / (RATE * 0.005)); rel = np.exp(-1 / (RATE * 0.08))
e = np.zeros_like(env); cur = 0.0
for i, v in enumerate(env):
    cur = a * cur + (1 - a) * v if v > cur else rel * cur + (1 - rel) * v
    e[i] = cur
thr = 10 ** (-18 / 20)
gain = np.where(e > thr, (thr + (e - thr) / 4) / np.maximum(e, 1e-6), 1.0)
save('A_V4_RADIO.wav', r * gain)

# V5 POGLOS: maly room (2 grzebienie + allpass), wet 12%
def comb(x, delay_ms, fb):
    d = int(RATE * delay_ms / 1000)
    y = x.copy()
    for i in range(d, len(y)):
        y[i] += fb * y[i - d]
    return y
wet = comb(base, 29, 0.5) + comb(base, 37, 0.45)
wet = wet / max(1e-6, np.max(np.abs(wet))) * np.max(np.abs(base)) * 0.12
save('A_V5_POGLOS.wav', base + wet)

print('DONE ->', OUT)
