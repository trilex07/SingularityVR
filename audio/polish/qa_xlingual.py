import json, os, wave, struct

AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish'
MANIFEST = AUDIO + '/story_voicepack_nllb/manifest.jsonl'
OUTDIR = AUDIO + '/story_voicepack_xlingual'

def wav_info(p):
    with wave.open(p, 'rb') as w:
        n = w.getnframes(); r = w.getframerate(); ch = w.getnchannels()
        data = w.readframes(n)
    fmt = 'int16' if w.getsampwidth() == 2 else 'f32'
    if w.getsampwidth() == 2:
        s = struct.unpack('<%dh' % (len(data) // 2), data)
        peak = max(abs(x) for x in s) / 32768.0
        tail = s[-int(r * 0.2):] if len(s) > int(r * 0.2) else s
        tail_e = (sum(x * x for x in tail) / len(tail)) ** 0.5 / 32768.0
    else:
        import array
        s = struct.unpack('<%df' % (len(data) // 4), data)
        peak = max(abs(x) for x in s)
        tail = s[-int(r * 0.2):] if len(s) > int(r * 0.2) else s
        tail_e = (sum(x * x for x in tail) / len(tail)) ** 0.5
    return n / r, peak, tail_e

entries = [json.loads(l) for l in open(MANIFEST, encoding='utf-8') if l.strip()]
print('manifest:', len(entries))

missing, bad = [], []
long_token, abrupt, short = [], [], []
for e in entries:
    f = os.path.join(OUTDIR, e['file'])
    if not os.path.exists(f):
        missing.append(e['file'])
        continue
    try:
        dur, peak, tail = wav_info(f)
    except Exception as ex:
        bad.append((e['file'], str(ex)))
        continue
    txt = e.get('text_pl_machine', '')
    nwords = len(txt.split())
    # heurystyka: ~2.2 slowa/s normalnie; flaga gdy audio podejrzanie krotkie
    if dur < nwords / 4.5 and nwords > 6:
        short.append((e['file'], txt[:70], round(dur, 1), nwords))
    # urwany koniec: ogon glosny = mozliwe uciencie bez EOS
    if tail > 0.08 and dur > 1.0:
        abrupt.append((e['file'], txt[:70], round(dur, 1), round(tail, 3)))
    if nwords > 40:
        long_token.append((e['file'], txt[:70], nwords, round(dur, 1)))

print('missing:', len(missing), missing[:5])
print('unreadable:', len(bad), bad[:5])
print('long-text(>40 slow, ryzyko skip):', len(long_token))
for x in long_token:
    print('  ', x)
print('abrupt-tail(podejrzenie uciencie):', len(abrupt))
for x in abrupt[:20]:
    print('  ', x)
print('too-short-vs-text:', len(short))
for x in short[:20]:
    print('  ', x)

with open(os.path.join(OUTDIR, 'QA_FLAGS.json'), 'w', encoding='utf-8') as f:
    json.dump({'missing': missing, 'unreadable': bad, 'long_token': long_token,
               'abrupt_tail': abrupt, 'too_short': short}, f, ensure_ascii=False, indent=1)
print('flags saved')
