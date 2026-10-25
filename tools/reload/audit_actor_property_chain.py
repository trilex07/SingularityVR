"""Read-only reproduction of the Actor UField chain (private Engine.u required)."""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'title'))
from ue3_layout import inspect


def audit(path):
    data = path.read_bytes()
    records = inspect(path, data=data, include_all=True)['records']
    actor, = [r for r in records if r['name'] == 'Actor' and r['cls'] == 0]
    children = {r['index']: r for r in records if r['outer'] == actor['index']}
    # Serialized UObject prefix is 16 bytes; UField.Next follows it. Resolve
    # the chain head independently from the set of direct child exports.
    links = {i: struct.unpack_from('<i', data, r['offset'] + 16)[0]
             for i, r in children.items()}
    head, = set(children) - set(links.values())
    chain = []
    seen = set()
    current = head
    while current:
        assert current in children and current not in seen
        seen.add(current)
        chain.append(children[current]['name'])
        current = links[current]
    assert len(chain) == len(children) == 458
    assert chain.index('Instigator') + 1 == 417
    assert chain.index('Location') + 1 == 304
    return dict(package_sha256=hashlib.sha256(data).hexdigest(),
                actor_export=actor['index'], direct_fields=len(chain),
                instigator_ordinal=chain.index('Instigator') + 1,
                old_walk_budget=400, corrected_walk_budget=4096,
                old_budget_reaches_instigator=False,
                limitation='Serialized source order, not live UObject memory proof')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('engine', type=Path)
    p.add_argument('--report', type=Path, required=True)
    args = p.parse_args()
    result = audit(args.engine)
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
