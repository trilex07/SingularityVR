"""Task B step 3 — parametryzowany gain na KOPIACH (Side, L1/L2, offline).

Roznice wzgledem audio/polish/louden_xlingual.py (ktorego NIE ruszamy):
  - zero hardkodow (wszystko z argparse),
  - nigdy nie pracuje w miejscu ani na zrodlach — tylko --copies-dir -> --out-dir,
  - sufit parametryzowany (--ceiling-db, domyslnie -1.0),
  - swiadomosc true-peak: jesli dla pliku zmierzono true peak (ebur128),
    gain = ceiling - max(sample_peak, true_peak), chyba ze --ignore-true-peak,
  - obsluga PCM16 i FLOAT32 (louden assertuje tylko float32),
  - zero clippingu: po zapisie odczyt weryfikacyjny, licznik probek 32767/-32768,
  - tabela per plik: {file, pack, gain_db, peak_przed/po, true_peak_przed [, clipped]}.

Bez regeneracji glosow, bez odsłuchu, bez tez o miksie z muzyka.
"""
import argparse, json, math, os, struct
from collections import defaultdict

def read_wav_samples(path):
    with open(path, 'rb') as fh:
        raw = fh.read()
    assert raw[0:4] == b'RIFF' and raw[8:12] == b'WAVE', path
    pos, tag, ch, sr, bits, data = 12, None, 0, 0, 0, None
    while pos + 8 <= len(raw):
        cid = raw[pos:pos + 4]
        sz = struct.unpack('<I', raw[pos + 4:pos + 8])[0]
        body = raw[pos + 8:pos + 8 + sz]
        if cid == b'fmt ':
            tag, ch, sr, _, _, bits = struct.unpack('<HHIIHH', body[:16])
        elif cid == b'data':
            data = body
        pos += 8 + sz + (sz & 1)
    assert data is not None, path
    if tag == 3 and bits == 32:
        n = len(data) // 4
        s = list(struct.unpack('<%df' % n, data[:n * 4]))
    elif tag == 1 and bits == 16:
        n = len(data) // 2
        s = [v / 32768.0 for v in struct.unpack('<%dh' % n, data[:n * 2])]
    else:
        raise ValueError('unsupported format %s/%s in %s' % (tag, bits, path))
    if ch == 2:
        s = [(a + b) / 2 for a, b in zip(s[0::2], s[1::2])]
    return sr, ch, tag, bits, s

def write_pcm16_mono(path, sr, s):
    import array
    body = array.array('h', [max(-32768, min(32767, int(round(x * 32767)))) for x in s]).tobytes()
    with open(path, 'wb') as fh:
        fh.write(b'RIFF' + struct.pack('<I', 36 + len(body)) + b'WAVEfmt ')
        fh.write(struct.pack('<IHHIIHH', 16, 1, 1, sr, sr * 2, 2, 16))
        fh.write(b'data' + struct.pack('<I', len(body)))
        fh.write(body)

def db(x):
    return 20 * math.log10(x) if x > 0 else float('-inf')

def load(dirpath, name):
    out = []
    with open(os.path.join(dirpath, name), encoding='utf-8') as fh:
        for line in fh:
            line = line.strip()
            if line:
                out.append(json.loads(line))
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--copies-dir', required=True)
    ap.add_argument('--measurements-dir', required=True)
    ap.add_argument('--out-dir', required=True)
    ap.add_argument('--packs', default='skipped,vignette,intro')
    ap.add_argument('--ceiling-db', type=float, default=-1.0)
    ap.add_argument('--ignore-true-peak', action='store_true')
    ap.add_argument('--dry-run', action='store_true')
    a = ap.parse_args()
    packs = set(a.packs.split(','))
    os.makedirs(a.out_dir, exist_ok=True)

    vol = {}
    for r in load(a.measurements_dir, 'levels_volumedetect.jsonl'):
        try:
            peak = float(r['max_volume'])
        except (TypeError, ValueError):
            continue
        vol.setdefault(r['copy'], []).append((r['pack'], r['key'], peak))
    tp = {}
    for r in load(a.measurements_dir, 'levels_ebur128.jsonl'):
        if r['true_peak_dbtp'] is not None:
            tp[r['copy']] = float(r['true_peak_dbtp'])
    hdr = {}
    for r in load(a.measurements_dir, 'headers.jsonl'):
        hdr[r['copy']] = r

    table = []
    for copy, entries in sorted(vol.items()):
        pack = entries[0][0]
        if pack not in packs:
            continue
        peak = max(p for _, _, p in entries)
        ref = peak
        tp_used = None
        if not a.ignore_true_peak and copy in tp:
            tp_used = tp[copy]
            ref = max(ref, tp_used)
        gain = a.ceiling_db - ref
        row = {
            'file': copy,
            'pack': pack,
            'manifest_keys': sorted(set(k for _, k, _ in entries)),
            'n_manifest_rows': len(entries),
            'gain_db': round(gain, 2),
            'peak_before_db': round(peak, 2),
            'peak_after_db_expected': round(peak + gain, 2),
            'true_peak_before_db': tp_used,
            'format_before': hdr.get(copy, {}).get('format'),
            'sample_rate': hdr.get(copy, {}).get('sample_rate'),
        }
        if not a.dry_run:
            src = os.path.join(a.copies_dir, copy)
            sr, ch, tag, bits, s = read_wav_samples(src)
            assert sr == 24000, (copy, sr)
            g = 10 ** (gain / 20)
            out = [x * g for x in s]
            dst = os.path.join(a.out_dir, copy)
            write_pcm16_mono(dst, 24000, out)
            # weryfikacja bez clippingu na zapisie
            _, _, _, _, back = read_wav_samples(dst)
            peak_after = db(max(abs(x) for x in back) or 0.0)
            clipped = sum(1 for x in back if abs(x) >= 32767 / 32768.0)
            row['peak_after_db_measured'] = round(peak_after, 2)
            row['clipped_samples'] = clipped
            assert clipped == 0, (copy, clipped)
            assert peak_after <= a.ceiling_db + 0.15, (copy, peak_after)
        table.append(row)

    with open(os.path.join(a.out_dir, 'gain_table.jsonl'), 'w', encoding='utf-8') as fh:
        for row in table:
            fh.write(json.dumps(row, ensure_ascii=False) + '\n')
    gains = [r['gain_db'] for r in table]
    print('files: %d packs: %s' % (len(table), sorted(set(r['pack'] for r in table))), flush=True)
    print('gain_db min/med/max: %.2f / %.2f / %.2f' % (
        min(gains), sorted(gains)[len(gains) // 2], max(gains)), flush=True)
    print('DRY_RUN' if a.dry_run else 'APPLIED (copies only)', flush=True)

if __name__ == '__main__':
    main()
