import sys, os, json, hashlib, struct
sys.path.insert(0, 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/tools/audio')
from ue3_sound_package import inspect

VDIR = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/vignette'
data = open('C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/_work_u/SP_RT3_Vignette_LOC_INT.u', 'rb').read()
by = {}
for e in inspect(data)['exports']:
    if e['class_name'] == 'SoundNodeWave':
        by[e['name']] = {x['name']: x.get('value') for x in e['properties']}

PL = {
    'RT99_0105_SC1': 'Bo to nie ma sensu, Michale! Narażasz tych ludzi bez powodu!',
    'RT99_0106_SC2': 'A ty tylko czekasz na śmierć.',
    'RT99_0107_SC1': 'Ale tam jest zbyt niebezpiecznie!',
    'RT99_0108_SC2': 'Posłuchaj mnie. Kanały doprowadzą nas do parowozowni. Stamtąd weźmiemy lokomotywę do doków i uciekniemy.',
    'RT99_0111_SC1': 'Nie. To samobójstwo.',
    'RT99_0112_SC2': 'Bardzo dobrze. Życzę ci wszystkiego najlepszego, przyjacielu. Mam nadzieję, że zobaczymy się na stałym lądzie, w otoczeniu rodziny, towarzyszu.',
}
QA = {  # z ASR screeningu
    'RT99_0105_SC1': 'REVIEW: srodek kwestii do potwierdzenia uchem',
    'RT99_0106_SC2': 'ASR-screen PASS (krotka)',
    'RT99_0107_SC1': 'ASR-screen PASS (krotka)',
    'RT99_0108_SC2': 'REVIEW: srodek do potwierdzenia uchem',
    'RT99_0111_SC1': 'ASR-screen PASS (krotka)',
    'RT99_0112_SC2': 'REVIEW: dluga kwestia, czesciowe pokrycie ASR',
}

def wav_dur_peak(p):
    with open(p, 'rb') as fh:
        raw = fh.read()
    pos = 12; sr = 0; data = None; bits = 16
    while pos + 8 <= len(raw):
        cid = raw[pos:pos+4]; sz = struct.unpack('<I', raw[pos+4:pos+8])[0]
        body = raw[pos+8:pos+8+sz]
        if cid == b'fmt ':
            sr = struct.unpack('<HHI', body[:8])[2]; bits = struct.unpack('<H', body[14:16])[0]
        elif cid == b'data':
            data = body
        pos += 8 + sz + (sz & 1)
    import numpy as np
    if bits == 16:
        n = len(data) // 2
        s = np.array(struct.unpack('<%dh' % n, data[:n*2]), dtype=np.float32) / 32768.0
    else:
        n = len(data) // 4
        s = np.array(struct.unpack('<%df' % n, data[:n*4]), dtype=np.float32)
    peak = float(np.max(np.abs(s)))
    import math
    return len(s) / sr, 20 * math.log10(max(peak, 1e-9))

rows = []
for asset, pl in PL.items():
    f = os.path.join(VDIR, asset + '.pl.wav')
    h = hashlib.sha256(open(f, 'rb').read()).hexdigest()
    dur, peakdb = wav_dur_peak(f)
    p = by[asset]
    frames = p['RawPCMDataSize'] // 2
    budget = frames / p['SampleRate']
    rows.append({
        'asset': asset, 'source_wav': asset + '.pl.wav', 'source_sha256': h,
        'english': p['SpokenText'], 'polish': pl,
        'text_exact_match': False, 'qa_recall': None, 'qa_flagged': True,
        'qa_note': QA[asset],
        'true_peak_dbtp': round(peakdb, 2), 'tempo': round(dur / budget, 3),
        'guard_ms': 200, 'source_duration': round(dur, 3),
        'native_frames': frames, 'rate': p['SampleRate'],
        'native_ch': p['NumChannels'],
        'stereo_note': 'RT99_0105_SC1 native STEREO 44.1k: kandydat mono-downmix, decyzja MAIN' if asset == 'RT99_0105_SC1' else '',
    })
    print(asset, 'tempo=%.2f' % (dur / budget), 'peak=%.1f' % peakdb, QA[asset][:40])

json.dump(rows, open(VDIR + '/manifest_rows.json', 'w'), ensure_ascii=False, indent=1)
print('zapisano manifest_rows.json')
