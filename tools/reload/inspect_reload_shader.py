"""Decode bounded existing RenderWords evidence OFFLINE. No game or injection.

Private shader bytes/assembly stay in the explicit ignored SIDE build folder.
Does not infer coordinate semantics from CTAB names or successful draw counts.
"""
import argparse
import hashlib
import importlib.util
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[2]


def extract(lines, identity):
    header = re.compile(rf'RenderBytecode id={identity} stage=VS words=(\d+)$')
    chunk = re.compile(rf'RenderWords id={identity} stage=VS at=(\d+)((?: [0-9A-Fa-f]{{8}})+)$')
    count = None
    words = []
    for line in lines:
        line = line.rstrip('\r\n')
        if match := header.fullmatch(line):
            if count is not None:
                raise ValueError('duplicate shader capture; split sessions')
            count = int(match[1])
            if not 2 <= count <= 4096:
                raise ValueError('shader size bound')
        elif match := chunk.fullmatch(line):
            if count is None or int(match[1]) != len(words):
                raise ValueError('missing header/chunk, duplicate or out of order')
            words += [int(w, 16) for w in match[2].split()]
            if len(words) > count:
                raise ValueError('extra shader words')
    if count is None or len(words) != count or words[0] != 0xfffe0300 or words[-1] != 0xffff:
        raise ValueError('incomplete/unexpected vs_3_0 capture')
    return struct.pack('<'+'I'*count, *words)


def run(log, identity, output):
    if log.stat().st_size > 256*1024*1024:
        raise ValueError('log exceeds limit')
    allowed = (ROOT/'build/centurion-reload').resolve()
    output = output.resolve()
    if not output.is_relative_to(allowed) or output == allowed or output.exists():
        raise ValueError('output must be NEW child of own ignored lab')
    raw = log.read_bytes()
    binary = extract(raw.decode('utf-8', errors='replace').splitlines(), identity)
    spec = importlib.util.spec_from_file_location('private_disassembler', ROOT/'tools/knife/inspect_shader.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    output.mkdir(parents=True)
    shader = output/'original.vs.bin'
    shader.write_bytes(binary)
    assembly = module.disassemble(shader)
    (output/'original.vs.asm').write_text(assembly, encoding='utf-8')
    print('log SHA256:', hashlib.sha256(raw).hexdigest())
    print('shader SHA256:', hashlib.sha256(binary).hexdigest())
    print('assembly:', output/'original.vs.asm')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log', type=Path, required=True)
    parser.add_argument('--id', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not 0 <= args.id < 24:
        parser.error('capture id must be0..23')
    run(args.log, args.id, args.output)
