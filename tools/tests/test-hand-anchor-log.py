"""Synthetic parser regressions; no captured game constants in Git."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('anchor', Path(__file__).parents[1]/'analyze-hand-anchor-log.py')
anchor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(anchor)

def record(ride=False, stage=False):
    header = 'hand-anchor sample=1 frame=10 moved=0 targetL=(1,2,3) targetR=(4,5,6)'
    if stage:
        header += '; stage=GameplayStereo'
    fields = [('local230',16),('controllerAim',16),('controllerOff',16)]
    if ride:
        fields += [('cameraRide',16)]
    fields += [(x,12) for x in ('nativeLeft9','nativeRight51','posedLeft9','posedRight51')]
    return '\n'.join([header]+['hand-anchor '+name+': '+' '.join(['0']*width) for name,width in fields])

class Tests(unittest.TestCase):
    def test_legacy_context(self):
        rows=anchor.extract('SceneMode: NativeCinematic -> GameplayStereo\n'+record())
        self.assertEqual(rows[0]['mode'],'GameplayStereo')
        self.assertNotIn('cameraRide',rows[0])
    def test_explicit_stage_and_ride(self):
        rows=anchor.extract('SceneMode: GameplayStereo -> NativeCinematic\n'+record(True,True))
        self.assertEqual(rows[0]['mode'],'GameplayStereo')
        self.assertEqual(len(rows[0]['cameraRide']),16)
    def test_incomplete_refuses(self):
        with self.assertRaises(ValueError):anchor.extract(record()[:-2])
    def test_bound_refuses(self):
        with self.assertRaises(ValueError):anchor.extract('\n'.join([record()]*9))
    def test_current_two_epochs(self):
        first=record(True,True).replace('moved=0 targetL','moved=0 weapon=00000000 epoch=1 targetL')
        second=record(True,True).replace('moved=0 targetL','moved=0 weapon=01234567 epoch=2 targetL')
        rows=anchor.extract('\n'.join([first]*8+[second]*8))
        self.assertEqual(len(rows),16)
        self.assertEqual(rows[0]['weapon'],0)
        self.assertEqual(rows[-1]['weapon'],0x01234567)
    def test_nonfinite_refuses(self):
        with self.assertRaises(ValueError):anchor.extract(record().replace('local230: 0','local230: nan'))
    def test_tmd_is_not_misread_as_notmd(self):
        tmd=record(True,True).replace('hand-anchor','tmd-hand-anchor')
        self.assertEqual(anchor.extract(tmd),[])
        self.assertEqual(len(anchor.extract(tmd+'\n'+record(True,True))),1)

if __name__=='__main__':unittest.main()
