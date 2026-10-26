import io,json,struct,sys,unittest
from pathlib import Path
import numpy as np
from PIL import Image
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from audit_cooked_texture import mips,properties
from sync_hires_targets import extend
from prepare_hires_first_person import upscale_diffuse

def external(w,h,offset=128):
    n=((w+3)//4)*((h+3)//4)*16
    return struct.pack('<IIiiII',0x11,n,100,offset,w,h)

class CookedRecovery(unittest.TestCase):
    def test_first_present_is_mip1_not_stripped_mip0(self):
        data=bytes(12)+struct.pack('<II',123,2)+struct.pack('<IIiiII',0x21,0,-1,-1,2048,1024)+external(1024,512)
        r=mips(data,0,len(data),'PF_DXT5')
        self.assertEqual(r[0]['flags'],0x21)
        self.assertEqual(r[0]['raw_size'],0)
        first=next(x for x in r if x['flags']!=0x21)
        self.assertEqual((first['index'],first['width'],first['height']),(1,1024,512))
    def test_real_layout_not_nominal_dimensions_is_checked(self):
        data=bytes(12)+struct.pack('<II',123,1)+external(1024,512)
        broken=bytearray(data);struct.pack_into('<I',broken,24,2048*1024)
        with self.assertRaisesRegex(ValueError,'byte layout'):mips(broken,0,len(broken),'PF_DXT5')
        with self.assertRaises(ValueError):mips(data[:-1],0,len(data)-1,'PF_DXT5')
    def test_invalid_flags_or_unused_descriptor_refused(self):
        for flags,n,size,at in ((0x31,0,-1,-1),(0x21,2,-1,-1),(0x11,16,20,-1)):
            data=bytes(12)+struct.pack('<II',123,1)+struct.pack('<IIiiII',flags,n,size,at,4,4)
            with self.assertRaises(ValueError):mips(data,0,len(data),'PF_DXT5')
    def test_property_stream_contract_and_bounds(self):
        names=['SizeX','IntProperty','None']
        data=struct.pack('<iiiiiii',0,0,1,0,4,0,1024)+struct.pack('<ii',2,0)
        p,q=properties(data,names,0,len(data));self.assertEqual(p['SizeX'],1024);self.assertEqual(q,len(data))
        with self.assertRaises(ValueError):properties(data,names,0,len(data)-1)
    def test_registry_idempotence_and_collision_refusal(self):
        text='static const Target kTargets[] = {\n    { "sample", 4u, 4u, D3DFMT_DXT5, 7ULL, "sample.dds", "weapons" },\n};'
        spec={'status':'staged','source_layout':[4,4],'source_fourcc':'DXT5','source_fnv64':7}
        self.assertEqual(extend(text,{'maps':{'sample':spec}}),text)
        with self.assertRaisesRegex(ValueError,'Ambiguous'):extend(text,{'maps':{'new':spec}})
        added=extend(text,{'maps':{'new':{**spec,'source_fnv64':8}}})
        self.assertEqual(added.count('"new",'),1)
        self.assertEqual(extend(added,{'maps':{'new':{**spec,'source_fnv64':8}}}),added)
    def test_four_times_diffuse_keeps_independent_alpha_and_rgb(self):
        a=np.zeros((8,8,4),dtype=np.uint8);a[:,:,:3]=[235,75,15];a[:4,:,3]=255
        r=np.asarray(upscale_diffuse(Image.fromarray(a),4))
        self.assertEqual(r.shape,(32,32,4))
        self.assertGreater(int(r[-1,0,0]),225) # invisible RGB must not become black
        self.assertLess(int(r[-1,0,3]),10)
        self.assertGreater(int(r[0,0,3]),245)
        with self.assertRaises(ValueError):upscale_diffuse(Image.fromarray(a),3)

if __name__=='__main__':unittest.main()
