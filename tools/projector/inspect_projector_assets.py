"""Read-only evidence for projector LightFunction ownership in a logical UE3 package.

Run on private decompressed packages. Names/outer links are asset evidence,
not proof of the live shader or of visible pixels. Never changes a game package.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'title'))
from ue3_layout import inspect


def report(path):
    path = Path(path)
    if path.stat().st_size > 32 * 1024 * 1024:
        raise ValueError('Offline package exceeds 32 MiB budget')
    data = path.read_bytes()
    package = inspect(path, data, include_all=True)
    exports, imports, names = package['exports'], package['imports'], package['names']

    def identity(index):
        if not (-len(imports) <= index <= len(exports)):
            raise ValueError('Object reference outside package table')
        if index < 0:
            row = imports[-index - 1]
            return dict(index=index, name=names[row[5]], class_name=names[row[2]], outer=row[4])
        if index > 0:
            row = exports[index - 1]
            return {key: row[key] for key in ('index', 'name', 'class_name', 'outer')}
        return dict(index=0, name='None', class_name='None', outer=0)

    def chain(index):
        result, seen = [], set()
        while index:
            if index in seen or len(seen) >= 64:
                raise ValueError('Cyclic or excessive outer chain')
            seen.add(index)
            node = identity(index)
            result.append(node)
            index = node['outer']
        return result

    functions = []
    for entry in exports:
        if entry['class_name'] != 'LightFunction':
            continue
        if 'parse_error' in entry:
            raise ValueError('Cannot prove LightFunction properties')
        source = [p for p in entry['properties'] if p['name'] == 'SourceMaterial']
        if len(source) != 1 or source[0]['type'] != 'ObjectProperty' or source[0]['tag_bytes'] != 28:
            raise ValueError('Expected one tagged SourceMaterial object reference')
        offset = source[0]['tag_offset'] + 24
        target = struct.unpack_from('<i', data, offset)[0]
        functions.append(dict(owner_chain=chain(entry['index']), material_chain=chain(target)))
    hints = [identity(-i - 1) for i, row in enumerate(imports)
             if any(word in names[row[5]].lower() for word in ('projector', 'movie_screen'))]
    return dict(status='OFFLINE_OWNERSHIP_ONLY', package=path.name,
                sha256=hashlib.sha256(data).hexdigest(), light_functions=functions,
                related_imports=hints,
                warning='Component properties are not parsed here; shader, texture and live draw identity remain unproven.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package', type=Path)
    args = parser.parse_args()
    print(json.dumps(report(args.package), indent=2))
