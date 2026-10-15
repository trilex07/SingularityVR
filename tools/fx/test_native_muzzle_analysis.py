import unittest
from unittest.mock import patch
import analyze_native_muzzle as module
import inspect_native_muzzle_assets as assets
import struct

def fixture(count=1,complete=True):
    vs=dict(fnv1a64='000000000000000A',bytes=128,sha256='a',parameter_registers={})
    ps=dict(fnv1a64='000000000000000B',bytes=256,sha256='b',parameter_registers={})
    return dict(complete_budget=complete,shaders={'v':vs,'p':ps},groups={0:dict(VS='v',PS='p',draws=str(count))})

class Analysis(unittest.TestCase):
    def test_increase_not_identification(self):
        with patch.object(module.trace,'read_trace',side_effect=[fixture(),fixture(3),fixture(2)]):
            report=module.compare('base',['s1','s2'])
        self.assertEqual(report['status'],'CANDIDATES_NOT_IDENTIFIED')
        self.assertEqual(len(report['candidates']),1)
        self.assertEqual(report['candidates'][0]['baseline_count'],1)
        ini=module.drop_snippet(report,'000000000000000a:000000000000000b')
        self.assertIn('NativeMuzzleStereoMode=1',ini)
        self.assertIn('NativeMuzzleVSBytes=128',ini)
        with self.assertRaises(ValueError):module.drop_snippet(report,'000000000000000C:000000000000000B')
    def test_overflow_refusal(self):
        with patch.object(module.trace,'read_trace',side_effect=[fixture(),fixture(3,False)]):
            with self.assertRaises(ValueError):module.compare('b',['s'])
    def test_no_increase_no_candidate(self):
        with patch.object(module.trace,'read_trace',side_effect=[fixture(3),fixture(2)]):
            self.assertEqual(module.compare('b',['s'])['candidates'],[])
    def test_hash_contract(self):
        self.assertEqual(module.trace.fnv1a(b'hello'),'A430D84680AABD0B')
    def test_shader_budget_refusal(self):
        bad=fixture(3);bad['shader_budget_overflow']=1
        with patch.object(module.trace,'read_trace',side_effect=[fixture(),bad]):
            with self.assertRaises(ValueError):module.compare('b',['s'])
    def test_native_light_array(self):
        prop=dict(type='ArrayProperty',tag_offset=0,value=dict(bytes=8))
        exports=[dict(name='originalLight',class_name='ExplosionLight',outer=2)]
        data=bytes(24)+struct.pack('<Ii',1,1)
        self.assertEqual(assets.array_refs(data,prop,exports)[0]['name'],'originalLight')
        for count,ref in ((2,1),(1,-1),(1,2)):
            with self.assertRaises(ValueError):assets.array_refs(bytes(24)+struct.pack('<Ii',count,ref),prop,exports)

if __name__=='__main__':unittest.main()
