"""Task B step 1 — MIARA przed galka (Side, L1, read-only sources).

Copies manifest-listed PL WAVs from read-only source dirs into a temp work dir
(never writes to sources), then measures on COPIES only:
  - WAV header census (sample_rate/channels/bits/format) via struct parsing
  - per-file peak + mean volume via ffmpeg volumedetect (full census)
  - integrated LUFS + true peak via ffmpeg ebur128 (seeded stratified sample
    per pack + all QA-flagged story files)

Outputs (into --out-dir inside own worktree):
  headers.jsonl, levels_volumedetect.jsonl, levels_ebur128.jsonl, SUMMARY.md

No gain changes, no WAV regeneration, no listening claims.
"""
import argparse, csv, hashlib, json, os, random, re, shutil, struct, subprocess, sys

PACKS = ("story", "barks", "mp", "skipped", "intro", "vignette", "nllb_control")

def parse_wav_header(path):
    with open(path, 'rb') as fh:
        raw = fh.read(4096)
    if raw[0:4] != b'RIFF' or raw[8:12] != b'WAVE':
        return {"ok": False, "error": "not RIFF/WAVE"}
    pos, fmt = 12, None
    while pos + 8 <= len(raw):
        cid = raw[pos:pos + 4]
        sz = struct.unpack('<I', raw[pos + 4:pos + 8])[0]
        body = raw[pos + 8:pos + 8 + min(sz, 64)]
        if cid == b'fmt ' and len(body) >= 16:
            tag, ch, sr, _, _, bits = struct.unpack('<HHIIHH', body[:16])
            fmt = {"format_tag": tag, "channels": ch, "sample_rate": sr,
                   "bits": bits,
                   "format": {1: "PCM16", 3: "FLOAT32"}.get(tag, "tag%d" % tag)}
        pos += 8 + sz + (sz & 1)
    if fmt is None:
        return {"ok": False, "error": "no fmt chunk in first 4k"}
    fmt["ok"] = True
    return fmt

def run(cmd):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return p.returncode, p.stdout.decode('utf-8', 'replace'), p.stderr.decode('utf-8', 'replace')

def measure_volumedetect(ffmpeg, path):
    rc, out, err = run([ffmpeg, '-hide_banner', '-nostats', '-i', path,
                        '-af', 'volumedetect', '-vn', '-sn', '-dn',
                        '-f', 'null', 'NUL' if os.name == 'nt' else '/dev/null'])
    txt = err
    m = {}
    for key in ('max_volume', 'mean_volume'):
        mm = re.search(key + r':\s*(-?\d+(?:\.\d+)?|-\s*inf(?:inity)?)\s*dB', txt)
        m[key] = None if mm is None else mm.group(1).replace(' ', '')
    mm = re.search(r'n_samples:\s*(\d+)', txt)
    m['n_samples'] = int(mm.group(1)) if mm else None
    m['rc'] = rc
    return m

def measure_ebur128(ffmpeg, path):
    rc, out, err = run([ffmpeg, '-hide_banner', '-nostats', '-i', path,
                        '-filter_complex', 'ebur128=peak=true:framelog=quiet',
                        '-f', 'null', 'NUL' if os.name == 'nt' else '/dev/null'])
    txt = err
    m = {}
    mm = re.search(r'I:\s*(-?\d+(?:\.\d+)?)\s*LUFS', txt)
    m['integrated_lufs'] = float(mm.group(1)) if mm else None
    mm = re.search(r'Peak:\s*(-?\d+(?:\.\d+)?)\s*dBFS', txt)
    m['true_peak_dbtp'] = float(mm.group(1)) if mm else None
    m['rc'] = rc
    return m

def load_json_any(path):
    raw = open(path, 'rb').read()
    for enc in ('utf-8-sig', 'utf-8', 'cp1250'):
        try:
            return json.loads(raw.decode(enc)), enc
        except Exception:
            pass
    raise ValueError('cannot decode %s' % path)

def build_file_list(repo, src_story, src_out, src_nllb):
    files = []  # (pack, src_path, key)
    man = os.path.join(repo, 'audio', 'polish', 'story_voicepack_xlingual',
                       'manifest_xlingual.jsonl')
    n = 0
    for line in open(man, encoding='utf-8'):
        line = line.strip()
        if not line:
            continue
        r = json.loads(line)
        files.append(('story', os.path.join(src_story, r['file']), r['file']))
        n += 1
    # nllb control: same basenames, cap 60
    nllb_names = [f for _, _, f in files if os.path.exists(os.path.join(src_nllb, f))]
    for f in sorted(nllb_names)[:60]:
        files.append(('nllb_control', os.path.join(src_nllb, f), f))
    out = os.path.join(repo, 'remaining-voices', 'out')
    barks, _ = load_json_any(os.path.join(out, 'barks', 'bark_rows.json'))
    for r in barks:
        files.append(('barks', os.path.join(src_out, 'barks', r['asset'] + '.pl.wav'), r['asset']))
    mp, _ = load_json_any(os.path.join(out, 'mp', 'mp_rows.json'))
    for r in mp:
        files.append(('mp', os.path.join(src_out, 'mp',
                                         r['package'] + '__' + r['asset'] + '.pl.wav'),
                      r['package'] + '/' + r['asset']))
    sk, _ = load_json_any(os.path.join(out, 'skipped', 'skipped_final.json'))
    for r in sk:
        files.append(('skipped', os.path.join(src_out, 'skipped', r['asset'] + '.pl.wav'), r['asset']))
    vig, _ = load_json_any(os.path.join(out, 'vignette', 'manifest_rows.json'))
    for r in vig:
        files.append(('vignette', os.path.join(src_out, 'vignette', r['source_wav']), r['asset']))
    # intro: no manifest in repo; take INTRO_*.pl.wav except _ref* and *alt* (alt measured separately if present)
    intro_dir = os.path.join(src_out, 'intro')
    if os.path.isdir(intro_dir):
        for f in sorted(os.listdir(intro_dir)):
            if f.startswith('INTRO_') and f.endswith('.pl.wav') and not f.startswith('_ref'):
                files.append(('intro', os.path.join(intro_dir, f), f))
    return files

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', required=True)
    ap.add_argument('--src-story', required=True)
    ap.add_argument('--src-out', required=True)
    ap.add_argument('--src-nllb', required=True)
    ap.add_argument('--work-dir', required=True, help='temp dir for COPIES')
    ap.add_argument('--out-dir', required=True, help='results dir in own worktree')
    ap.add_argument('--ffmpeg', default='ffmpeg')
    ap.add_argument('--ebur128-n', type=int, default=60)
    ap.add_argument('--seed', type=int, default=7)
    ap.add_argument('--skip-copy', action='store_true')
    a = ap.parse_args()

    os.makedirs(a.work_dir, exist_ok=True)
    os.makedirs(a.out_dir, exist_ok=True)
    files = build_file_list(a.repo, a.src_story, a.src_out, a.src_nllb)
    print('listed: %d' % len(files), flush=True)

    rng = random.Random(a.seed)
    copies = []
    missing = []
    for pack, src, key in files:
        dst = os.path.join(a.work_dir, pack + '__' + os.path.basename(src))
        if not a.skip_copy:
            if os.path.exists(src):
                shutil.copyfile(src, dst)
            else:
                missing.append((pack, src))
                continue
        elif not os.path.exists(dst):
            missing.append((pack, dst))
            continue
        copies.append((pack, dst, key))
    print('copied: %d missing: %d' % (len(copies), len(missing)), flush=True)
    with open(os.path.join(a.out_dir, 'missing_sources.json'), 'w', encoding='utf-8') as fh:
        json.dump([{"pack": p, "src": s} for p, s in missing], fh, ensure_ascii=False, indent=1)

    with open(os.path.join(a.out_dir, 'headers.jsonl'), 'w', encoding='utf-8') as fh:
        for pack, dst, key in copies:
            h = parse_wav_header(dst)
            h.update({"pack": pack, "key": key, "copy": os.path.basename(dst),
                      "bytes": os.path.getsize(dst)})
            fh.write(json.dumps(h, ensure_ascii=False) + '\n')
    print('headers done', flush=True)

    with open(os.path.join(a.out_dir, 'levels_volumedetect.jsonl'), 'w', encoding='utf-8') as fh:
        for i, (pack, dst, key) in enumerate(copies):
            m = measure_volumedetect(a.ffmpeg, dst)
            m.update({"pack": pack, "key": key, "copy": os.path.basename(dst)})
            fh.write(json.dumps(m, ensure_ascii=False) + '\n')
            if (i + 1) % 200 == 0:
                print('volumedetect %d/%d' % (i + 1, len(copies)), flush=True)
    print('volumedetect done', flush=True)

    # ebur128 sample: stratified per pack + all QA-flagged story files
    qa = set()
    for q in ('QA_FLAGS.json', 'ASR_QA.json'):
        p = os.path.join(a.repo, 'audio', 'polish', 'story_voicepack_xlingual', q)
        try:
            d, _ = load_json_any(p)
        except Exception:
            continue
        if isinstance(d, dict):
            for v in d.values():
                for row in v:
                    qa.add(row[0])
        elif isinstance(d, list):
            for row in d:
                qa.add(row.get('file'))
    by_pack = {}
    for pack, dst, key in copies:
        by_pack.setdefault(pack, []).append((dst, key))
    sample = []
    for pack, items in sorted(by_pack.items()):
        if pack == 'nllb_control':
            sample += [(pack, d, k) for d, k in items]  # all (<=60)
            continue
        forced = [(pack, d, k) for d, k in items if k in qa or d in qa]
        rest = [(pack, d, k) for d, k in items if k not in qa and d not in qa]
        rng.shuffle(rest)
        sample += forced + rest[:a.ebur128_n]
    # also force basename match for qa (story keys are basenames)
    print('ebur128 sample: %d (qa-forced story: %d)' %
          (len(sample), sum(1 for p, d, k in sample if p == 'story' and (k in qa))), flush=True)
    with open(os.path.join(a.out_dir, 'levels_ebur128.jsonl'), 'w', encoding='utf-8') as fh:
        for i, (pack, dst, key) in enumerate(sample):
            m = measure_ebur128(a.ffmpeg, dst)
            m.update({"pack": pack, "key": key, "copy": os.path.basename(dst)})
            fh.write(json.dumps(m, ensure_ascii=False) + '\n')
            if (i + 1) % 50 == 0:
                print('ebur128 %d/%d' % (i + 1, len(sample)), flush=True)
    print('ebur128 done', flush=True)

if __name__ == '__main__':
    main()
