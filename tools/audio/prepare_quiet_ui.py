"""Offline PCM attenuation of the existing revoice candidate; never generates speech.

Keeps all1886 identities/QA states, changes only894 UI wave payloads. Existing
WAV lease remains the installer; output is private and never goes into Git.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


def attenuate(data, percent=35):
    if not 0 <= percent <= 100:
        raise ValueError('gain must be 0..100 percent')
    if len(data) > 16*1024*1024 or len(data) < 44 or data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise ValueError('bounded RIFF WAVE required')
    if struct.unpack_from('<I', data, 4)[0]+8 != len(data):
        raise ValueError('RIFF size mismatch')
    pos = 12
    fmt = payload = None
    while pos+8 <= len(data):
        kind = data[pos:pos+4]
        size = struct.unpack_from('<I', data, pos+4)[0]
        start = pos+8
        if start+size > len(data):
            raise ValueError('truncated chunk')
        if kind == b'fmt ':
            if fmt is not None or size < 16:
                raise ValueError('duplicate/short fmt')
            fmt = struct.unpack_from('<HHIIHH', data, start)
        if kind == b'data':
            if payload is not None or not size or size%2:
                raise ValueError('duplicate/empty/odd PCM payload')
            payload = (start, size)
        pos = start+size+(size%2)
    if pos != len(data) or fmt != (1, 1, 24000, 48000, 2, 16) or payload is None:
        raise ValueError('mono PCM16 24kHz required')
    out = bytearray(data)
    start, size = payload
    for offset in range(start, start+size, 2):
        sample = struct.unpack_from('<h', data, offset)[0]
        scaled = abs(sample)*percent//100
        struct.pack_into('<h', out, offset, -scaled if sample < 0 else scaled)
    return bytes(out)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def prepare(candidate, out):
    if out.exists():
        raise ValueError('refuse existing output')
    manifest = candidate/'manifest.jsonl'
    qa = candidate/'qa.json'
    rows = [json.loads(l) for l in manifest.read_text(encoding='utf-8-sig').splitlines() if l]
    qa_rows = json.loads(qa.read_text(encoding='utf-8-sig'))
    if len(rows) != 1886 or len({r['line_id'] for r in rows}) != 1886 or len(qa_rows) != 1886:
        raise ValueError('exact1886 identity/QA required')
    if {r['line_id'] for r in rows} != {r['line_id'] for r in qa_rows}:
        raise ValueError('QA identity mismatch')
    if sum(r['pack'] == 'full_localization' for r in rows) != 894 or sum(r['pack'] == 'story_nllb' for r in rows) != 992:
        raise ValueError('exact894 UI /992 story required')
    # Check every source before creating the candidate; basename cannot escape output.
    sources = []
    for row in rows:
        src = Path(row['file'])
        if not re.fullmatch(r'[a-zA-Z0-9_-]+\.wav', src.name):
            raise ValueError('invalid source WAV basename')
        data = src.read_bytes()
        validated = attenuate(data, 100)
        if validated != data:
            raise ValueError('identity PCM validation failed')
        sources.append((src, sha(data)))
    if len({s.name for s, _ in sources}) != 1886:
        raise ValueError('duplicate output name')
    out.mkdir(parents=True)
    qa_by_id = {r['line_id']:r for r in qa_rows}
    report = []
    for row, (src, expected) in zip(rows, sources):
        data = src.read_bytes()
        if sha(data) != expected:
            raise ValueError('source changed while preparing')
        ui = row['pack'] == 'full_localization'
        result = attenuate(data) if ui else data
        target = out/src.name
        target.write_bytes(result)
        row['file'] = str(target.resolve())
        row['ui_pcm_gain_percent'] = 35 if ui else 100
        qa_by_id[row['line_id']]['source_qa_file'] = qa_by_id[row['line_id']]['file']
        qa_by_id[row['line_id']]['file'] = row['file']
        qa_by_id[row['line_id']]['pcm_attenuation_percent'] = 35 if ui else 100
        report.append(dict(line_id=row['line_id'], ui=ui, source_sha256=expected,
                           candidate_sha256=sha(result), bytes=len(result)))
    (out/'manifest.jsonl').write_text('\n'.join(json.dumps(r, ensure_ascii=False) for r in rows)+'\n', encoding='utf-8')
    (out/'qa.json').write_text(json.dumps(qa_rows,ensure_ascii=False,indent=2),encoding='utf-8') # inherited QA, not a new audition
    (out/'quiet-ui.json').write_text(json.dumps(dict(source_manifest_sha256=sha(manifest.read_bytes()),
        gain_percent=35, ui=894, story_unchanged=992, files=report,
        status='OFFLINE_PCM_PASS_NOT_AUDITORY_ACCEPTANCE'), indent=2), encoding='utf-8')


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--candidate', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    prepare(args.candidate, args.out)
    print('PASS: 894 UI PCM35%, 992 story byte-identical; no Steam writes')
