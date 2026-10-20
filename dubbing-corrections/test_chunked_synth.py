import sys, os, time, wave, numpy as np, miniaudio, re, subprocess
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
from pocket_tts import TTSModel

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.whisper_tools')
from faster_whisper import WhisperModel

EN_BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
src_ogg = os.path.join(EN_BASE, 'SP_VI2_zone02a_2010_LOC_INT', 'SoundNodeWave', 'VI99_1210_SC1.ogg')

dec = miniaudio.decode_file(src_ogg, output_format=miniaudio.SampleFormat.SIGNED16)
x = np.array(dec.samples, dtype=np.float32) / 32768.0
if dec.nchannels == 2: x = x.reshape(-1, 2).mean(axis=1)
if dec.sample_rate != 24000:
    idx = np.linspace(0, len(x) - 1, int(len(x) * 24000 / dec.sample_rate))
    x = np.interp(idx, np.arange(len(x)), x)

ref_wav = r'C:\Users\Trilex\AppData\Local\Temp\sc1_ref.wav'
with wave.open(ref_wav, 'wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
    w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())

text = 'Od czasu wyjazdu doktora Demiczewa do Moskwy napływ nowych naukowców i techników na Katordze dwanaście wzrósł dziesięciokrotnie. Prezentacja naszych prac musiała odnieść wielki sukces. Powstają nowe obiekty. Fundusze na badania stale rosną. Pomyśleć, że zaledwie cztery tygodnie temu doktor Demiczew niemal zginął w tamtym pożarze. Ktokolwiek go uratował, wyświadczył nam wszystkim ogromną przysługę.'

# Split into natural sentences
sentences = [s.strip() for s in re.split(r'(?<=[.!?])\s+', text) if s.strip()]
print('Sentences to synthesize:', len(sentences), flush=True)

model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.65, quantize=True)
state = model.get_state_for_audio_prompt(ref_wav)

audios = []
for i, s in enumerate(sentences, 1):
    t0 = time.time()
    audio = model.generate_audio(state, s).numpy().squeeze()
    audios.append(np.atleast_1d(audio).astype(np.float32))
    print(f'Chunk {i}: "{s[:35]}..." -> {len(audio)/24000:.2f}s ({time.time()-t0:.2f}s)', flush=True)

gap = np.zeros(int(24000 * 0.12), dtype=np.float32)
full = audios[0]
for a in audios[1:]:
    full = np.concatenate([full, gap, a])

out_wav = r'C:\Users\Trilex\AppData\Local\Temp\fixed_abed.wav'
with wave.open(out_wav, 'wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
    w.writeframes((np.clip(full, -1, 1) * 32767).astype(np.int16).tobytes())

asr = WhisperModel('small', device='cpu', compute_type='int8')
dec2 = miniaudio.decode_file(out_wav, output_format=miniaudio.SampleFormat.FLOAT32)
s2 = np.array(dec2.samples, dtype=np.float32)
idx2 = np.linspace(0, len(s2) - 1, int(len(s2) * 16000 / dec2.sample_rate))
x16 = np.interp(idx2, np.arange(len(s2)), s2)
segs, _ = asr.transcribe(x16, language='pl')
print('Fixed ASR Transcription:', ' '.join(seg.text for seg in segs), flush=True)
