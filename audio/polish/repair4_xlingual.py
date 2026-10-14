"""RUNDA 4: 3 pliki - respelling nazw wlasnych + pauzy + dawca testowy dla 6ba2."""
import sys, os, json
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish'
OUTDIR = AUDIO + '/story_voicepack_xlingual'
REFDIR = os.path.join(OUTDIR, '_refs_en')
BAK4 = os.path.join(OUTDIR, '_backup_r4')
os.makedirs(BAK4, exist_ok=True)

JOBS = {
    # 6ba2: inny dawca (645b) + respelling; dziwne koncowki zdan
    '6ba243d8f3f9a7f9.wav': {
        'donor': '645b6de719763bc4_en.wav',
        'gap_ms': 200,
        'chunks': [
            'Podobno doktor Barisow może zostać nowym szefem Katorgi dwanaście, ale podobno Moskwa nie ma do niego wielkiego zaufania.',
            'Gdyby tylko doktor Demiczew nie zginął w tym pożarze.'],
    },
    # db5a: bełkot -> krotsze chunki + respelling, ten sam dawca
    'db5a9bb47d436a5a.wav': {
        'donor': '5339b0068ab0840e_en.wav',
        'gap_ms': 200,
        'chunks': [
            'Ile jeszcze wytrzymam?',
            'Singularność wybuchła dwa dni temu i od tamtej pory nie odpoczywałam.',
            'Kilku medyków zdołało otworzyć punkt triażu w tych tunelach, ale ilu byśmy nie opatrzyli, przybywa jeszcze więcej chorych i rannych.',
            'Ta wyspa mnie wykończy.'],
    },
    # 825b: brak pauz -> zdanie = chunk, szerokie przerwy 300ms
    '825b608f491f738b.wav': {
        'donor': '5339b0068ab0840e_en.wav',
        'gap_ms': 300,
        'chunks': [
            'Pomocy!',
            'Syreny!',
            'A potem nastąpił wybuch, pani Barisow krzyknęła i wybiegła z klasy.',
            'Zostawiła nas tam.'],
    },
}

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.5, quantize=True)
SR = 24000
for fname, job in JOBS.items():
    outp = os.path.join(OUTDIR, fname)
    if os.path.exists(outp):
        os.replace(outp, os.path.join(BAK4, fname))
    ref = os.path.join(REFDIR, job['donor'])
    try:
        parts = []
        for ch in job['chunks']:
            state = model.get_state_for_audio_prompt(ref)
            a = model.generate_audio(state, ch).numpy().squeeze()
            parts.append(np.atleast_1d(a).astype(np.float32))
        gap = np.zeros(int(SR * job['gap_ms'] / 1000), dtype=np.float32)
        full = parts[0]
        for p in parts[1:]:
            full = np.concatenate([full, gap, p])
        # normalizacja -1dBFS od razu (reszta packu juz znormalizowana)
        full = full * (10 ** (-1 / 20) / max(1e-6, np.max(np.abs(full))))
        import array
        with open(outp, 'wb') as fh:
            body = array.array('h', (np.clip(full, -1, 1) * 32767).astype(np.int16)).tobytes()
            fh.write(b'RIFF' + __import__('struct').pack('<I', 36 + len(body)) + b'WAVEfmt ')
            fh.write(__import__('struct').pack('<IHHIIHH', 16, 1, 1, SR, SR * 2, 2, 16))
            fh.write(b'data' + __import__('struct').pack('<I', len(body)))
            fh.write(body)
        print('R4', fname, round(len(full) / SR, 1), 's', flush=True)
    except Exception as ex:
        print('FAIL', fname, type(ex).__name__, str(ex)[:120], flush=True)
print('DONE')
