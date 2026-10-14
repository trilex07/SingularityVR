import hashlib
import json
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'audio'))
from patch_note_manifest import patch


class Tests(unittest.TestCase):
    def test_exact_scope_and_guards(self):
        text='A first paragraph.\n\nA second paragraph.'
        original=json.dumps(dict(key='a',section='Notes',en=text,pl=text,status='skip'))+'\n'
        unrelated='{"key":"a","section":"Other","en":"Unrelated"}\n'
        p=dict(key='a',source_sha256=hashlib.sha256(text.encode()).hexdigest(),pl='Pierwszy akapit.\n\nDrugi akapit.')
        result=patch(original+unrelated,[p])
        self.assertTrue(result.endswith(unrelated))
        self.assertEqual(json.loads(result.splitlines()[0])['status'],'translated')
        for bad in ([p,p], [dict(p,source_sha256='0'*64)], [dict(p,pl='Bez podziału')], [dict(p,key='missing')]):
            with self.assertRaises(ValueError):patch(original,bad)
        with self.assertRaises(ValueError):patch(original+original,[p])


if __name__=='__main__':unittest.main()
