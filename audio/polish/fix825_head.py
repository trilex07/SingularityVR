import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np, wave, struct, array, shutil

AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish'
OUTDIR = AUDIO + '/story_voicepack_xlingual'
REFDIR = os.path.join(OUTDIR, '_refs_en')
BAK6 = os.path.join(OUTDIR, '_backup_r6')
os.makedirs(BAK6, exist_ok=True)
SR = 24000

FNAME = '825b608f491f738b.wav'
cur = os.path.join(OUTDIR, FNAME)
shutil.copy2(cur, os.path.join(BAK6, FNAME))

with wave.open(cur, 'rb') as w:
    n = w.getnframes()
    s = np.array(struct.unpack('<%dh' % n, w.readframes(n)), dtype=float) / 32768

win = SR // 100
env = np.array([np.sqrt(np.mean(s[i:i+win]**2)) for i in range(0, len(s), win)])
sil = env < 0.03
gaps = []
i = 0
while i < len(sil):
    if sil[i]:
        j = i
        while j < len(sil) and sil[j]:
            j += 1
        if (j - i) >= 20:
            gaps.append((i * win, j * win))
        i = j
    else:
        i += 1
print('przerwy>=200ms:', [(round(a/SR, 2), round(b/SR, 2)) for a, b in gaps], flush=True)
# ogon = od konca 2. przerwy (po 2 krotkich okrzykach)
tail_start = gaps[1][1] if len(gaps) >= 2 else gaps[0][1]
print('ogon od %.2fs' % (tail_start / SR), flush=True)
tail = s[tail_start:]

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.3, quantize=True)
state = model.get_state_for_audio_prompt(os.path.join(REFDIR, '645b6de719763bc4_en.wav'))
head = np.atleast_1d(model.generate_audio(state, 'Pomocy! Syreny!').numpy().squeeze()).astype(np.float32)
# trim ciszy z head
a = np.where(np.abs(head) > 0.02)[0]
head = head[max(0, a[0]-int(SR*0.03)):a[-1]+int(SR*0.1)] if len(a) else head
gap = np.zeros(int(SR * 0.30), dtype=np.float32)
full = np.concatenate([head, gap, tail.astype(np.float32)])
full = full * (10 ** (-1 / 20) / max(1e-6, np.max(np.abs(full))))
with open(cur, 'wb') as fh:
    body = array.array('h', (np.clip(full, -1, 1) * 32767).astype(np.int16)).tobytes()
    fh.write(b'RIFF' + struct.pack('<I', 36 + len(body)) + b'WAVEfmt ')
    fh.write(struct.pack('<IHHIIHH', 16, 1, 1, SR, SR * 2, 2, 16))
    fh.write(b'data' + struct.pack('<I', len(body)))
    fh.write(body)
print('SAVED %.1fs (ogon od %.2fs)' % (len(full)/SR, tail_start/SR), flush=True)
