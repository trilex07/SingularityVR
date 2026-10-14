"""Synthetic parser guards; no copyrighted asset fixture."""
import struct
import unittest
from inspect_scripted_speech import Graph


class TestTaggedSpeech(unittest.TestCase):
    def make(self, payload):
        graph=Graph.__new__(Graph)
        graph.names=['None','Time','Volume','Pitch','Sound','FloatProperty',
                     'ObjectProperty','FaceFXSoundCue']
        graph.data=b'\0'*24+payload
        graph.ref=lambda i:dict(index=i,path='synthetic.'+str(i),class_name='SoundCue')
        return graph,dict(tag_offset=0,value=dict(bytes=len(payload)))

    def field(self, name, typ, value):
        return struct.pack('<6i',name,0,typ,0,4,0)+value

    def test_sound_typed_reference(self):
        payload=struct.pack('<i',1)
        for n in (1,2,3):
            payload+=self.field(n,5,struct.pack('<f',1))
        payload+=self.field(4,6,struct.pack('<i',42))+struct.pack('<2i',0,0)
        graph,prop=self.make(payload)
        self.assertEqual(graph.sound_entries(prop)[0]['Sound']['index'],42)

    def test_sound_unrecognized_field_refused(self):
        payload=struct.pack('<i',1)+self.field(7,6,struct.pack('<i',42))+struct.pack('<2i',0,0)
        graph,prop=self.make(payload)
        with self.assertRaises(ValueError):graph.sound_entries(prop)

    def test_facefx_typed_reference(self):
        payload=struct.pack('<i',1)+self.field(7,6,struct.pack('<i',-42))+struct.pack('<2i',0,0)
        graph,prop=self.make(payload)
        self.assertEqual(graph.facefx_entries(prop)[0]['Sound']['index'],-42)

    def test_facefx_wrong_type_refused(self):
        payload=struct.pack('<i',1)+self.field(7,5,struct.pack('<i',42))+struct.pack('<2i',0,0)
        graph,prop=self.make(payload)
        with self.assertRaises(ValueError):graph.facefx_entries(prop)

    def test_facefx_array_tail_refused(self):
        payload=struct.pack('<i',1)+self.field(7,6,struct.pack('<i',42))+struct.pack('<2i',0,0)+b'x'
        graph,prop=self.make(payload)
        with self.assertRaises(ValueError):graph.facefx_entries(prop)

    def test_excessive_count_refused(self):
        graph,prop=self.make(struct.pack('<i',1025))
        with self.assertRaises(ValueError):graph.facefx_entries(prop)


if __name__=='__main__':unittest.main()
