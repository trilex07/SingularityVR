"""Read-only original UE3 animation-notify inventory (private decompressed copy).

Requires the existing table inspector. No native addresses, calls, asset edits
or runtime claims are produced; a named refill notify is not proof it executed.
"""
import argparse
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect


def fields(data,offset,end,names):
    if not 0 <= offset <= end <= len(data):
        raise ValueError('invalid field bounds')
    result = {}
    for _ in range(32):
        if offset+8 > end:
            raise ValueError('truncated notify name')
        index,number = struct.unpack_from('<ii',data,offset);offset+=8
        if not 0 <= index < len(names) or number:
            raise ValueError('unsupported notify field name')
        name = names[index]
        if name == 'None':
            return result,offset
        if name in result or offset+16 > end:
            raise ValueError('duplicate/truncated notify field')
        typ,tn,size,array = struct.unpack_from('<4i',data,offset);offset+=16
        if not 0 <= typ < len(names) or tn or array or size < 0 or offset+size > end:
            raise ValueError('invalid notify property range')
        kind = names[typ]
        if kind == 'FloatProperty' and size == 4:
            value = struct.unpack_from('<f',data,offset)[0]
            if not -1e6 < value < 1e6:
                raise ValueError('invalid notify time')
        elif kind == 'ObjectProperty' and size == 4:
            value = struct.unpack_from('<i',data,offset)[0]
        elif kind in ('NameProperty','ByteProperty') and size == 8:
            ni,nn = struct.unpack_from('<ii',data,offset)
            if not 0 <= ni < len(names) or nn:
                raise ValueError('invalid notify name value')
            value = names[ni]
        else:
            raise ValueError(f'unsupported notify field {name}/{kind}/{size}')
        result[name] = value;offset+=size
    raise ValueError('notify field budget exhausted')


def describe(path):
    data = Path(path).read_bytes()
    pkg = inspect(path,data=data,include_all=True)
    exports = {e['index']:e for e in pkg['exports']}
    names = pkg['names']
    def prop(e,name):
        matches = [p for p in e.get('properties',[]) if p['name']==name]
        if len(matches)!=1 or e.get('parse_error'):
            raise ValueError(f'missing/ambiguous {name}')
        p = matches[0]
        return p['tag_offset']+24,p['tag_offset']+p['tag_bytes']
    def named(e,name):
        p,end = prop(e,name)
        if end-p != 8:
            raise ValueError('invalid named property')
        ni,nn = struct.unpack_from('<ii',data,p)
        if not 0 <= ni < len(names) or nn:
            raise ValueError('invalid property name index')
        return names[ni]
    result = []
    for e in exports.values():
        if e['class_name']!='AnimSequence' or named(e,'SequenceName')!='Reload':
            continue
        owner = exports.get(e['outer'])
        if not owner or owner['name'] != 'Alt_pistol':
            continue
        p,end = prop(e,'Notifies')
        if p+4 > end:
            raise ValueError('truncated notify array')
        count = struct.unpack_from('<i',data,p)[0];p+=4
        if not 0 <= count <= 128:
            raise ValueError('notify count limit')
        notifies = []
        for _ in range(count):
            row,p = fields(data,p,end,names)
            notify = exports.get(row.get('Notify'))
            if not notify or not 0 <= row.get('Time',0) <= 60:
                raise ValueError('invalid notify reference/time')
            item = dict(seconds=row.get('Time',0),class_name=notify['class_name'],
                object_index=notify['index'])
            if notify['class_name']=='AnimNotify_Script':
                item['function'] = named(notify,'NotifyName')
            notifies.append(item)
        if p != end:
            raise ValueError('notify array not fully consumed')
        result.append(dict(sequence='Reload',owner=owner['name'],export=e['index'],notifies=notifies))
    if len(result)!=1:
        raise ValueError('expected one measured Alt_pistol Reload')
    return result[0]


if __name__ == '__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('package',type=Path)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args()
    report=describe(args.package)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
