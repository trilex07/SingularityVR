"""Structural wiring regression checks, NOT native execution or headset proof."""
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
class Wiring(unittest.TestCase):
    def test_no_shared_gun_or_camera_write(self):
        s=(ROOT/'spikes/view_matrix/native_tmd_aim.h').read_text()
        for forbidden in ('g_aimYawUU','g_aimPitchUU','g_wantYaw =','g_wantPitch =',
                          'VfxFireEvent','Hook_SingleLineCheck','RotateTrace('):
            self.assertNotIn(forbidden,s)
        self.assertIn('originalAim(self,edx,start,direction);',s)
        self.assertIn('if(!agingDepth)Rewrite',s)
        self.assertIn('__finally { --agingDepth; }',s)

    def test_default_off_real_native_gates(self):
        s=(ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        h=(ROOT/'spikes/view_matrix/native_tmd_aim.h').read_text()
        self.assertIn('volatile LONG g_tmdNativeAim = 0;',s)
        self.assertIn('GetPrivateProfileIntA("Input","TmdNativeAim",0,path)',s)
        for marker in ('CameraOwnedPawn()', '"mTMD"','"RvPlayerPawnSP"','P(tmd+0x98)==pawn',
                       'engineThread!=GetCurrentThreadId()', 'tmdaim::Current(sample,pawn',
                       'Rd(g_fireDepth)>0','TryAcquireSRWLockShared', 'Method(aim,119',
                       'Method(aging,59','Method(owner,7'):
            self.assertIn(marker,h)

    def test_renderer_publishes_same_matrix(self):
        s=(ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        start=s.index('static void BuildTmdC(float C[4][4]) {')
        end=s.index('// 0 = leave it where the engine put it',start)
        block=s[start:end]
        self.assertIn('memcpy(sample.matrix,C,sizeof(sample.matrix))',block)
        self.assertLess(block.index('ApplyMeshScale(C, H)'),block.index('NativeTmdAim::Publish'))
        self.assertIn('sample.pawn=g_playerPawn;sample.time=GetTickCount64()',block)
        self.assertIn('NativeTmdAim::Poll();',(ROOT/'spikes/view_matrix/native_ui_input.h').read_text())

if __name__=='__main__':
    unittest.main()
