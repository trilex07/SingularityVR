"""Read-only Centurion log correlation, NOT native reload/ABI acceptance.

The producer logs HUD changes, not every sample, and ammo rows omit pawn ID.
Even conserved deltas plus a callback therefore establish correlation only.
No game access, external dependencies, output-file writes or engine calls.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re


MAX_BYTES = 256 * 1024 * 1024
MAX_LINE = 65536
MAX_RECORDS = 4096
NAMES = ('ServerStartReload', 'ReloadFinished', 'AttemptReload', 'IsReloading',
         'AnimNotify_RefillClip')
HEX = r'([0-9A-Fa-f]{8})'
AMMO = re.compile(
    r'centurion reload ammo: weapon=' + HEX + r' hud=' + HEX + r' class=' + HEX +
    r' clip=(-?\d+) reserve=(-?\d+) size=(-?\d+) t=(\d+) \(observer only\)$')
PROBE = re.compile(
    r'centurion reload probe: (' + '|'.join(NAMES) + r') self=' + HEX +
    r' (\S+) weapon=' + HEX + r' pawn=' + HEX +
    r' t=(\d+) \(entry only, no ABI call\)$')
HEADER = re.compile(r'^=== view_matrix attached .* - run starts here ===$')


def parse(lines):
    """Keep sessions separate; sort by producer ticks, not async file order."""
    runs = []
    current = None
    records = 0
    for number, line in enumerate(lines, 1):
        if len(line) > MAX_LINE:
            raise ValueError('line exceeds safe limit')
        line = line.rstrip('\r\n')
        if HEADER.fullmatch(line):
            current = dict(header=line, ammo=[], probes=[], malformed_lines=[], bindings=0)
            runs.append(current)
            continue
        if 'centurion reload' not in line:
            continue
        if current is None:
            current = dict(header=None, ammo=[], probes=[], malformed_lines=[], bindings=0)
            runs.append(current)
        records += 1
        if records > MAX_RECORDS:
            raise ValueError('too many relevant records; split log by session')
        match = AMMO.fullmatch(line)
        if match:
            weapon, hud, klass, clip, reserve, size, tick = match.groups()
            current['ammo'].append(dict(line=number, weapon=weapon.upper(),
                hud=hud.upper(), klass=klass.upper(), clip=int(clip),
                reserve=int(reserve), size=int(size), t=int(tick)))
        elif (match := PROBE.fullmatch(line)):
            name, self_id, obj, weapon, pawn, tick = match.groups()
            current['probes'].append(dict(line=number, name=name, self=self_id.upper(),
                object=obj, weapon=weapon.upper(), pawn=pawn.upper(), t=int(tick)))
        elif line.startswith('centurion reload ammo:') or (
            line.startswith('centurion reload probe:') and
            not line.startswith('centurion reload probe: name attempt=')):
            current['malformed_lines'].append(number)
        elif line.startswith('centurion reload binding:'):
            current['bindings'] += 1  # structural only; never treated as execution
    return runs


def valid_ammo(row):
    return (all(row[k] != '00000000' for k in ('weapon', 'hud', 'klass')) and
            0 <= row['clip'] <= row['size'] <= 1000 and row['size'] > 0 and
            0 <= row['reserve'] <= 1000000 and 0 <= row['t'] < 2**64)


def valid_probe(row):
    expected = row['pawn'] if row['name'] == 'AnimNotify_RefillClip' else row['weapon']
    return (row['self'] == expected and row['pawn'] != '00000000' and
            row['weapon'] != '00000000' and 0 <= row['t'] < 2**64)


def analyze(lines, window_ms=5000):
    if not 1 <= window_ms <= 60000:
        raise ValueError('window-ms must be in 1..60000')
    summaries = []
    for run in parse(lines):
        ammo = sorted(run['ammo'], key=lambda r: (r['t'], r['line']))
        probes = [r for r in run['probes'] if valid_probe(r)]
        bad_probes = len(run['probes']) - len(probes)
        warnings = []
        if run['header'] is None:
            warnings.append('missing session header; capture may be partial')
        if run['malformed_lines']:
            warnings.append('malformed relevant rows; transfer correlation disabled')
        if bad_probes:
            warnings.append('probe self/weapon/pawn identity or tick invalid')
        counts = Counter(r['name'] for r in run['probes'])
        if len(ammo) >= 128 or any(n >= 16 for n in counts.values()):
            warnings.append('producer log cap may be reached; absence is inconclusive')
        if any(a['t'] > b['t'] for a, b in zip(run['ammo'], run['ammo'][1:])):
            warnings.append('ammo rows reordered by tick; async file order differed')
        transfers = []
        for before, after in zip(ammo, ammo[1:]):
            # Never bridge identity switches, invalid reads, capacity changes,
            # simultaneous observations or unbounded idle gaps.
            if run['malformed_lines'] or not valid_ammo(before) or not valid_ammo(after):
                continue
            if any(before[k] != after[k] for k in ('weapon', 'hud', 'klass', 'size')):
                continue
            gap = after['t'] - before['t']
            gain = after['clip'] - before['clip']
            loss = before['reserve'] - after['reserve']
            if not 0 < gap <= window_ms or not gain > 0 or gain != loss:
                continue
            nearby = sorted((r for r in probes if r['weapon'] == after['weapon'] and
                before['t'] <= r['t'] <= after['t']), key=lambda r: (r['t'], r['line']))
            # Polling IsReloading is not a begin/commit/end callback.
            callbacks = [r for r in nearby if r['name'] != 'IsReloading']
            pawns = sorted({r['pawn'] for r in nearby})
            correlated = bool(callbacks) and len(pawns) == 1
            transfers.append(dict(before_line=before['line'], after_line=after['line'],
                weapon=after['weapon'], hud=after['hud'], amount=gain,
                before_counts=[before['clip'], before['reserve']],
                after_counts=[after['clip'], after['reserve']], interval_ms=gap,
                callback_pawns=pawns, callbacks=nearby,
                classification='CORRELATION_ONLY' if correlated else 'CONSERVED_DELTA_ONLY'))
        summaries.append(dict(header=run['header'], ammo_rows=len(ammo),
            invalid_ammo_rows=sum(not valid_ammo(r) for r in ammo),
            valid_probe_rows=len(probes), invalid_probe_rows=bad_probes,
            probe_counts=dict(sorted(counts.items())), binding_rows=run['bindings'],
            malformed_lines=run['malformed_lines'], warnings=warnings, transfers=transfers))
    return dict(scope='OFFLINE_LOG_ANALYSIS', native_reload_proven=False,
        limits=['HUD is read-only, not proven authoritative inventory',
                'ammo rows omit pawn and unchanged samples; no freshness/ownership proof',
                'callbacks are interpreter entry only; no invocation ABI inferred',
                'counts can hide intervening actions; timing is correlation, not causality',
                'missing events may reflect hook coverage, caps or partial capture',
                'no Steam/OpenXR/PSVR2 acceptance from this report'],
        max_interval_ms=window_ms, sessions=summaries)


def read_log(path):
    if path.stat().st_size > MAX_BYTES:
        raise ValueError('log exceeds 256 MiB; split log by session')
    digest = hashlib.sha256()
    lines = []
    total = 0
    with path.open('rb') as source:
        while raw := source.readline(MAX_LINE + 1):
            total += len(raw)
            if len(raw) > MAX_LINE or total > MAX_BYTES:
                raise ValueError('log grew beyond byte/line limit')
            digest.update(raw)
            # Retain relevant rows/headers and preserve line numbers without
            # keeping an entire large renderer log in memory.
            line = raw.decode('utf-8', errors='replace')
            lines.append(line if b'centurion reload' in raw or
                         b'=== view_matrix attached ' in raw else '')
            if len(lines) > 2000000:
                raise ValueError('too many input lines; split log by session')
    return lines, digest.hexdigest()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log', type=Path, required=True)
    parser.add_argument('--window-ms', type=int, default=5000,
                        help='conservative maximum changed-sample gap, not an engine constant')
    args = parser.parse_args()
    try:
        lines, sha = read_log(args.log)
        report = analyze(lines, args.window_ms)
        report.update(source=str(args.log.resolve()), source_sha256=sha)
        print(json.dumps(report, indent=2))
    except (OSError, ValueError) as error:
        parser.exit(2, f'analysis failed: {error}\n')
