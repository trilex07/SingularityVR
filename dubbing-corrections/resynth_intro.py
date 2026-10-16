"""Resynteza batch3 intro (6 kwestii). Dawca = wlasny oryginalny ogg EN kwestii
(story_en_reference/SP_VI_intro_Kismet_LOC_INT/SoundNodeWave/<asset>.ogg).
Temp 0.5 (precedens gen_intro), 1 chunk = 1 zdanie, gap 0.04 (standard po fixie),
PCM16 mono 24 kHz peak -1 dBFS. Wyjscie TYLKO temp. Manifest do --manifest-out."""
import argparse, hashlib, json, os, re, sys, time
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections')
from resynth_corrected import clean, ogg_to_wav_mono24

ENB = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference\SP_VI_intro_Kismet_LOC_INT\SoundNodeWave'

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', required=True)
    ap.add_argument('--corrections', default='dubbing-corrections/corrections_intro.jsonl')
    ap.add_argument('--out-dir', required=True)
    ap.add_argument('--manifest-out', required=True)
    ap.add_argument('--temp', type=float, default=0.5)
    a = ap.parse_args()
    os.makedirs(a.out_dir, exist_ok=True)
    donors = os.path.join(a.out_dir, '_donors_en')
    os.makedirs(donors, exist_ok=True)

    pl = json.load(open(os.path.join(a.repo, 'remaining-voices', 'intro-pl.json'),
                         encoding='utf-8'))
    idx = {r['asset']: r for r in pl}
    cor = [json.loads(l) for l in
           open(os.path.join(a.repo, a.corrections.replace('/', os.sep)), encoding='utf-8')
           if l.strip()]
    jobs = []
    for c in cor:
        asset = c['id'].split(':')[-1]
        ogg = os.path.join(ENB, asset + '.ogg')
        assert os.path.exists(ogg), ogg
        dw = os.path.join(donors, asset + '_en.wav')
        if not os.path.exists(dw):
            ogg_to_wav_mono24(ogg, dw)
        jobs.append((asset, clean(c['proposed_pl']), dw))
    print('jobs: %d' % len(jobs), flush=True)

    from pocket_tts import TTSModel
    from scipy.io import wavfile
    model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                                temp=a.temp, quantize=True)
    SR = model.sample_rate
    assert SR == 24000
    man = []
    for n, (asset, text, donor) in enumerate(jobs, 1):
        t0 = time.time()
        try:
            sents = [p for p in re.split(r'(?<=[.!?…])\s+', text) if p]
            parts = []
            for ch in sents:
                st = model.get_state_for_audio_prompt(donor)
                au = model.generate_audio(st, ch).numpy().squeeze()
                parts.append(np.atleast_1d(au).astype(np.float32))
            full = parts[0]
            gap = np.zeros(int(SR * 0.04), dtype=np.float32)
            for p in parts[1:]:
                full = np.concatenate([full, gap, p])
            peak = float(np.max(np.abs(full))) or 1e-6
            g = (10 ** (-1.0 / 20)) / peak
            outp = os.path.join(a.out_dir, asset + '.pl.wav')
            wavfile.write(outp, SR, (np.clip(full * g, -1, 1) * 32767).astype(np.int16))
            h = hashlib.sha256(open(outp, 'rb').read()).hexdigest()[:16]
            man.append({'file': asset + '.pl.wav', 'asset': asset, 'donor': 'own-game-en-ogg',
                        'text_pl': text, 'chunks': len(sents), 'dur_s': round(len(full) / SR, 2),
                        'budget_s': idx[asset].get('dur'),
                        'peak_gain_db': round(20 * np.log10(g), 2),
                        'sha16': h, 'gen_s': round(time.time() - t0, 1)})
            print('SYN %d/%d %s %dch %.1fs' % (n, len(jobs), asset, len(sents),
                                              len(full) / SR), flush=True)
        except Exception as ex:
            man.append({'file': asset + '.pl.wav', 'asset': asset,
                        'FAIL': '%s: %s' % (type(ex).__name__, str(ex)[:160])})
            print('FAIL', asset, type(ex).__name__, flush=True)
    with open(a.manifest_out, 'w', encoding='utf-8') as fh:
        for m in man:
            fh.write(json.dumps(m, ensure_ascii=False) + '\n')
    print('DONE ok=%d/%d' % (sum(1 for m in man if 'FAIL' not in m), len(man)), flush=True)

if __name__ == '__main__':
    main()
