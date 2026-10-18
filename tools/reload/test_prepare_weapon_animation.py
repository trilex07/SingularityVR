"""Synthetic bounds/hierarchy oracle tests. No game files or Steam required."""
import copy
import struct
import unittest
from unittest.mock import patch
import numpy as np
import prepare_weapon_animation as a


def cooked():
    # Measured section, INDEX16, three skipped arrays, one skinned chunk.
    data=struct.pack('<ihhiH',1,0,0,0,1)
    data+=struct.pack('<ii3H',2,3,0,1,2)+struct.pack('<4i',0,0,0,1)
    data+=struct.pack('<4i3H3i',0,0,0,3,0,1,2,0,3,2)
    return data,dict(vertices=3,triangles=[1])


def material():
    data=bytearray(160)
    struct.pack_into('<2i',data,28,1,2)
    struct.pack_into('<2i',data,104,1,3)
    exports=[dict(name='mesh',class_name='SkeletalMesh',index=1,offset=0,size=40,property_end=0),
        dict(name='mat',class_name='MaterialInstanceConstant',index=2,offset=64,size=64,
            properties=[dict(name='ReferencedTextures',type='ArrayProperty',tag_offset=80,tag_bytes=32)]),
        dict(name='tex',class_name='Texture2D',index=3,offset=128,size=32)]
    return data,dict(exports=exports),dict(mesh='mesh',material='mat',diffuse='tex',triangles=[1])


def fixture():
    names=['cog_machinegun','clip','child'];bind=np.repeat(np.eye(4)[None],3,axis=0)
    psk=(names,[0,0,1],bind,bind,np.zeros((3,3)),{i:[(1.,i)] for i in range(3)},
        [(0,0.,0.),(1,1.,0.),(2,0.,1.)],[(0,1,2,0,0,0)])
    def chunk(name,size,count,raw=b''):
        return struct.pack('<20siii',name.encode(),20100422,size,count)+raw
    # Deliberately shuffled names; a large root movement must never enter cache.
    bones=b''.join(n.encode().ljust(120,b'\0') for n in ['child','cog_machinegun','clip'])
    info=b'Reload'.ljust(64,b'\0')+b'None'.ljust(64,b'\0')+struct.pack('<4i3f3i',3,0,0,9,0,3,30,0,0,3)
    keys=np.zeros((3,3,8),dtype='<f4');keys[:,:,6:]=1
    keys[:,0,0]=[0,2,4];keys[:,1,0]=[0,10000,20000];keys[:,2,0]=[0,1,2]
    psa=chunk('ANIMHEAD',0,0)+chunk('BONENAMES',120,3,bones)+chunk('ANIMINFO',168,1,info)+chunk('ANIMKEYS',32,9,keys.tobytes())
    spec=dict(a.SPECS['valkyrie'],vertices=3,triangles=[1],frames=3,fps=30)
    layout=dict(chunks=[dict(first=0,vertices=3,palette=[2,0,1])],index_fnv=123)
    return psk,psa,spec,layout


class Tests(unittest.TestCase):
    def test_cooked_exact_index_and_palette(self):
        data,spec=cooked();result=a.cooked_layout(data,spec,0,len(data))
        self.assertEqual(result['chunks'],[dict(first=0,vertices=3,palette=[0,1,2])])
        self.assertEqual(result['index_fnv'],a.fnv(struct.pack('<3H',0,1,2)))
        with self.assertRaises(ValueError):a.cooked_layout(data+data,spec,0,len(data)*2)

    def test_cooked_truncation_ranges_and_bad_indices(self):
        data,spec=cooked()
        for n in range(len(data)):
            with self.assertRaises(ValueError):a.cooked_layout(data,spec,0,n)
        for start,end in [(-1,len(data)),(0,len(data)+1),(2,1)]:
            with self.assertRaises(ValueError):a.cooked_layout(data,spec,start,end)
        bad=bytearray(data);struct.pack_into('<H',bad,22,3)
        with self.assertRaises(ValueError):a.cooked_layout(bad,spec,0,len(bad))

    def test_native_material_reference_chain(self):
        data,pkg,spec=material();mesh,w=a.material_witness(data,pkg,spec)
        self.assertEqual((mesh['index'],w['material'],w['diffuse']),(1,2,3))
        for pos in (28,32,104,108):
            bad=bytearray(data);struct.pack_into('<i',bad,pos,999)
            with self.assertRaises(ValueError):a.material_witness(bad,pkg,spec)

    def test_material_ambiguous_and_bounded(self):
        data,pkg,spec=material()
        for mutate in ('duplicate','meshshort','matshort','tagshort'):
            bad=copy.deepcopy(pkg)
            if mutate=='duplicate':bad['exports'].append(dict(bad['exports'][0]))
            if mutate=='meshshort':bad['exports'][0]['size']=35
            if mutate=='matshort':bad['exports'][1]['size']=24
            if mutate=='tagshort':bad['exports'][1]['properties'][0]['tag_bytes']=25
            with self.assertRaises(ValueError):a.material_witness(data,bad,spec)

    def generate(self,psk=None,psa=None,layout=None):
        p,k,s,l=fixture()
        with patch.dict(a.SPECS,{'valkyrie':s}),patch.object(a,'material_witness',return_value=(dict(property_end=0,offset=0,size=1),{})),patch.object(a,'cooked_layout',return_value=l if layout is None else layout):
            return a.prepare('valkyrie',p if psk is None else psk,k if psa is None else psa,b'x',{})

    def test_original_name_mapping_hierarchy_and_frozen_root(self):
        cache,report,animation=self.generate()
        self.assertEqual([r['psa_track'] for r in report['tracks']],[2,0])
        self.assertEqual([r['slot'] for r in report['tracks']],[2,0])
        self.assertEqual([r['parent'] for r in report['tracks']],[-1,0])
        self.assertEqual(cache,self.generate()[0])
        deltas=a.deltas_at(animation,.5,0)
        self.assertAlmostEqual(deltas[0][3,0],.5)
        self.assertAlmostEqual(deltas[1][3,0],1.5)
        oracle=a.oracle('valkyrie',animation,0)
        self.assertEqual(struct.unpack_from('<4s4I',oracle),(b'SVWO',1,1,2,9))
        self.assertEqual(len(oracle),20+9*(4+2*64))
        self.assertEqual(oracle,a.oracle('valkyrie',animation,0))

    def test_wrong_topology_palette_hierarchy_and_psa_fail_closed(self):
        psk,psa,_,layout=fixture()
        bad=list(psk);bad[1]=[0,2,0]
        with self.assertRaises(ValueError):self.generate(psk=bad)
        bad=list(psk);bad[1]=[0,1,1]
        with self.assertRaises(ValueError):self.generate(psk=bad)
        bad=list(psk);bad[0]=[psk[0][0],'clip','clip']
        with self.assertRaises(ValueError):self.generate(psk=bad)
        bad=copy.deepcopy(layout);bad['chunks'][0]['palette']=[0,1]
        with self.assertRaises(ValueError):self.generate(layout=bad)
        with self.assertRaises(ValueError):self.generate(psa=psa[:-1])

    def test_quaternion_sign_and_rigid_rotation(self):
        _,_,animation=self.generate();tracks=animation[-1]
        q=np.array([0,0,np.sin(.4),np.cos(.4)])
        tracks[0,0,3:7]=q;tracks[1,0,3:7]=-q
        delta=a.deltas_at(animation,.5,0)[0]
        expected=a.key_local([.5,0,0,*q])
        np.testing.assert_allclose(delta,expected,atol=1e-7)
        for bad in (-1,3,float('nan')):
            with self.assertRaises(ValueError):a.deltas_at(animation,bad,0)


if __name__=='__main__':unittest.main()
