import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]))
from swf_ui_polish import (action,action_records,brand_actions,dot_shape,
                          insert_in_body,patch_cursor,patch_main_brand,push)
from swf_title_patch import encode_tag_header,split_tags


def movie(tags):
    # RECT nbits=1, all coordinates zero; fixed fps/frame count.
    b=b'FWS\x08'+b'\0'*4+b'\x08\0'+struct.pack('<HH',30*256,1)
    for code,pay in tags:b+=encode_tag_header(code,len(pay))+pay
    b+=b'\0\0'
    return b[:4]+struct.pack('<I',len(b))+b[8:]


def fake_cursor(extra=b''):
    placement=bytes.fromhex('02000100860606010001000040000000')
    return movie([(2,struct.pack('<H',1)+b'\0'*30),(39,placement),
                  (56,struct.pack('<HH',1,2)+b'Mouse\0'),(12,extra)])


def run_brand(text):
    # Tiny oracle for ONLY our emitted action subset; never executes game bytecode.
    obj={'MajorTitle':{'text':text}};variables={'Title':obj};stack=[]
    data=brand_actions();rows={r['start']:r for r in action_records(data)};pos=0
    while pos<len(data):
        r=rows[pos];pos=r['end'];c=r['code'];p=data[r['payload']:r['metadata_end']]
        if c==0x96:
            offset=0
            while offset<len(p):
                assert p[offset]==0;end=p.index(0,offset+1)
                stack.append(p[offset+1:end].decode('ascii'));offset=end+1
        elif c==0x1c:stack.append(variables[stack.pop()])
        elif c==0x4e:key=stack.pop();stack.append(stack.pop()[key])
        elif c==0x13:right=stack.pop();stack.append(stack.pop()==right)
        elif c==0x12:stack.append(not stack.pop())
        elif c==0x9d:
            if stack.pop():pos+=struct.unpack('<h',p)[0]
        elif c==0x4f:value=stack.pop();key=stack.pop();stack.pop()[key]=value
        else:raise AssertionError(c)
    assert stack==[]
    return obj['MajorTitle']['text']


class Tests(unittest.TestCase):
    def test_native_sprite_untouched(self):
        before=fake_cursor();after,r=patch_cursor(before)
        for code in (39,56,12):
            def payload(b):
                _,tags=split_tags(b);t=next(x for x in tags if x['code']==code)
                return b[t['pay_off']:t['pay_off']+t['pay_len']]
            self.assertEqual(payload(before),payload(after))
        self.assertEqual(r['shape'],1);self.assertGreater(len(after),len(before))
    def test_shared_cursor_rejected(self):
        with self.assertRaises(ValueError):patch_cursor(fake_cursor(bytes.fromhex('060100010000')))
    def test_unknown_sprite_refused(self):
        b=fake_cursor().replace(bytes.fromhex('020001008606'),bytes.fromhex('020002008606'))
        with self.assertRaises(ValueError):patch_cursor(b)
    def test_shape_character_and_count(self):
        b=dot_shape(91);self.assertEqual(struct.unpack_from('<H',b)[0],91)
        self.assertGreater(len(b),180);self.assertLess(len(b),512)
    def test_brand_only_exact_owned_text(self):
        self.assertEqual(run_brand('SINGULARITY'),'SINGULARITY VR')
        for other in ('PAUSE','OPTIONS','SINGULARITY VR','Singularity Labs','Kill the Singularity',''):
            self.assertEqual(run_brand(other),other)
    def test_function_metadata_body_boundaries(self):
        body=push('test')+action(0x17)+action(0)
        metadata=b'Initialize\0'+struct.pack('<H',0)+b'\0\0\0'+struct.pack('<H',len(body))
        data=action(0x8e,metadata)+body+action(0)
        r=action_records(data)
        self.assertEqual(r[0]['body_end'],len(data)-1)
        self.assertEqual(len(r[0]['body']),3)
        with self.assertRaises(ValueError):action_records(data[:-2])
    def test_branches_relocated(self):
        data=action(0x99,struct.pack('<h',1))+action(0x17)+action(0)
        changed=insert_in_body(data,5,action(0x04)*3)
        self.assertEqual(struct.unpack_from('<h',changed,3)[0],4)
        data=action(0x99,struct.pack('<h',0))+action(0)
        changed=insert_in_body(data,5,action(0x04)*3)
        self.assertEqual(struct.unpack_from('<h',changed,3)[0],0) # branches to seam execute hook
    def test_main_initialize_changes_scoped_function(self):
        body=push('unchanged')+action(0x17)+bytes.fromhex('960300090001524f')+action(0)
        meta=b'Initialize\0'+struct.pack('<H',0)+b'\0\0\0'+struct.pack('<H',len(body))
        data=action(0x8e,meta)+body+action(0)
        before=movie([(12,data)]);after,r=patch_main_brand(before)
        self.assertEqual(r['exact_match'],'SINGULARITY')
        self.assertEqual(after.count(b'SINGULARITY VR'),1)
        _,tags=split_tags(after);t=next(t for t in tags if t['code']==12)
        fn=action_records(after[t['pay_off']:t['pay_off']+t['pay_len']])[0]
        self.assertEqual(fn['body_end']-fn['body_start'],len(body)+len(brand_actions()))


if __name__=='__main__':unittest.main()
