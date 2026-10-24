import sys, tempfile, unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import stage_first_hires_gate as s
import hashlib
TEMP_ROOT=Path(__file__).resolve().parents[3]/'build'/'stage-test-temp'
TEMP_ROOT.mkdir(parents=True,exist_ok=True)

class Stage(unittest.TestCase):
    def test_four_only_and_no_overwrite(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            root=Path(tmp); src=root/'material-remaster-hires/ready'; src.mkdir(parents=True)
            receipts={}
            for name in (*s.FIRST,'AR_E99Rifle_n'):
                data=name.encode(); (src/(name+'.dds')).write_bytes(data)
                receipts[name]={'sha256':hashlib.sha256(data).hexdigest()}
            with patch.object(s,'verify',return_value=receipts):
                out=s.stage(root,'candidate',{},'')
                self.assertEqual({p.stem for p in out.glob('*.dds')},set(s.FIRST))
                with self.assertRaises(FileExistsError): s.stage(root,'candidate',{},'')
                with self.assertRaises(ValueError): s.stage(root,'../escape',{},'')
    def test_failed_validation_writes_nothing(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            root=Path(tmp)
            with patch.object(s,'verify',side_effect=ValueError('bad hash')):
                with self.assertRaises(ValueError): s.stage(root,'candidate',{},'')
            self.assertFalse((root/'candidate').exists())

if __name__=='__main__': unittest.main()
