"""Offline geometry + source identity, not native shader/physical validation."""
import argparse
import json
from pathlib import Path
import struct
import unittest
import numpy as np
from PIL import Image
from prepare_native_hands import prepare, read_psk, unit, quaternion, rotation, boundary_loops
from preview_gloves import panel, ray_distance

PSK=COOKED=OUTPUT=None


def matrix(q):
    x,y,z,w=q
    return np.array([[1-2*(y*y+z*z),2*(x*y+z*w),2*(x*z-y*w)],
                     [2*(x*y-z*w),1-2*(x*x+z*z),2*(y*z+x*w)],
                     [2*(x*z+y*w),2*(y*z-x*w),1-2*(x*x+y*y)]])


class Native(unittest.TestCase):
    def test_quaternion_roundtrip(self):
        for axis in ([1.,0,0],[0,1.,0],[0,0,1.],[1.,2.,3.]):
            for angle in np.linspace(-3,3,20):
                r=rotation(np.array(axis),angle)
                np.testing.assert_allclose(matrix(quaternion(r)),r,atol=1e-10)

    def test_private_source_and_cuff(self):
        if PSK is None:self.skipTest('private offline PSK/cooked not supplied')
        data,r,points,tris=prepare(PSK,COOKED)
        self.assertTrue(r['full_index_roundtrip'])
        self.assertEqual(struct.unpack_from('<4sI',data),(b'SVNI',1))
        self.assertEqual(len(data),32+r['indices']*2+90*16)
        psk=read_psk(PSK);names,parents,local,bind,pnts,weights,wedges,faces=psk
        q=np.frombuffer(data,'<f4',360,32+r['indices']*2).reshape(90,4)
        posed=[]
        for b,l in enumerate(local):
            l=l.copy();l[:3,:3]=l[:3,:3]@matrix(q[b])
            posed.append(l if b==0 else l@posed[parents[b]])
        posed=np.array(posed)
        # Native rigid cuff/open pose, original weighted fingers closed pose.
        skin=np.linalg.inv(bind)@posed
        for b in range(90):
            if not ((10<=b<=25 and b!=13) or 70<=b<=84):skin[b]=np.eye(4)
        refl=np.diag([1.,-1.,1.,1.]);raw_skin=refl@skin@refl
        target=[]
        for point,_,_ in wedges:
            p=np.r_[pnts[point],1.]
            target.append(sum(w*(p@raw_skin[b])[:3] for w,b in weights[point])/sum(w for w,_ in weights[point]))
        target=np.array(target)
        raw_bind=refl@bind@refl
        views=[];rays={};native_count=r['triangles']-r['caps']
        for side,hand in enumerate(r['hands']):
            sign=1 if side==0 else -1
            side_faces=tris[np.all(points[tris,0]*sign>0,axis=1)]
            for mesh in (points,target):self.assertEqual(boundary_loops(mesh,side_faces),[])
            self.assertLess(hand['cuff_rim_uu'][1],-.5)
            wrist=raw_bind[9 if side==0 else 51,3,:3]
            axis=unit(wrist-raw_bind[8 if side==0 else 50,3,:3])
            side_cap=tris[native_count:][np.all(points[tris[native_count:],0]*sign>0,axis=1)]
            centre=points[np.unique(side_cap)].mean(0)
            origin=centre-axis*7
            before=ray_distance(origin,axis,points,tris[:native_count])
            after=ray_distance(origin,axis,points,side_faces)
            cap=ray_distance(origin,axis,points,side_cap)
            self.assertAlmostEqual(after,cap,places=6)
            self.assertGreater(before,after+.01)
            rays[hand['side']]=dict(before_uu=before,after_uu=after,cap_uu=cap,cap_first=True)
            # The original cuff must not deform as fingers close.
            np.testing.assert_allclose(target[np.unique(side_cap)],points[np.unique(side_cap)],atol=1e-5)
            finger_delta=np.linalg.norm(target-points,axis=1)
            self.assertGreater(finger_delta[np.unique(side_faces)].max(),2.)
            axes=np.array([[1.,0,0],[0,1.,0],[0,0,1.]])
            for mesh,label in ((points,'open'),(target,'fist')):
                used,remap=np.unique(side_faces,return_inverse=True)
                centred=(mesh[used]-wrist)/52.5*.75
                views.append(panel(centred,remap.reshape(-1,3),axes,hand['side']+' native topology '+label,extent=.35))
        if OUTPUT:
            OUTPUT.mkdir(parents=True,exist_ok=True)
            atlas=Image.new('RGB',(4*440,440))
            for i,view in enumerate(views):atlas.paste(view,(i*440,0))
            atlas.save(OUTPUT/'native-topology.png')
            (OUTPUT/'axial-rays.json').write_text(json.dumps(rays,indent=2)+'\n')


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--psk',type=Path)
    ap.add_argument('--cooked',type=Path);ap.add_argument('--output',type=Path)
    args,rest=ap.parse_known_args();PSK,COOKED,OUTPUT=args.psk,args.cooked,args.output
    if bool(PSK)!=bool(COOKED):ap.error('supply both private sources, never a Steam path')
    unittest.main(argv=[__file__]+rest)
