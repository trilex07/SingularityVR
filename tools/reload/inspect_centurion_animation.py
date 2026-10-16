"""Offline PSA measurement; no engine writes, animation replacement or Steam.

Narrow UEViewer PSA layout, checked against its primary format/export source:
https://github.com/gildor2/UEViewer/blob/master/Exporters/Psk.h
https://github.com/gildor2/UEViewer/blob/master/Exporters/ExportPsk.cpp
PSA bone parents are placeholders: map NAMES to the measured PSK hierarchy.
Private keys remain outside Git. The report is geometry evidence, not an ABI,
animation-notify, inventory-commit or controller-mount proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from prepare_centurion import read_psk


def text(raw):
    if b'\0' not in raw:
        raise ValueError('unterminated name')
    return raw.split(b'\0', 1)[0].decode('ascii')


def read_psa(data):
    if not 128 <= len(data) <= 64*1024*1024:
        raise ValueError('PSA size limit')
    chunks, offset = {}, 0
    while offset < len(data):
        if offset+32 > len(data):
            raise ValueError('truncated chunk header')
        ident, version, size, count = struct.unpack_from('<20siii', data, offset)
        name = text(ident)
        end = offset+32+size*count
        if name in chunks or size < 0 or count < 0 or end > len(data):
            raise ValueError('duplicate or invalid chunk range')
        chunks[name] = (version, size, count, data[offset+32:end])
        offset = end
    if list(chunks) not in (['ANIMHEAD','BONENAMES','ANIMINFO','ANIMKEYS'],
                           ['ANIMHEAD','BONENAMES','ANIMINFO','ANIMKEYS','SCALEKEYS']):
        raise ValueError('unsupported PSA chunks/order')
    if chunks['ANIMHEAD'][1:3] != (0, 0):
        raise ValueError('invalid animation header')
    if 'SCALEKEYS' in chunks and chunks['SCALEKEYS'][1:3] != (16, 0):
        raise ValueError('scaled animation requires separate review')
    _, bone_size, bone_count, raw = chunks['BONENAMES']
    if bone_size != 120 or not 1 <= bone_count <= 4096:
        raise ValueError('invalid bone records')
    names = [text(raw[i*120:i*120+64]) for i in range(bone_count)]
    if len(set(names)) != len(names) or not all(names):
        raise ValueError('duplicate/empty bone names')
    _, size, count, raw = chunks['ANIMINFO']
    if size != 168 or not 1 <= count <= 4096:
        raise ValueError('invalid sequence records')
    sequences, total_frames = [], 0
    for i in range(count):
        row = raw[i*168:(i+1)*168]
        bones, root, compression, quota, reduction, time, rate, start, first, frames = struct.unpack_from('<4i3f3i', row, 128)
        if bones != bone_count or root or compression or start or first != total_frames:
            raise ValueError('non-contiguous/unsupported sequence')
        if not 1 <= frames <= 100000 or quota != frames*bones:
            raise ValueError('invalid sequence frame count')
        if not np.isfinite([reduction,time,rate]).all() or not 0 < rate <= 1000 or time != frames:
            raise ValueError('invalid sequence rate/time')
        sequences.append(dict(name=text(row[:64]),first=first,frames=frames,rate=rate))
        total_frames += frames
    if len({s['name'] for s in sequences}) != count:
        raise ValueError('ambiguous sequence names')
    _, size, count, raw = chunks['ANIMKEYS']
    if size != 32 or not 1 <= count <= 1000000 or count != total_frames*bone_count:
        raise ValueError('invalid animation key count')
    keys = np.frombuffer(raw, '<f4').reshape(total_frames,bone_count,8)
    if not np.isfinite(keys).all() or np.any(keys[:,:,7] != 1):
        raise ValueError('invalid key values/time')
    lengths = np.linalg.norm(keys[:,:,3:7].astype(float),axis=2)
    if np.any(np.abs(lengths-1) > .005):
        raise ValueError('non-unit animation rotation')
    # Re-encode every original record, including FLOAT32 keys; byte equality is
    # required before using a measurement. This is not a package patcher.
    rebuilt = b''
    for name, (version,size,count,raw) in chunks.items():
        if name == 'ANIMKEYS':
            raw = keys.astype('<f4').tobytes()
        rebuilt += struct.pack('<20siii',name.encode(),version,size,count)+raw
    if rebuilt != data:
        raise ValueError('PSA exact roundtrip failed')
    return names, sequences, keys


def describe(data, mesh_names, mesh_parents):
    names, sequences, keys = read_psa(data)
    if len(mesh_names) != len(mesh_parents) or len(set(mesh_names)) != len(mesh_names):
        raise ValueError('ambiguous PSK hierarchy')
    if any(name not in mesh_names for name in names) or 'clip' not in names:
        raise ValueError('animation/mesh bone-name mismatch')
    mesh_clip = mesh_names.index('clip')
    parent = mesh_parents[mesh_clip]
    if not 0 <= parent < mesh_clip:
        raise ValueError('invalid clip parent')
    clip = names.index('clip')
    result = []
    for s in sequences:
        track = keys[s['first']:s['first']+s['frames'],clip].astype(float)
        travel = np.linalg.norm(track[:,:3]-track[0,:3],axis=1)
        quat = track[:,3:7]
        quat /= np.linalg.norm(quat,axis=1)[:,None]
        angle = np.degrees(2*np.arccos(np.clip(np.abs(quat @ quat[0]),0,1)))
        result.append(dict(name=s['name'],frames=s['frames'],rate=s['rate'],
            duration_seconds=s['frames']/s['rate'],
            max_clip_local_travel_uu=float(travel.max()),
            peak_travel_frame=int(travel.argmax()),
            max_clip_local_rotation_degrees=float(angle.max()),
            end_clip_local_travel_uu=float(travel[-1]),
            end_clip_local_rotation_degrees=float(angle[-1])))
    return dict(sha256=hashlib.sha256(data).hexdigest(),
        animation_tracks=len(names),mesh_bones=len(mesh_names),
        clip_psa_track=clip,clip_psk_bone=mesh_clip,clip_parent=mesh_names[parent],
        frames=len(keys),keys=int(keys.shape[0]*keys.shape[1]),
        roundtrip='all records and FLOAT32 keys re-encoded byte-exact',
        axes='UEViewer reflected PSA, clip-local relative to PSK parent; NOT controller axes',
        limits='geometry only: no animation notify/inventory commit/engine ABI or live draw proof',
        sequences=result)


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--psa',type=Path,required=True)
    ap.add_argument('--psk',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    args = ap.parse_args()
    psk = read_psk(args.psk)
    report = describe(args.psa.read_bytes(),psk[0],psk[1])
    report['mesh_sha256'] = hashlib.sha256(args.psk.read_bytes()).hexdigest()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
