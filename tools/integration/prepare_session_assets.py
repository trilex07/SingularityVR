"""Offline campaign asset intake. No install discovery, launch or Steam writes.

Audit the entire SIDE audio manifest, stage campaign sources for native cooking,
patch the existing text lookup (not a stale wholesale localization pack), and
stage only the requested original Revert 8K pair. Outputs never contain MP or UI
audio. A ready intake is NOT a ready native dialogue installation.
"""
import argparse
import array
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct
import sys
import wave

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools/audio'))
from patch_note_manifest import patch


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def leaf(name):
    require(isinstance(name, str) and re.fullmatch(r'[A-Za-z0-9_.-]+', name)
            and name not in ('.', '..'), 'unsafe filename')
    return name


def pcm_info(data):
    import io
    with wave.open(io.BytesIO(data)) as w:
        require((w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getcomptype())
                == (1, 2, 24000, 'NONE'), 'expected PCM16 mono 24kHz')
        n = w.getnframes()
        raw = w.readframes(n)
    require(n > 0 and len(raw) == n*2, 'empty/truncated PCM')
    samples = array.array('h', raw)
    if sys.byteorder != 'little':
        samples.byteswap()
    peak = max(abs(x) for x in samples)
    require(peak > 0 and peak < 32767, 'silent/clipped audio')
    return dict(frames=n, rate=24000, peak_sample=peak)


def normalized_pcm(data):
    """Four unchanged intro takes remain FLOAT32 despite the handoff wording.

    Preserve every sample/time position, gain=1; quantize only private copies.
    Reject malformed chunks, non-finite values and clipping, never guess rate.
    """
    require(data[:4] == b'RIFF' and data[8:12] == b'WAVE' and
            struct.unpack_from('<I', data, 4)[0]+8 == len(data), 'invalid RIFF size')
    at = 12
    chunks = {}
    while at < len(data):
        require(at+8 <= len(data), 'truncated WAV chunk')
        kind, n = struct.unpack_from('<4sI', data, at)
        require(at+8+n <= len(data), 'truncated WAV payload')
        if kind in (b'fmt ', b'data'):
            require(kind not in chunks, 'duplicate WAV chunk')
            chunks[kind] = data[at+8:at+8+n]
        at += 8+n+(n & 1)
    require(at == len(data) and b'fmt ' in chunks and b'data' in chunks, 'invalid WAV chunks')
    fmt = chunks[b'fmt ']
    require(len(fmt) >= 16, 'short WAV format')
    tag, channels, rate, byte_rate, align, bits = struct.unpack_from('<HHIIHH', fmt)
    if tag == 1:
        return data, pcm_info(data), False
    require((tag, channels, rate, byte_rate, align, bits) == (3, 1, 24000, 96000, 4, 32),
            'unsupported non-PCM WAV')
    raw = chunks[b'data']
    require(raw and len(raw) % 4 == 0, 'invalid float PCM size')
    samples = [x[0] for x in struct.iter_unpack('<f', raw)]
    require(all(math.isfinite(x) and abs(x) < 1 for x in samples), 'float PCM nonfinite/clipped')
    import io
    out = io.BytesIO()
    with wave.open(out, 'wb') as w:
        w.setparams((1, 2, 24000, 0, 'NONE', 'not compressed'))
        w.writeframes(b''.join(struct.pack('<h', round(x*32767)) for x in samples))
    result = out.getvalue()
    return result, pcm_info(result), True


def dds_info(data):
    require(len(data) >= 128 and data[:4] == b'DDS ' and
            struct.unpack_from('<I', data, 4)[0] == 124, 'invalid DDS header')
    h, w = struct.unpack_from('<2I', data, 12)
    levels = struct.unpack_from('<I', data, 28)[0]
    fmt = data[84:88]
    require(fmt in (b'DXT1', b'DXT5') and w == h and 4 <= w <= 8192 and
            w & (w-1) == 0 and 1 <= levels <= w.bit_length(), 'DDS dimensions/format')
    block = 8 if fmt == b'DXT1' else 16
    size = sum(max(1, (max(1, w >> m)+3)//4)**2*block for m in range(levels))
    require(len(data) == 128+size, 'DDS byte/mip count mismatch')
    require(max(1, w >> (levels-1)) <= 4, 'incomplete DDS mip chain')
    return dict(width=w, height=h, levels=levels, format=fmt.decode(), bytes=len(data))


def audit_audio(root, manifest):
    rows = [json.loads(x) for x in manifest.read_text(encoding='utf-8-sig').splitlines() if x.strip()]
    unique = {}
    for r in rows:
        identity = (r['pack'], leaf(r['file']).lower())
        if identity in unique:
            old = unique[identity]
            # SIDE lists 67 story +7 intro files twice under two origins.
            # Coalesce only cryptographically identical declarations, not an
            # arbitrary last-writer-wins resolution of competing audio takes.
            require(bool(old.get('sha256')) and old.get('sha256') == r.get('sha256')
                    and old.get('sha16') == r.get('sha16'), 'conflicting duplicate delivery')
            continue
        unique[identity] = r
    rows = list(unique.values())
    expected = dict(story=1084, barks=268, mp=409, skipped=97, vignette=6, intro=11)
    require(dict(Counter(r['pack'] for r in rows)) == expected, 'unexpected SIDE delivery inventory')
    seen = set()
    audited = []
    for r in rows:
        pack, name = r['pack'], leaf(r['file'])
        require((pack, name.lower()) not in seen and name.lower().endswith('.wav'), 'duplicate/non-WAV delivery')
        seen.add((pack, name.lower()))
        path = root/pack/name
        data = path.read_bytes()
        digest = sha(data)
        require(re.fullmatch('[0-9a-fA-F]{16}', r.get('sha16', '')) and
                digest.startswith(r['sha16'].lower()), 'SIDE sha16 mismatch: '+str(path))
        if r.get('sha256'):
            require(digest == r['sha256'].lower(), 'SIDE SHA256 mismatch: '+str(path))
        try:
            normalized, info, converted = normalized_pcm(data)
        except Exception as ex:
            raise ValueError(f'{path}: {ex}') from ex
        audited.append(dict(pack=pack, file=name, source=str(path.resolve()), sha256=digest,
                            staged_sha256=sha(normalized), float_to_pcm16=converted, **info))
    return audited


def translations(manifest, reference):
    delivery = json.loads(manifest.read_text(encoding='utf-8-sig'))
    sections = delivery['Sections']
    require({s['SectionName']: len(s['Items']) for s in sections} == dict(Notes=72, ChronoNote=32),
            'unexpected note delivery inventory')
    original = {}
    for line in reference.read_text(encoding='utf-8-sig').splitlines():
        if not line.strip():
            continue
        r = json.loads(line)
        if r.get('section') in ('Notes', 'ChronoNote') and r.get('record', '').endswith('RvGame.int'):
            key = (r['section'], r['key'])
            require(key not in original, 'ambiguous reference text')
            original[key] = r.get('en', r.get('text'))
            require(isinstance(original[key], str), 'reference lacks native English text')
            if r.get('utf8_sha256'):
                require(sha(original[key].encode('utf-8')) == r['utf8_sha256'], 'reference inventory hash mismatch')
    result = []
    reviewed_source_pairs = {
        # Individually compared against the complete native English inventory
        # and SIDE Polish text (not permission for arbitrary hash drift).
        'VI5_Diary_1': ('d308b80700fd0c81315198f17ac445a7cd0fe1bca2fe95360bc4b9175b5cfacf',
                        '855a124ddb3186683ad093ed7c69658546ea4e0cacfe864c260e5192d8aa0a35'),
        'WL6_electro_mag_note': ('7ed9d3a81bccc402fea31f0ea29f4595c20037db12f0bdfee3136b77784af0c0',
                               '307453579e9be7e126b9e40f874f84db05e6e4c72b7d31d3e759b2131b84adfc'),
    }
    for s in sections:
        for r in s['Items']:
            key = (s['SectionName'], r['key'])
            require(key in original, 'unknown native note identity: '+str(key))
            expected = sha(original[key].encode('utf-8'))
            # SIDE omitted English hashes for ChronoNote. Establish them from
            # the versioned native-English inventory, never from the target's
            # possibly changed translation. patch() independently checks target.
            row = dict(r, section=key[0], source_sha256=expected)
            declared = r.get('source_sha256', expected)
            if declared != expected:
                if key == ('Notes', 'FR10_Samuson'):
                    native = original[key]
                    require(sha(native.strip().encode('utf-8')) == declared, 'Samuson source drift')
                    # SIDE trimmed two leading newlines. Preserve that layout
                    # instead of weakening the paragraph-preservation gate.
                    row['pl'] = native[:len(native)-len(native.lstrip())]+row['pl']
                else:
                    require(reviewed_source_pairs.get(key[1]) == (declared, expected),
                            'SIDE/reference text disagreement: '+str(key))
            if key == ('Notes', 'WL6_electro_mag_note'):
                require(row['pl'].endswith('Dr Pietrowa'), 'reviewed signature changed')
                row['pl'] = row['pl'][:-len('Dr Pietrowa')]+'Dr Pietrow'
            native = original[key]
            prefix = native[:len(native)-len(native.lstrip())]
            if prefix and not row['pl'].startswith(prefix):
                row['pl'] = prefix+row['pl'].lstrip()
            result.append(row)
    return result


def prepare(a):
    out = a.out.resolve()
    allowed = (ROOT/'build/next-weapon-reload').resolve()
    require(out != allowed and out.is_relative_to(allowed) and not out.exists(),
            'use a NEW child of this checkout build/next-weapon-reload')
    audited = audit_audio(a.audio_root, a.audio_manifest)
    old_text = a.text.read_bytes()
    text = patch(old_text.decode('utf-8-sig'), translations(a.notes_manifest, a.reference_text)).encode('utf-8')
    tiers = json.loads(a.tier_manifest.read_text(encoding='utf-8-sig'))['tiers']
    texture_rows = []
    for name in ('revert_d2', 'revert_n2'):
        source = a.texture_root/(name+'__A_lanczos.dds')
        data = source.read_bytes()
        info = dds_info(data)
        require(info['width'] == 8192 and info['format'] == 'DXT1', 'not the reviewed original Revert 8K pair')
        # Research stored the 8K Revert proof inside its mixed "4" tier.
        # Select by independently checked dimensions/hash, never the label.
        witnesses = [items[name] for items in tiers.values() if name in items and items[name]['dims'] == [8192, 8192]]
        require(len(witnesses) == 1 and sha(data) == witnesses[0]['sha256'].lower(), '8K research manifest mismatch: '+name)
        texture_rows.append(dict(file=name+'.dds', source=str(source.resolve()), sha256=sha(data), **info))
    # All source validation above finishes before any output is created.
    out.mkdir(parents=True)
    for row in audited:
        if row['pack'] == 'mp':
            continue
        dest = out/'audio-sources'/row['pack']/row['file']
        dest.parent.mkdir(parents=True, exist_ok=True)
        data = Path(row['source']).read_bytes()
        require(sha(data) == row['sha256'], 'audio source changed during staging')
        normalized, _, _ = normalized_pcm(data)
        dest.write_bytes(normalized)
        require(sha(dest.read_bytes()) == row['staged_sha256'], 'audio copy mismatch')
    target = out/'payload/Binaries/vr-assets/localization/pl/polish_text.jsonl'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(text)
    for row in texture_rows:
        # Current accepted bridge routes these two names through remasterdir
        # when remaster_apply=1, even with Quality=8K. Putting them only in the
        # tier directory would silently retain the installed 4K remaster.
        target = out/'payload/Binaries/texture-override-remaster'/row['file']
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(row['source'], target)
        require(sha(target.read_bytes()) == row['sha256'], 'DDS copy mismatch')
    report = dict(schema='singularityvr-session-assets-intake-v1', steam_write=False,
                  status='INTAKE_ONLY_NATIVE_COOK_AND_LIVE_BASELINE_PENDING',
                  source_manifest_sha256=sha(a.audio_manifest.read_bytes()), audio=audited,
                  reference_text_sha256=sha(a.reference_text.read_bytes()),
                  campaign_staged=sum(r['pack'] != 'mp' for r in audited), multiplayer_staged=0, notes=104,
                  text_before_sha256=sha(old_text), text_after_sha256=sha(text), textures=texture_rows,
                  launch_timeout_seconds=None, restore_while_running=False,
                  physical_acceptance='PENDING',
                  note='Private audio sources are NOT runtime voicebanks. Do not deploy intake alone.')
    (out/'intake.json').write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
    print(f"PASS: {len(audited)} unique audio audited; {report['campaign_staged']} campaign sources +104 text entries +2 Revert 8K staged OFFLINE")
    print('Native bank cooking, consumer mapping and deployment lease still required.')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ('audio-root', 'audio-manifest', 'text', 'reference-text', 'notes-manifest', 'texture-root', 'tier-manifest', 'out'):
        ap.add_argument('--'+name, type=Path, required=True)
    prepare(ap.parse_args())
