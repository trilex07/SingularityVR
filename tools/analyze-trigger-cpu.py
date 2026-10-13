"""Stream private xperf sample/image CSV; never publish game code or other processes."""
import argparse
import collections
import csv
import json
import pathlib
import re

def field_key(value):
    return re.sub(r'[^a-z0-9]', '', value.lower())

def number(value):
    try:
        return int(value.strip(), 0)
    except ValueError:
        return None

def analyze(path, pid, stack_thread=None):
    headers, modules = {}, {}
    samples = collections.Counter()
    by_thread = collections.defaultdict(collections.Counter)
    bins = collections.defaultdict(collections.Counter)
    stack_bins = collections.defaultdict(collections.Counter)
    pending_stack = None
    in_header = False
    with path.open(encoding='utf-8-sig', errors='replace', newline='') as stream:
        for row in csv.reader(stream, skipinitialspace=True):
            if not row:
                continue
            event = row[0].strip()
            if event == 'BeginHeader':
                in_header = True
                continue
            if event == 'EndHeader':
                in_header = False
                continue
            if in_header:
                headers[event] = [field_key(item) for item in row[1:]]
                continue
            if event == 'Stack' and pending_stack and len(row)>=4:
                timestamp, tid = number(row[1]), number(row[2])
                if (timestamp, tid)==pending_stack:
                    # Unique frame hits per sampled stack, not additive CPU times.
                    stack_bins[timestamp//1000000].update(set(item.strip() for item in row[3:]))
                    pending_stack=None
                continue
            fields = dict(zip(headers.get(event, []), [item.strip() for item in row[1:]]))
            target = re.fullmatch(r'Singularity\.exe\s*\(\s*' + str(pid) + r'\s*\)', fields.get('processnamepid', ''), re.I)
            if not target:
                continue
            if event in ('I-Start', 'I-DCStart', 'I-DCEnd'):
                base, end = number(fields.get('baseaddr', '')), number(fields.get('endaddr', ''))
                if base is not None and end is not None and end > base:
                    modules[base] = {'base': base, 'end': end, 'name': fields.get('filename', '').replace('\\', '/').rsplit('/', 1)[-1]}
            elif field_key(event) == 'sampledprofile':
                ip = number(fields.get('instructionpointer', fields.get('ip', fields.get('prgrmctr', ''))))
                tid, timestamp = number(fields.get('threadid', '')), number(fields.get('timestamp', ''))
                if ip is None or tid is None or timestamp is None:
                    continue
                samples[ip] += 1
                by_thread[tid][ip] += 1
                bins[(timestamp // 1000000, tid)][ip] += 1
                pending_stack=(timestamp, tid) if tid==stack_thread else None
    def resolve(ip, count):
        for module in modules.values():
            if module['base'] <= ip < module['end']:
                return {'module': module['name'], 'rva': hex(ip-module['base']), 'address': hex(ip), 'samples': count}
        return {'module': 'UNKNOWN', 'address': hex(ip), 'samples': count}
    return {'pid': pid, 'sample_count': samples.total(), 'modules': list(modules.values()),
            'hot': [resolve(ip, count) for ip, count in samples.most_common(30)],
            'threads': {str(tid): {'samples': counts.total(), 'hot': [resolve(ip, count) for ip, count in counts.most_common(12)]} for tid, counts in by_thread.items()},
            'seconds': [{'second': second, 'tid': tid, 'samples': counts.total(), 'hot': [resolve(ip, count) for ip, count in counts.most_common(3)]} for (second, tid), counts in sorted(bins.items())],
            'stack_thread': stack_thread,
            'sampled_stack_frame_hits': {str(second): counts.most_common(35) for second, counts in sorted(stack_bins.items())},
            'limitations': 'IP samples are not call stacks, press-duration proof, or function names. Image unload/reload address reuse is not modeled. CPU samples do not measure off-CPU waiting.'}

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('csv', type=pathlib.Path)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--stack-thread', type=int)
    args = parser.parse_args()
    print(json.dumps(analyze(args.csv, args.pid, args.stack_thread), indent=2))
