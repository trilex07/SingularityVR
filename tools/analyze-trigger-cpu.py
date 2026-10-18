"""Stream private xperf sample/image CSV; never publish game code or other processes."""
import argparse
import bisect
import collections
import csv
import json
import pathlib
import re
import struct

def field_key(value):
    return re.sub(r'[^a-z0-9]', '', value.lower())

def number(value):
    try:
        return int(value.strip(), 0)
    except ValueError:
        return None

def map_symbols(map_path, image_path):
    """Refuse stale MSVC maps. Resolve IP self samples, NOT untrusted x86 callers."""
    raw=image_path.read_bytes()
    pe=struct.unpack_from('<I',raw,0x3c)[0]
    if raw[pe:pe+4]!=b'PE\0\0' or struct.unpack_from('<H',raw,pe+4)[0]!=0x14c or struct.unpack_from('<H',raw,pe+24)[0]!=0x10b:
        raise ValueError('Expected exact x86 PE32 candidate image')
    timestamp=struct.unpack_from('<I',raw,pe+8)[0]
    base=struct.unpack_from('<I',raw,pe+24+28)[0]
    text=map_path.read_text()
    stamp=re.search(r'Timestamp is ([0-9a-fA-F]+)',text)
    preferred=re.search(r'Preferred load address is ([0-9a-fA-F]+)',text)
    if not stamp or int(stamp[1],16)!=timestamp or not preferred or int(preferred[1],16)!=base:
        raise ValueError('Map timestamp/preferred base does not match candidate image')
    # Only actual executable sections; data-address "callers" from imperfect
    # WOW64/FPO unwinding must not be named as executing renderer functions.
    section_count=struct.unpack_from('<H',raw,pe+6)[0]
    opt_size=struct.unpack_from('<H',raw,pe+20)[0]
    ranges=[]
    for n in range(section_count):
        section=pe+24+opt_size+n*40
        size,rva=struct.unpack_from('<II',raw,section+8)
        flags=struct.unpack_from('<I',raw,section+36)[0]
        if flags&0x20000000:ranges.append((rva,rva+size))
    symbols=sorted((int(address,16)-base,name) for name,address in re.findall(
        r'^\s+0001:[0-9a-fA-F]+\s+(\S+)\s+([0-9a-fA-F]{8})\s',text,re.M))
    keys=[rva for rva,_ in symbols]
    def resolve(rva):
        if not any(start<=rva<end for start,end in ranges):return None
        index=bisect.bisect_right(keys,rva)-1
        return symbols[index][1] if index>=0 else None
    return resolve


def analyze(path, pid, stack_thread=None, symbol_map=None, symbol_image=None, symbol_base=None):
    if any(v is not None for v in (symbol_map,symbol_image,symbol_base)) and not all(v is not None for v in (symbol_map,symbol_image,symbol_base)):
        raise ValueError('Symbol analysis requires map, exact image and observed module base together')
    symbolize=map_symbols(symbol_map,symbol_image) if symbol_map is not None else None
    headers, modules = {}, {}
    samples = collections.Counter()
    by_thread = collections.defaultdict(collections.Counter)
    bins = collections.defaultdict(collections.Counter)
    stack_bins = collections.defaultdict(collections.Counter)
    pending_stack = None
    pending_frames = set()
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
                    stack_fields = dict(zip(headers.get(event, []), [item.strip() for item in row[1:]]))
                    # Current xperf dumper: one frame per row (No., Address,
                    # Image!Function). Do not count frame ordinals as functions,
                    # or discard all callers after the first row.
                    if 'no' in stack_fields and 'imagefunction' in stack_fields:
                        frames = {stack_fields['imagefunction']}
                    else:  # Older inline-stack export / synthetic legacy oracle.
                        frames = set(item.strip() for item in row[3:])
                    frames.discard('')
                    fresh = frames - pending_frames
                    stack_bins[timestamp//1000000].update(fresh)
                    pending_frames.update(frames)
                continue
            if event not in ('I-Start', 'I-DCStart', 'I-DCEnd', 'SampledProfile'):
                continue
            fields = dict(zip(headers.get(event, []), [item.strip() for item in row[1:]]))
            target = re.fullmatch(r'Singularity\.exe\s*\(\s*' + str(pid) + r'\s*\)', fields.get('processnamepid', ''), re.I)
            if not target:
                continue
            if event in ('I-Start', 'I-DCStart', 'I-DCEnd'):
                base, end = number(fields.get('baseaddr', '')), number(fields.get('endaddr', ''))
                if base is not None and end is not None and end > base:
                    filename=fields.get('filename', '').replace('\\', '/')
                    modules[base] = {'base': base, 'end': end, 'name': filename.rsplit('/', 1)[-1], 'path': filename}
            elif field_key(event) == 'sampledprofile':
                ip = number(fields.get('instructionpointer', fields.get('ip', fields.get('prgrmctr', ''))))
                tid, timestamp = number(fields.get('threadid', '')), number(fields.get('timestamp', ''))
                if ip is None or tid is None or timestamp is None:
                    continue
                samples[ip] += 1
                by_thread[tid][ip] += 1
                bins[(timestamp // 1000000, tid)][ip] += 1
                pending_stack=(timestamp, tid) if tid==stack_thread else None
                pending_frames=set()
    def resolve(ip, count):
        for module in modules.values():
            if module['base'] <= ip < module['end']:
                return {'module': module['name'], 'module_base':hex(module['base']), 'path':module['path'], 'rva': hex(ip-module['base']), 'address': hex(ip), 'samples': count}
        return {'module': 'UNKNOWN', 'address': hex(ip), 'samples': count}
    symbol_hits={}
    if symbolize:
        if symbol_base not in modules:raise ValueError('Requested symbol module base is absent from target PID images')
        for tid,counts in by_thread.items():
            hits=collections.Counter()
            for ip,count in counts.items():
                if modules[symbol_base]['base']<=ip<modules[symbol_base]['end']:
                    name=symbolize(ip-symbol_base)
                    if name:hits[name]+=count
            symbol_hits[str(tid)]=hits.most_common(30)
    return {'pid': pid, 'sample_count': samples.total(), 'modules': list(modules.values()),
            'symbol_self_hits':symbol_hits,
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
    parser.add_argument('--symbol-map', type=pathlib.Path)
    parser.add_argument('--symbol-image', type=pathlib.Path)
    parser.add_argument('--symbol-base', type=lambda value:int(value,0))
    args = parser.parse_args()
    print(json.dumps(analyze(args.csv, args.pid, args.stack_thread,args.symbol_map,args.symbol_image,args.symbol_base), indent=2))
