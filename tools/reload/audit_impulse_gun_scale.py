"""Verify gun_scale producer using private original inputs, no process/Steam access.

Prints measurements/hashes, not extracted game content. Requires capstone/numpy.
Run with --exe <archived exe> --text <archived decrypted text> --package <decompressed RvGame>
and --volk <original ch_inview_shotgun_mesh.psk>.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import capstone
import numpy as np
from inspect_native_reload import PE
from prepare_centurion import read_psk
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect


def audit(exe, text, package, volk):
    pe=PE(exe.read_bytes());t=text.read_bytes();b=package.read_bytes();base=0x402000
    r=inspect(package,data=b,include_all=True);e={x['index']:x for x in r['exports']}
    names=r['names']
    def code(va,n):return t[va-base:va-base+n]
    def instructions(va,n):
        md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
        return [(i.address,i.mnemonic,i.op_str) for i in md.disasm(code(va,n),va)]
    assert pe.base==0x400000
    # Independently read virtual dispatch and the native instance constructor.
    assert pe.read(0x189f0a8+0x140,4)==struct.pack('<I',0xb3e400)
    assert code(0xb43ab2,6)==b'\xc7\x06'+struct.pack('<I',0x189f0a8)
    assert instructions(0xb3e400,6)==[(0xb3e400,'fld','dword ptr [ecx + 0x7c]'),(0xb3e403,'ret','8')]
    expected=[('mov','ecx, dword ptr [ebp - 0x38]'),('mov','edx, dword ptr [ecx]'),
              ('push','edi'),('push','eax'),('mov','eax, dword ptr [edx + 0x140]'),
              ('call','eax'),('fld1',''),('fsub','st(1), st(0)'),('fxch','st(1)'),
              ('fmul','dword ptr [ebp - 0x18]'),('fadd','st(1)')]
    assert [(m,o) for _,m,o in instructions(0xaec25d,26)][:len(expected)]==expected
    # Body, slide, clip all share the cog_gun ancestor, not only palette slot 0.
    n,parents,local,bind,points,weights,wedges,faces=read_psk(volk)
    root=n.index('cog_gun');bones=[55,56,53,54,57]
    assert [n[x] for x in bones]==['clip','Slide','cog_shotgun','release','Trigger']
    for bone in bones:
        j=bone
        while j!=root:
            assert j>0 and parents[j]!=j
            j=parents[j]
    # Authored control data: attached to cog_gun, target=0 scale with instant blend.
    assert e[34896]['class_name']=='SkelControlSingleBone' and e[34896]['outer']==17332
    props={p['name']:p for p in e[34896]['properties']}
    def payload(p):return b[p['tag_offset']+24:p['tag_offset']+p['tag_bytes']]
    assert names[struct.unpack_from('<i',payload(props['ControlName']))[0]]=='gun_scale'
    for prop in ('BoneScale','ControlStrength','StrengthTarget','BlendInTime','BlendOutTime'):
        assert struct.unpack('<f',payload(props[prop]))[0]==0
    rows=payload(next(p for p in e[17332]['properties'] if p['name']=='SkelControlLists'))
    links=[]
    for j in range(struct.unpack_from('<i',rows)[0]):
        row=rows[4+j*96:4+(j+1)*96]
        if struct.unpack_from('<i',row,56)[0]==34896:
            links.append(names[struct.unpack_from('<i',row,24)[0]])
    assert links==['cog_gun']
    # PostBeginPlay assignment links reflected field -> mInviewMesh.FindSkelControl.
    raw=b[e[12220]['offset']:e[12220]['offset']+e[12220]['size']]
    assert e[12061]['name']=='mGunScaleCtrl' and e[12030]['name']=='mInviewMesh'
    at=raw.index(b'\x0f\x01'+struct.pack('<i',12061))
    term=raw.index(b'\x21'+struct.pack('<ii',names.index('gun_scale'),0),at)
    assert term-at<40 and b'\x19\x01'+struct.pack('<i',12030) in raw[at:term]
    # Native SwitchToTMD activates first control at pawn+1024, Weapon deactivates it.
    for start,active in [(0xfd18e0,1),(0xfd1a85,0)]:
        rows2=instructions(start,7)
        assert rows2[0][1:]==('push',str(active)) and rows2[1][1:]==('call','0xac7960')
    assert instructions(0xfd18c4,6)[0][1:]==('mov','ecx, dword ptr [esi + 0x1024]')
    assert instructions(0xfd1a69,6)[0][1:]==('mov','ecx, dword ptr [esi + 0x1024]')
    # Measured bind palette + animated rigid descendants: native scale 0 collapses
    # the complete subtree; result 1 retains the authored, per-frame skinning.
    errors=[];collapsed=[];point_errors=[];collapsed_points=[]
    owned={p for p,row in weights.items() if sum(w for w,bone in row if bone in bones)>1-1e-6}
    visible_faces=[f for f in faces if all(wedges[w][0] in owned for w in f[:3])]
    assert len(visible_faces)==10835 and len(wedges)==8297
    # read_psk returns ROW matrices and reflects UEViewer Y. Respect both;
    # inverseBind * animatedBone is the actual row-vector skin transform.
    points_h=np.c_[points*np.array([1.,-1.,1.]),np.ones(len(points))]
    for angle in (0.,0.6,-1.1):
        c,s=np.cos(angle),np.sin(angle);anim=np.eye(4)
        anim[:3,:3]=[[c,s,0],[-s,c,0],[0,0,1]];anim[3,:3]=[2,4,-3]
        origin=bind[root][3,:3]
        for scale in (0.,1.):
            collapse=np.eye(4);collapse[:3,:3]*=scale;collapse[3,:3]=origin*(1-scale)
            for bone in bones:
                motion=np.eye(4);motion[3,0]=angle*(bone-52)*0.1  # distinct current child animation
                animated=motion@bind[bone]@anim
                skin=np.linalg.inv(bind[bone])@motion@bind[bone]@collapse@anim
                reference=np.linalg.inv(bind[bone])@animated
                pts=[p for p,row in weights.items() if len(row)==1 and row[0][1]==bone]
                final=points_h[pts]@skin
                if scale==0:
                    collapsed.append(abs(np.linalg.det(skin[:3,:3])))
                    collapsed_points.append(float(np.max(abs(final-(np.r_[origin,1.]@anim)))))
                else:
                    errors.append(float(np.max(abs(skin-reference))))
                    point_errors.append(float(np.max(abs(final-points_h[pts]@reference))))
    assert max(collapsed)<1e-10 and max(errors)<1e-10 and max(point_errors)<1e-10 and max(collapsed_points)<1e-10
    return dict(status='PASS offline producer/consumer/asset contract; NOT PSVR2',
                source_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (exe,text,package,volk)},
                gun_ancestor=dict(id=root,name=n[root]),palette_bones=[dict(id=i,name=n[i]) for i in bones],
                original_scale=0,corrected_getter_scale=1,poses=3,max_collapsed_det=max(collapsed),
                max_corrected_matrix_error=max(errors),max_skinned_vertex_error=max(point_errors),
                max_zero_scale_vertex_separation=max(collapsed_points),triangles=len(visible_faces),
                matrix_convention='row vectors, reflected UEViewer Y, inverseBind * animatedBone',component='mInviewMesh',
                getter='0xB3E400 virtual +0x140',caller_return='0xAEC26C',abi='ECX=this; stack bone, component; ret8; ST(0) float')


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','text','package','volk'):ap.add_argument('--'+name,type=Path,required=True)
    a=ap.parse_args();print(json.dumps(audit(a.exe,a.text,a.package,a.volk),indent=2))
