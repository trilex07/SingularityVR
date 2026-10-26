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
    def test_valkyrie_pair_only_and_default_off_receipt(self):
        import json
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            root=Path(tmp); src=root/'material-remaster-hires/ready'; src.mkdir(parents=True)
            receipts={}
            for name in (*s.FIRST,*s.GROUPS['valkyrie']):
                data=name.encode(); (src/(name+'.dds')).write_bytes(data)
                receipts[name]={'sha256':hashlib.sha256(data).hexdigest()}
            with patch.object(s,'verify',return_value=receipts):
                out=s.stage(root,'candidate',{},'','valkyrie')
            self.assertEqual({p.stem for p in out.glob('*.dds')},set(s.GROUPS['valkyrie']))
            report=json.loads((out/'review.json').read_text())
            self.assertEqual(report['default'],0)
            self.assertEqual(report['ini_key'],'valkyrie_material_mode')
            self.assertFalse(report['deployed'])
            with self.assertRaises(ValueError): s.stage(root,'other',{},'','all')
    def test_game_destination_refused_before_validation(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            root=Path(tmp)/'steamapps'; root.mkdir()
            with patch.object(s,'verify') as verify:
                with self.assertRaisesRegex(ValueError,'Steam'): s.stage(root,'candidate',{},'')
                verify.assert_not_called()
    def test_remaining_pairs_use_independent_off_gates(self):
        import json
        for group in ('centurion','kasimov'):
            with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
                root=Path(tmp);src=root/'material-remaster-hires/ready';src.mkdir(parents=True)
                receipts={}
                for name in s.GROUPS[group]:
                    data=name.encode();(src/(name+'.dds')).write_bytes(data)
                    receipts[name]={'sha256':hashlib.sha256(data).hexdigest()}
                with patch.object(s,'verify',return_value=receipts):out=s.stage(root,'candidate',{},'',group)
                r=json.loads((out/'review.json').read_text())
                self.assertEqual(r['ini_key'],group+'_material_mode');self.assertEqual(r['default'],0)
                self.assertEqual({p.stem for p in out.glob('*.dds')},set(s.GROUPS[group]))
    def test_complete_stage_lists_real_independent_keys(self):
        import json
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            root=Path(tmp);src=root/'material-remaster-hires/ready';src.mkdir(parents=True)
            receipts={}
            for name in s.GROUPS['complete']:
                data=name.encode();(src/(name+'.dds')).write_bytes(data)
                receipts[name]={'sha256':hashlib.sha256(data).hexdigest()}
            with patch.object(s,'verify',return_value=receipts):out=s.stage(root,'candidate',{},'','complete')
            r=json.loads((out/'review.json').read_text())
            self.assertIsNone(r['ini_key']);self.assertEqual(len(r['ini_keys']),4)
            self.assertEqual(len(list(out.glob('*.dds'))),10);self.assertEqual(r['default'],0)

if __name__=='__main__': unittest.main()
