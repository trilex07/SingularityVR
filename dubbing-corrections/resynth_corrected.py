"""Resynteza 130 poprawionych kwestii SP_VI5_KISMET03 (Side, za zgoda uzytkownika).

Dawca per kwestia = WLASNY angielski oryginal tej kwestii: najpierw
_refs_en/<file>_en.wav, fallback na wlasny ogg EN z gry (story_en_reference) —
czyli cieplo/tempo/emocje bierze od aktora tej linii; mowione po polsku
(Pocket-TTS Polish 6L, temp produkcyjny 0.65). Mapowanie glos->kto:
  soldier (GST/SPT/ESP/GRD) -> profil soldier_natan,
  unclassified              -> profil narrator_user,
  demichev                  -> profil demichev_ruslan
(dawca i tak zawsze wlasna linia EN, profil tylko informacyjnie w manifeście).

Wzorzec: repair2_xlingual.py (fresh state na chunk, split >25 slow, gap 0.12 s,
clean '/'->spacja). Wyjscie TYLKO do --out-dir (temp, NIE cudzy worktree),
final PCM16 mono 24 kHz peak -1 dBFS (forma packa). Manifest do --manifest-out.
Bez dotykania istniejacych WAV-ow, bez merge.
"""
import argparse, hashlib, json, os, re, sys, time
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

EN_BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'

def clean(t):
    t = t.replace('/', ' ')
    return re.sub(r'\s+', ' ', t).strip()

def split25(t):
    if len(t.split()) <= 25:
        return [t]
    parts = [p for p in re.split(r'(?<=[.!?…])\s+', t) if p]
    if len(parts) < 2:
        parts = [p for p in re.split(r'(?<=[,;:])\s+', t) if p]
    out, cur = [], ''
    for p in parts:
        if cur and len((cur + ' ' + p).split()) <= 25:
            cur = cur + ' ' + p
        else:
            if cur:
                out.append(cur)
            cur = p
    if cur:
        out.append(cur)
    if len(out) < 2 and len(t.split()) > 25:
        w = t.split()
        out = [' '.join(w[:len(w) // 2]), ' '.join(w[len(w) // 2:])]
    return out

def ogg_to_wav_mono24(src, dst):
    import miniaudio, array as pyarray
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != 24000:
        idx = np.linspace(0, len(x) - 1, int(len(x) * 24000 / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    import wave
    with wave.open(dst, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())
    return dst

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', required=True)
    ap.add_argument('--refdir', required=True, help='katalog _refs_en (tylko odczyt)')
    ap.add_argument('--out-dir', required=True, help='katalog wynikowy w temp')
    ap.add_argument('--manifest-out', required=True)
    ap.add_argument('--temp', type=float, default=0.65)
    ap.add_argument('--limit', type=int, default=0, help='0 = wszystkie')
    a = ap.parse_args()
    os.makedirs(a.out_dir, exist_ok=True)

    rows = [json.loads(l) for l in
            open(os.path.join(a.repo, 'localization', 'pl', 'polish_story_voice.jsonl'),
                 encoding='utf-8') if l.strip()]
    idx = {('story:%s:%s' % (r['package'], r['asset'])): r for r in rows}
    cor = [json.loads(l) for l in
           open(os.path.join(a.repo, 'dubbing-corrections', 'corrections_sp_vi5_kismet03.jsonl'),
                encoding='utf-8') if l.strip()]
    jobs = []
    donor_tmp = os.path.join(a.out_dir, '_donors_en')
    os.makedirs(donor_tmp, exist_ok=True)
    for c in cor:
        r = idx[c['id']]
        if r['text_pl'] == r['tts_text']:
            continue  # no-op (np. VI4_4220): audio juz zgodne
        ref_en = os.path.join(a.refdir, r['file'].replace('.wav', '_en.wav'))
        if os.path.exists(ref_en):
            donor, donor_kind = ref_en, 'own-xlingual-ref'
        else:
            # fallback: wlasny oryginalny ogg EN z gry (te same glosy, te same emocje)
            src_ogg = os.path.join(EN_BASE, r['source'])
            if not os.path.exists(src_ogg):
                jobs.append((c['id'], r, None, None, 'MISSING-EN-OGG'))
                continue
            donor = os.path.join(donor_tmp, r['file'].replace('.wav', '_en.wav'))
            if not os.path.exists(donor):
                donor = ogg_to_wav_mono24(src_ogg, donor)
            donor, donor_kind = donor, 'own-game-en-ogg'
        jobs.append((c['id'], r, clean(c['proposed_pl']), donor, donor_kind))
    if a.limit:
        jobs = [j for j in jobs if j[4] != 'MISSING-EN-OGG'][:a.limit]
    hard_missing = [j[0] for j in jobs if j[4] == 'MISSING-EN-OGG']
    print('jobs: %d (donor=own-EN ok), missing-en: %d' % (
        sum(1 for j in jobs if j[4] != 'MISSING-EN-OGG'), len(hard_missing)), flush=True)

    from pocket_tts import TTSModel
    from scipy.io import wavfile
    print('loading model...', flush=True)
    model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                                temp=a.temp, quantize=True)
    SR = model.sample_rate
    print('model SR: %d' % SR, flush=True)
    assert SR == 24000, SR

    man, t_all = [], time.time()
    synth_jobs = [j for j in jobs if j[4] != 'MISSING-EN-OGG']
    for m in jobs:
        if m[4] == 'MISSING-EN-OGG':
            man.append({'file': m[1]['file'], 'id': m[0], 'FAIL': m[4]})
    for n, (cid, r, text, donor, kind) in enumerate(synth_jobs, 1):
        t0 = time.time()
        try:
            state = model.get_state_for_audio_prompt(donor)
            chunks = split25(text)
            parts = []
            for ch in chunks:
                st = model.get_state_for_audio_prompt(donor)  # fresh state na chunk
                au = model.generate_audio(st, ch).numpy().squeeze()
                parts.append(np.atleast_1d(au).astype(np.float32))
            full = parts[0]
            gap = np.zeros(int(SR * 0.12), dtype=np.float32)
            for p in parts[1:]:
                full = np.concatenate([full, gap, p])
            peak = float(np.max(np.abs(full))) or 1e-6
            g = (10 ** (-1.0 / 20)) / peak
            pcm = np.clip(full * g, -1.0, 1.0)
            outp = os.path.join(a.out_dir, r['file'])
            wavfile.write(outp, SR, (pcm * 32767).astype(np.int16))
            h = hashlib.sha256(open(outp, 'rb').read()).hexdigest()[:16]
            man.append({'file': r['file'], 'id': cid, 'package': r['package'],
                        'asset': r['asset'], 'profile': r.get('profile'),
                        'speaker': r.get('speaker_guess'), 'donor': os.path.basename(donor),
                        'donor_kind': kind, 'text_pl': text, 'chunks': len(chunks),
                        'dur_s': round(len(full) / SR, 2),
                        'source_dur_s': r.get('source_duration_s'),
                        'peak_gain_db': round(20 * np.log10(g), 2),
                        'sha16': h, 'gen_s': round(time.time() - t0, 1)})
            print('SYN %d/%d %s %dch %.1fs' % (n, len(synth_jobs), r['file'][:16], len(chunks),
                                              len(full) / SR), flush=True)
        except Exception as ex:
            man.append({'file': r['file'], 'id': cid, 'FAIL': '%s: %s' % (
                type(ex).__name__, str(ex)[:160])})
            print('FAIL', cid, type(ex).__name__, str(ex)[:120], flush=True)
    with open(a.manifest_out, 'w', encoding='utf-8') as fh:
        for m in man:
            fh.write(json.dumps(m, ensure_ascii=False) + '\n')
    ok = sum(1 for m in man if 'FAIL' not in m)
    print('DONE ok=%d/%d total=%.1f min' % (ok, len(man), (time.time() - t_all) / 60), flush=True)

if __name__ == '__main__':
    main()
