"""Offline ownership-field audit of the user's extracted Engine.u; no game writes."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'title'))
from ue3_layout import inspect


def audit(path):
    data = inspect(path, include_all=True)
    records = data['records']
    expected = {
        ('Camera', 'PCOwner'): 'ObjectProperty',
        ('PlayerController', 'PlayerCamera'): 'ObjectProperty',
        ('Controller', 'Pawn'): 'ObjectProperty',
        ('Pawn', 'Controller'): 'ObjectProperty',
        ('Pawn', 'Mesh'): 'ComponentProperty',
        ('SkeletalMeshComponent', 'SkeletalMesh'): 'ObjectProperty',
    }
    found = []
    for e in data['exports']:
        owner = records[e['outer'] - 1]['name'] if e['outer'] > 0 else None
        key = (owner, e['name'])
        if key not in expected:
            continue
        assert e['class_name'] == expected[key], (key, e['class_name'])
        found.append(dict(owner=owner, field=e['name'], type=e['class_name'], export=e['index']))
    assert len(found) == len(expected)
    return dict(status='OFFLINE_FIELD_TYPES_PASS',
                package_sha256=hashlib.sha256(Path(path).read_bytes()).hexdigest(),
                fields=found, runtime_offsets='reflection only', physical_validation=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine', required=True)
    print(json.dumps(audit(parser.parse_args().engine), indent=2))
