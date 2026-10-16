import copy
import struct
import unittest
import numpy as np
from prepare_centurion_shells import bake, particle_chain


def chunk(name, size, rows):
    return struct.pack('<20siii', name.encode(), 0, size, len(rows))+b''.join(rows)


def fixture():
    return (chunk('ACTRHEAD', 0, [])+
            chunk('PNTS0000', 12, [struct.pack('<3f', *p) for p in [(1,2,3),(2,2,3),(1,3,3)]])+
            chunk('VTXW0000', 16, [struct.pack('<IffBBH', i, float(i==1), float(i==2), 0, 0, 0) for i in range(3)])+
            chunk('FACE0000', 12, [struct.pack('<3HBBI', 0,1,2,0,0,0)])+
            chunk('REFSKELT', 120, [])+chunk('RAWWEIGHTS', 12, []))


def chain_fixture():
    # Independent synthetic reference graph; names alone must not pass.
    data = bytearray(); exports = []
    def obj(i, name, kind, fields):
        props = []
        for key, refs in fields.items():
            array = isinstance(refs, list)
            raw = (struct.pack('<i', len(refs))+struct.pack('<'+'i'*len(refs), *refs)) if array else struct.pack('<i', refs)
            at = len(data); data.extend(b'\0'*24+raw)
            props.append(dict(name=key, type='ArrayProperty' if array else 'ObjectProperty', tag_offset=at, tag_bytes=24+len(raw)))
        exports.append(dict(index=i, name=name, class_name=kind, properties=props))
    obj(1, 'FX_handgunShellsEject', 'ParticleSystem', {'Emitters':[2]})
    obj(2, 'emitter', 'ParticleSpriteEmitter', {'LODLevels':[3,4]})
    for i in (3,4):obj(i, 'lod', 'ParticleLODLevel', {'TypeDataModule':5,'RequiredModule':6})
    obj(5, 'mesh', 'ParticleModuleTypeDataMesh', {'Mesh':7})
    obj(6, 'required', 'ParticleModuleRequired', {'Material':8})
    obj(7, 'shell_rifle', 'StaticMesh', {})
    obj(8, 'shells', 'MaterialInstanceConstant', {'ReferencedTextures':[9]})
    obj(9, 'shell_rifle_d', 'Texture2D', {})
    return dict(exports=exports), data


class Tests(unittest.TestCase):
    def test_original_uv_roundtrip_centre_and_determinism(self):
        data, report = bake(fixture())
        self.assertEqual((data,report), bake(fixture()))
        self.assertEqual(struct.unpack_from('<4sIII', data), (b'SVRK',2,3,3))
        rows = [struct.unpack_from('<3fI2f', data,16+24*i) for i in range(3)]
        np.testing.assert_allclose(np.array([r[:3] for r in rows])*52.5+report['original_centre_uu'], [(1,2,3),(2,2,3),(1,3,3)], rtol=0,atol=2e-6)
        self.assertEqual(rows[1][3:], (0xffffffff,1.,0.))
    def test_truncation_duplicate_and_invalid_units(self):
        for b in (fixture()[:-1],fixture()+fixture()):
            with self.assertRaises(ValueError):bake(b)
        for u in (0,float('nan'),1001):
            with self.assertRaises(ValueError):bake(fixture(),u)
    def test_invalid_point_face_material_and_skin(self):
        original = fixture()
        point = original.index(b'PNTS0000')+32
        wedge = original.index(b'VTXW0000')+32
        face = original.index(b'FACE0000')+32
        for offset, fmt, value in ((point,'<f',float('nan')),(wedge,'<I',999),(face,'<H',999),(face+6,'<B',1)):
            b=bytearray(original);struct.pack_into(fmt,b,offset,value)
            with self.assertRaises(ValueError):bake(b)
        b=original.replace(chunk('RAWWEIGHTS',12,[]),chunk('RAWWEIGHTS',12,[struct.pack('<fii',1.,0,0)]))
        with self.assertRaises(ValueError):bake(b)
    def test_graph_requires_actual_mesh_material_texture_references(self):
        pkg, data = chain_fixture();r=particle_chain(pkg,data)
        self.assertEqual((r['mesh'],r['material'],r['diffuse']), (7,8,9))
        for index, key in ((4,'Mesh'),(5,'Material'),(7,'ReferencedTextures')):
            altered = bytearray(data)
            p=[p for p in pkg['exports'][index]['properties'] if p['name']==key][0]
            struct.pack_into('<i',altered,p['tag_offset']+24+(4 if key=='ReferencedTextures' else 0),1)
            with self.assertRaises(ValueError):particle_chain(pkg,altered)
    def test_graph_ambiguous_names_wrong_type_or_missing_reference_refused(self):
        pkg,data=chain_fixture()
        altered=copy.deepcopy(pkg);altered['exports'].append(dict(altered['exports'][-1],index=10))
        with self.assertRaises(ValueError):particle_chain(altered,data)
        altered=copy.deepcopy(pkg);altered['exports'][4]['class_name']='ParticleModuleTypeDataSprite'
        with self.assertRaises(ValueError):particle_chain(altered,data)
        altered=copy.deepcopy(pkg);altered['exports'][5]['properties']=[]
        with self.assertRaises(ValueError):particle_chain(altered,data)
    def test_graph_short_or_unbounded_array_refused(self):
        pkg,data=chain_fixture()
        for size in (-1,0,129):
            altered=bytearray(data);struct.pack_into('<i',altered,24,size)
            with self.assertRaises(ValueError):particle_chain(pkg,altered)
        altered=copy.deepcopy(pkg);altered['exports'][0]['properties'][0]['tag_bytes']=25
        with self.assertRaises(ValueError):particle_chain(altered,data)


if __name__ == '__main__':unittest.main()
