import struct
import unittest
from inspect_reload_notifies import fields

NAMES=['None','Time','FloatProperty','Notify','ObjectProperty','NotifyDirection',
       'ByteProperty','AN_Forward','NotifyName','NameProperty','AnimNotify_RefillClip']


def tag(name,kind,raw):
    return struct.pack('<6i',NAMES.index(name),0,NAMES.index(kind),0,len(raw),0)+raw


class Tests(unittest.TestCase):
    def test_float_reference_enum_and_exact_end(self):
        data=(tag('Time','FloatProperty',struct.pack('<f',1.5))+
              tag('Notify','ObjectProperty',struct.pack('<i',2))+
              tag('NotifyDirection','ByteProperty',struct.pack('<ii',7,0))+
              struct.pack('<ii',0,0))
        row,end=fields(data,0,len(data),NAMES)
        self.assertEqual(end,len(data));self.assertEqual(row,
            dict(Time=1.5,Notify=2,NotifyDirection='AN_Forward'))

    def test_script_notify_name(self):
        data=tag('NotifyName','NameProperty',struct.pack('<ii',10,0))+bytes(8)
        row,_=fields(data,0,len(data),NAMES)
        self.assertEqual(row['NotifyName'],'AnimNotify_RefillClip')

    def test_truncation_duplicate_and_nonfinite(self):
        good=tag('Time','FloatProperty',struct.pack('<f',1))
        for data in (good,good[:-1],good+good+bytes(8),
                     tag('Time','FloatProperty',struct.pack('<f',float('nan')))+bytes(8)):
            with self.assertRaises(ValueError):fields(data,0,len(data),NAMES)
        with self.assertRaises(ValueError):fields(bytes(8),0,9,NAMES)

    def test_negative_size_and_unknown_name(self):
        for values in ((1,0,2,0,-1,0),(999,0,2,0,4,0)):
            data=struct.pack('<6i',*values)+bytes(8)
            with self.assertRaises(ValueError):fields(data,0,len(data),NAMES)


if __name__=='__main__':unittest.main()
