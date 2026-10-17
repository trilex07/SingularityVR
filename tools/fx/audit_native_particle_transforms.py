"""Audit private driver traces for world particles misrouted through HUD.

No game access or writes. CTAB + exact bytecode identities determine register
ownership; left/right constants establish the faulty transform. Pixel ownership
and headset acceptance are deliberately not inferred from this report.
"""
import argparse
import json
import math
from pathlib import Path

from analyze_native_muzzle import trace

FAMILIES = {
    ('314D478A9CFDBE37', 'C118540CCDBCB241'): (3384, 448),
    ('488BAEB579E71A73', '27DFE7D9C67BCC03'): (2968, 452),
    ('4FAEF2389750DBFC', '2FEC493CD52D7D78'): (2908, 336),
}


def row_mul(point, matrix):
    return [sum(point[i] * matrix[i][j] for i in range(4)) for j in range(4)]


def half_remap(matrix, shift):
    result = [row[:] for row in matrix]
    for row in result:
        row[0] = 0.5 * row[0] + shift * row[3]
    return result


def undo_half(matrix, shift):
    result = [row[:] for row in matrix]
    for row in result:
        row[0] = 2 * (row[0] - shift * row[3])
    return result


def max_error(a, b):
    values = [abs(x - y) for ar, br in zip(a, b) for x, y in zip(ar, br)]
    return max(values) if all(math.isfinite(v) for v in values) else math.inf


def audit(path):
    data = trace.read_trace(path)
    if not data['complete_budget'] or data['shader_budget_overflow'] or data['shader_unavailable']:
        raise ValueError('Incomplete trace: cannot audit particle transform ownership')
    findings = []
    for index, left in data['groups'].items():
        right = data['groups'].get(index + 1)
        vs, ps = data['shaders'].get(left['VS']), data['shaders'].get(left['PS'])
        if not right or not vs or not ps:
            continue
        pair = vs['fnv1a64'], ps['fnv1a64']
        if FAMILIES.get(pair) != (vs['bytes'], ps['bytes']):
            continue
        if any(left[k] != right[k] for k in ('VS', 'PS', 'RT', 'vp', 'verts')):
            continue
        if left['textures'].get('0', {}).get('id') != right['textures'].get('0', {}).get('id'):
            continue
        viewport = [int(v) for v in left['vp'].split(',')]
        x, y, w, h = viewport
        if left['sc'] != f'{x},{y},{x+w//2},{y+h}' or right['sc'] != f'{x+w//2},{y},{x+w},{y+h}':
            continue
        params = vs['parameter_registers']
        if any(params.get(name) != dict(register_set=2, register_index=reg, register_count=4)
               for name, reg in [('ViewProjectionMatrix', 0), ('LocalToWorld', 5)]):
            raise ValueError('Known shader identity has unexpected CTAB layout')
        try:
            lc = [left['constants'][i]['VS'] for i in range(4)]
            rc = [right['constants'][i]['VS'] for i in range(4)]
            ll = [left['constants'][i]['VS'] for i in range(5, 9)]
            rl = [right['constants'][i]['VS'] for i in range(5, 9)]
        except KeyError:
            continue
        same_camera = max_error(lc, rc) < 1e-6
        local_changed = max_error(ll, rl) > 0.1
        recovered_error = max_error(undo_half(ll, -0.5), undo_half(rl, 0.5))
        findings.append(dict(pair=':'.join(pair), groups=[index, index+1],
            phase=left.get('geometry', {}).get('phase'),
            texture=left['textures'].get('0', {}).get('size'),
            camera_identical=same_camera, local_changed=local_changed,
            recovered_local_error=recovered_error,
            hud_world_transform_corruption=same_camera and local_changed and recovered_error < 0.002))
    return dict(trace=str(path), findings=findings,
                status='TRANSFORM_EVIDENCE_ONLY_NOT_PIXEL_ACCEPTANCE')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('traces', type=Path, nargs='+')
    args = parser.parse_args()
    print(json.dumps([audit(p) for p in args.traces], indent=2))
