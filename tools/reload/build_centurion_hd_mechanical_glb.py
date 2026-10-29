"""Build original Centurion rigid rig and source-only modular glTF actions.

No Steam/native writes. PSA keys and PSK hierarchy are read from explicit private
sources; the cinematic weapon-root track is excluded for controller ownership.
"""
import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path

import numpy as np

from build_valkyrie_mechanical_glb import Buffer,quaternion
from prepare_centurion import read_psk,extract
from inspect_centurion_animation import read_psa
from prepare_weapon_animation import key_local

UNITS=52.5
PSK_SHA='4e18a25d5241de1b3f18100e3bd9373d464ee16baf3e29d7c60289ae18fe73d4'
PSA_SHA='9a30ece4c009bdbea8672aeea67b832f1ad840e946cb3887cef bf584daa412d3'.replace(' ','')
PARTS=((53,'weapon_body'),(57,'cylinder'),(58,'clip_release'),(56,'hammer'),(55,'trigger'))
RELOAD_SEGMENTS=(('cylinder_unlock_release',0,10),
                 ('cylinder_extraction',10,19),
                 ('fresh_cylinder_acquisition_reference',19,26),
                 ('cylinder_insertion_seating',26,33),
                 ('closing_lock_feedback',33,48))


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def rigid_parts(psk):
    names,_,_,bind,points,weights,wedges,faces=psk
    reflect=np.diag([1.,-1.,1.,1.]);raw_bind=reflect@bind@reflect
    remaps={bone:{} for bone,_ in PARTS};vertices={bone:[] for bone,_ in PARTS};indices={bone:[] for bone,_ in PARTS}
    for face in faces:
        bones=set()
        for wedge in face[:3]:
            point=wedges[wedge][0];rows=weights[point]
            if len(rows)!=1 or abs(rows[0][0]-1)>1e-6:raise ValueError('non-rigid source point')
            bones.add(rows[0][1])
        if len(bones)!=1 or face[3]!=0:raise ValueError('mixed source face/material')
        bone=bones.pop()
        if bone not in vertices:raise ValueError('unassigned source part')
        inv=np.linalg.inv(raw_bind[bone])
        for wedge in face[:3]:
            point,u,v=wedges[wedge];key=(point,u,v)
            if key not in remaps[bone]:
                xyz=(np.r_[points[point],1.]@inv)[:3]/UNITS
                remaps[bone][key]=len(vertices[bone]);vertices[bone].append((*xyz,u,v))
            indices[bone].append(remaps[bone][key])
    if sum(len(indices[b]) for b,_ in PARTS)!=len(faces)*3:raise ValueError('face omission')
    return {b:(np.asarray(vertices[b],dtype=np.float32),np.asarray(indices[b],dtype=np.uint16))
            for b,_ in PARTS}


def action(buf,name,source_keys,psa_names,psk_names,start,end,fps,targets):
    frames=end-start+1
    times=buf.add((np.arange(frames,dtype=np.float32)/fps).astype('<f4'),'times')
    samplers=[];channels=[];motion={}
    for bone_name in targets:
        bone=psk_names.index(bone_name)
        track=psa_names.index(bone_name)
        sample=source_keys[start:end+1,track,:7]
        matrices=[key_local(k) for k in sample]
        positions=np.asarray([m[3,:3]/UNITS for m in matrices],dtype='<f4')
        rotations=np.asarray([quaternion(m) for m in matrices],dtype='<f4')
        for i in range(1,len(rotations)):
            if np.dot(rotations[i-1],rotations[i])<0:rotations[i]*=-1
        pi=buf.add(positions,'translation');qi=buf.add(rotations,'rotation')
        for path,output in (('translation',pi),('rotation',qi)):
            samplers.append({'input':times,'output':output,'interpolation':'LINEAR'})
            channels.append({'sampler':len(samplers)-1,'target':{'node':bone,'path':path}})
        move=float(np.linalg.norm(positions-positions[0],axis=1).max())
        angle=float(np.degrees(2*np.arccos(np.clip(np.abs(rotations@rotations[0]).min(),0,1))))
        motion[bone_name]={'max_translation_m':move,'max_rotation_deg':angle,
                           'source_motion':move>1e-5 or angle>.1}
    return {'name':name,'samplers':samplers,'channels':channels,'extras':
            {'sourceFramesInclusive':[start,end],'fps':fps,'motion':motion,
             'source':'byte-exact original Alt_pistol PSA Reload/Fire keys; no synthetic keys'}}


def build(psk_path,psa_path,output):
    if sha(psk_path)!=PSK_SHA or sha(psa_path)!=PSA_SHA:raise ValueError('source hash mismatch')
    psk=read_psk(psk_path);names,parents,local=psk[:3]
    if len(names)!=79 or len(psk[6])!=5799 or len(psk[7])!=6767 or [names[i] for i,_ in PARTS]!=[
        'cog_alt_pistol','clip','clip_release','hammer','Trigger']:
        raise ValueError('unexpected Centurion topology')
    psa_names,sequences,keys=read_psa(psa_path.read_bytes())
    reload=next(s for s in sequences if s['name']=='Reload');fire=next(s for s in sequences if s['name']=='Fire')
    if (reload['frames'],reload['rate'],fire['frames'],fire['rate'])!=(53,24.,15,24.):
        raise ValueError('unreviewed animation timing')
    reflect=np.diag([1.,-1.,1.,1.]);nodes=[]
    for i,name in enumerate(names):
        raw=reflect@local[i]@reflect
        nodes.append({'name':name,'translation':(raw[3,:3]/UNITS).tolist(),
                      'rotation':quaternion(raw).tolist(),'extras':{'pskBoneIndex':i}})
    for i in range(1,len(names)):nodes[parents[i]].setdefault('children',[]).append(i)
    parts=rigid_parts(psk);buf=Buffer();meshes=[]
    for bone,label in PARTS:
        vertices,indices=parts[bone]
        pos=buf.add(vertices[:,:3].astype('<f4'),'POSITION',34962)
        uv=buf.add(vertices[:,3:5].astype('<f4'),'TEXCOORD_0',34962)
        idx=buf.add(indices.astype('<u2'),'indices',34963)
        meshes.append({'name':label,'primitives':[{'attributes':{'POSITION':pos,'TEXCOORD_0':uv},
                                                 'indices':idx,'mode':4}]})
        node_index=len(nodes);nodes.append({'name':label,'mesh':len(meshes)-1,
            'extras':{'parentSourceBone':names[bone],'detachable':bone==57}})
        nodes[bone].setdefault('children',[]).append(node_index)
    reload_keys=keys[reload['first']:reload['first']+53]
    fire_keys=keys[fire['first']:fire['first']+15]
    animations=[action(buf,'Reload_source_53f',reload_keys,psa_names,names,0,52,24.,('clip','clip_release'))]
    animations += [action(buf,label,reload_keys,psa_names,names,start,end,24.,('clip','clip_release'))
                   for label,start,end in RELOAD_SEGMENTS]
    animations.append(action(buf,'hammer_fire_source_15f',fire_keys,psa_names,names,0,14,24.,('hammer',)))
    doc={'asset':{'version':'2.0','generator':'build_centurion_hd_mechanical_glb.py'},
         'scene':0,'scenes':[{'nodes':[0]}],'nodes':nodes,'meshes':meshes,'animations':animations,
         'buffers':[{'byteLength':len(buf.data)}],'bufferViews':buf.views,'accessors':buf.accessors,
         'extras':{'coordinateSpace':'original PSK raw space; metres at 52.5 UU/m',
                   'rootPolicy':'cog_alt_pistol has no animation channels; controller owns weapon root',
                   'sourcePSKSha256':sha(psk_path),'sourcePSASha256':sha(psa_path)}}
    payload=json.dumps(doc,separators=(',',':')).encode();payload+=b' '*((-len(payload))%4)
    binary=bytes(buf.data)+b'\0'*((-len(buf.data))%4)
    glb=(struct.pack('<4sII',b'glTF',2,12+8+len(payload)+8+len(binary))+
         struct.pack('<I4s',len(payload),b'JSON')+payload+
         struct.pack('<I4s',len(binary),b'BIN\0')+binary)
    output.mkdir(parents=True,exist_ok=True);(output/'source').mkdir(exist_ok=True)
    for source in (psk_path,psa_path):
        target=output/'source'/source.name
        if source.resolve()!=target.resolve():shutil.copy2(source,target)
    (output/'centurion_mechanical_rig.glb').write_bytes(glb)
    _,cylinder=extract(psk)
    report={'status':'OFFLINE_SOURCE_ANIMATION_PREPARED','source_psk_sha256':sha(psk_path),
            'source_psa_sha256':sha(psa_path),'glb_sha256':hashlib.sha256(glb).hexdigest(),
            'bones':len(names),'parts':{label:{'bone':names[bone],'vertices':len(parts[bone][0]),
                'triangles':len(parts[bone][1])//3} for bone,label in PARTS},
            'cylinder_source_geometry':cylinder,
            'actions':[{'name':a['name'],'frames':a['extras']['sourceFramesInclusive'],
                        'fps':a['extras']['fps'],'motion':a['extras']['motion']} for a in animations],
            'unsupported':{'spent_cartridge_animation':'No individual cartridge/case source geometry or Reload track',
                           'trigger_reload_motion':'Trigger is static in original Reload',
                           'hinged_swing_out':'Original clip removes/reinserts; no hinge bone'},
            'socket':{'parent':'cog_alt_pistol','sourceBone':'clip',
                      'transform':'original PSK clip bind; no authored offset'},
            'root':'fixed by excluding source cinematic cog_alt_pistol translation; no controller movement baked'}
    (output/'animation_socket_manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--psk',type=Path,required=True)
    p.add_argument('--psa',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();print(json.dumps(build(a.psk,a.psa,a.output),indent=2))
