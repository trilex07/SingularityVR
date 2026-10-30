"""Recover exact Seeker first-person DXT blocks from the private cooked cache."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'textures'))
from audit_cooked_texture import inspect, properties, mips, sha


def main(root):
    package = root / 'wp_SP_ARSteerable_SF.u'
    cache = root / 'Textures_P0.tfc'
    data = package.read_bytes()
    pkg = inspect(data)
    out = root / 'native_identity'
    out.mkdir(exist_ok=True)
    report = {}
    for name in ('ar_steerable_d', 'ar_steerable_n'):
        e, = [e for e in pkg['exports'] if e['class_name'] == 'Texture2D' and e['name'] == name]
        props, cursor = properties(data, pkg['names'], e['offset'] + 4, e['offset'] + e['size'])
        chain = mips(data, cursor, e['offset'] + e['size'], props['Format'])
        mip = chain[0]
        if mip['flags'] != 0x11 or (mip['width'], mip['height']) != (2048, 1024):
            raise ValueError('true mip0 unavailable')
        raw = out / (name + '.mip0.bin')
        if not raw.exists():
            subprocess.run([str(root/'read_tfc_bulk.exe'), str(cache), str(mip['bulk_offset']),
                            str(mip['stored_size']), str(mip['raw_size']), str(raw)], check=True)
        blocks = raw.read_bytes()
        if len(blocks) != mip['raw_size']:
            raise ValueError('wrong compressed block size')
        fnv = 0xcbf29ce484222325
        for byte in blocks:
            fnv = ((fnv ^ byte) * 0x100000001b3) & 0xffffffffffffffff
        report[name] = dict(format=props['Format'], width=2048, height=1024,
                            fnv1a64=f'{fnv:016X}', mip0_sha256=sha(raw),
                            package_sha256=sha(package), cache_sha256=sha(cache),
                            export_index=e['index'], mip=mip,
                            properties={k:v for k,v in props.items() if isinstance(v,(str,int))})
    (out/'identity.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--private', type=Path, required=True)
    main(p.parse_args().private.resolve())
