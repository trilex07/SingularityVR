"""Export archived wrist receipts for the x86 action-pose regression. No game access.

These are FINAL material-0 matrices, not original animation uploads. Replaying
them validates the normalizer against real values, not the whole render pipeline.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import numpy as np
from prepare_centurion import read_psk


def export(psk, logs, matrices, report):
    names, _, _, bind, *_ = read_psk(psk)
    assert names[51] == 'r_arm_wrist'
    # read_psk has already undone the exporter mirror. This is the convention
    # of armbind::TmdBind, NOT the mirrored raw exported vertex coordinates.
    wrist = bind[51]
    records, evidence = [], []
    for log in logs:
        raw = log.read_bytes()
        rows = []
        for line in raw.decode('utf-8', errors='replace').splitlines():
            if 'tmd-right-wrist final:' not in line or 'eye=0' not in line:
                continue
            skin = np.array([float(x) for x in re.search(r'skin=\[([^]]+)\]', line)[1].replace(';', ',').split(',')]).reshape(3, 4)
            local = np.array([float(x) for x in re.search(r'local=\[([^]]+)\]', line)[1].replace(';', ',').split(',')]).reshape(4, 4)
            target = np.array([float(x) for x in re.search(r'targetRaw=([-\d,]+)', line)[1].split(',')]) / 1000.
            gpu = np.eye(4)
            gpu[:, :3] = skin.T
            records.append(' '.join(format(float(x), '.9g') for x in np.concatenate((skin.ravel(), local.ravel()))))
            row = dict(frame=int(re.search(r'frame=(\d+)', line)[1]),
                       hands=int(re.search(r'hands=(\d+)', line)[1]),
                       wrist=(wrist @ gpu @ local)[3, :3], target=target)
            if not rows or rows[-1]['frame'] != row['frame']:
                rows.append(row)
        jumps = []
        for a, b in zip(rows, rows[1:]):
            distance = float(np.linalg.norm(b['wrist'] - a['wrist']))
            controller = float(np.linalg.norm(b['target'] - a['target']))
            if 0 < b['frame'] - a['frame'] <= 3 and distance > 3 and controller < 1:
                jumps.append(dict(frames=[a['frame'], b['frame']], hands=[a['hands'], b['hands']],
                                  wrist_delta_uu=distance, controller_translation_uu=controller))
        evidence.append(dict(log=str(log), sha256=hashlib.sha256(raw).hexdigest(),
                             unique_frames=len(rows), jumps=jumps))
    matrices.write_text(str(len(records)) + '\n' + '\n'.join(records) + '\n')
    report.write_text(json.dumps(dict(records=len(records), logs=evidence), indent=2) + '\n')
    print(f'Exported {len(records)} final wrist matrices; full runtime validation remains open.')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--psk', type=Path, required=True)
    p.add_argument('--matrices', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('logs', type=Path, nargs='+')
    a = p.parse_args()
    export(a.psk, a.logs, a.matrices, a.report)
