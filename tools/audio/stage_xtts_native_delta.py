"""Offline staging of independently roundtripped XTTS banks for MAIN's asset lease.

No game writes/launch. Reports are input receipts, not assumed runtime acceptance.
Only exact approved-profile banks may change; no overlay/text/intro/UI targets.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[2]


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def entries(report, pins):
    seen = set()
    result = []
    for row in report:
        package = row['package']
        require(bool(re.fullmatch(r'SP_[A-Za-z0-9_]+_LOC_INT', package, re.I)), 'not a campaign voice bank')
        relative = 'RvGame/CookedPC/' + package + '.xxx'
        key = relative.casefold()
        require(key not in seen, 'duplicate bank receipt')
        seen.add(key)
        require(key in pins and row['before'].lower() == pins[key], 'unapproved input bank')
        if row['status'] == 'NO_SAFE_TAKES_ACCEPTED_BANK_RETAINED':
            require(row['after'] is None and not row['lines'], 'unsafe bank must stay unchanged')
            continue
        require(row['status'] == 'OFFLINE_CONTAINER_PASS_RUNTIME_PENDING' and row['lines'],
                'no independent decode receipt')
        require(bool(re.fullmatch(r'[0-9a-f]{64}', row['after'])), 'bad candidate hash')
        result.append(dict(path=relative, before=pins[key], after=row['after'], package=package))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, action='append', required=True)
    parser.add_argument('--profile', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    out = args.out.resolve()
    allowed = (ROOT / 'build/next-weapon-reload').resolve()
    require(out != allowed and out.is_relative_to(allowed), 'private asset-lease output required')
    require(not out.exists(), 'never overwrite a staged candidate')
    profile_data = args.profile.read_bytes()
    profile = json.loads(profile_data.decode('utf-8-sig'))
    require(profile['approved'] is True, 'baseline not approved')
    pins = {r['path'].casefold(): r['sha256'].lower() for r in profile['files']}
    rows, receipts = [], []
    seen = set()
    for report in args.report:
        raw = report.read_bytes()
        receipts.append(dict(path=str(report.resolve()), sha256=sha(raw)))
        for row in entries(json.loads(raw), pins):
            key = row['path'].casefold()
            require(key not in seen, 'overlapping reports')
            seen.add(key)
            source = report.parent / row['package'] / 'candidate.xxx'
            require(sha(source.read_bytes()) == row['after'], 'native candidate changed')
            rows.append((row, source))
    require(len(rows) >= 3, 'asset lease requires at least three verified targets')
    out.mkdir(parents=True)
    files = []
    for row, source in rows:
        target = out / 'payload' / row['path']
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        require(sha(target.read_bytes()) == row['after'], 'staged copy differs')
        files.append({k: row[k] for k in ('path', 'before', 'after')})
    protected = [dict(path=r['path'], sha256=r['sha256'].lower()) for r in profile['files']
                 if r['path'].casefold() not in seen]
    require(protected, 'protected baseline required')
    manifest = dict(schema='singularityvr-session-assets-delta-v1', files=files, protected=protected,
                    accepted_profile_sha256=sha(profile_data), source_reports=receipts,
                    status='OFFLINE_NATIVE_BANK_DELIVERY_RUNTIME_PENDING', requires_new_user_start=True,
                    steam_written=False)
    (out / 'asset-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print('Staged', len(files), 'native banks; no Steam writes; no runtime/visual acceptance')


if __name__ == '__main__':
    main()
