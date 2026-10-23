"""Integration guardrails (source wiring, not a GPU/PSVR2 claim)."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
CPP=(ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')

class Wiring(unittest.TestCase):
    def test_defaults(self):
        for section,key in [('Input','DualWeaponTmdReady'),('Diagnostic','DualWeaponTmdProbe')]:
            self.assertIn(f'GetPrivateProfileIntA("{section}","{key}",0,path)',CPP)
    def test_native_status_not_redefined(self):
        self.assertIn('inline bool WeaponInHands()  { return Rd(g_handsStatus) == HANDS_WEAPON; }',CPP)
        helper=CPP[CPP.index('static bool WeaponPresentationInHands() {'):].split('\n}',1)[0]
        self.assertIn('dualtmd::Weapon(WeaponInHands()',helper)
        self.assertNotIn('InterlockedExchange',helper)
    def test_no_tmd_melee_window(self):
        self.assertEqual(CPP.count('dualtmd::MeleeWindow(dualContext,g_hipKnifeState.held>=0)'),2)
        self.assertEqual(CPP.count('tmdgauntlet::ArmImpulseMelee(g_tmdShortGauntlet,Rd(g_haveTMD)==1,g_hipKnifeState.held>=0)'),2)
        self.assertIn('ArmMeleeWindowFromKnife();',CPP) # separate real-knife path retained
    def test_shared_support_and_release(self):
        self.assertIn('feedGame && WeaponPresentationInHands()',CPP)
        self.assertIn('dualPowerLatch[dualPower].Update(dualOwnsPower,dualPowerReserved,down)',CPP)
        self.assertIn('dualPowerReserved=!dualContext||supportActive||g_centurionOwnsGrip||g_nextOwnsGrip',CPP)
    def test_original_hardware_draw_scope(self):
        start=CPP.index('if (isolateTmdHardware) {')
        block=CPP[start:CPP.index('for (int s = 0; s < nSeg;',start)]
        self.assertIn('tmdgauntlet::DrawHardwareFallback',block)
        self.assertIn('g_origDrawIndexedPrim',block)
        self.assertIn('if(dualHardware&&(!independentRegs||Rd(g_twoHandActive)',CPP)
        # Original primitive list is emitted inside the existing per-eye lambda.
        pair=CPP.rfind('StereoPair(',0,start)
        self.assertGreater(pair,CPP.index('const bool isolateTmdHardware'))
        self.assertIn('nativeIndices.Restore()',CPP[start:])
        self.assertIn('const HRESULT handRestore=',CPP[start:])
    def test_probe_before_hide(self):
        start=CPP.index('const bool dualProbe=')
        self.assertLess(start,CPP.index('if (FgHidden(dev,numVertices',start))
        self.assertIn('[signature counts, NOT pixels]',CPP)
    def test_hardware_selection_precedes_broad_arm_hide(self):
        block=CPP[CPP.rindex('static bool FgHidden(IDirect3DDevice9* dev,UINT verts,'):]
        short=block.index('gloveisolate::IsArmFleshSection')
        gate=block.index('gloveisolate::TmdHardwareCutApplies')
        stray=block.index('gloveisolate::IsStrayFingerSection')
        self.assertLess(short,gate)
        self.assertLess(gate,stray)
        self.assertIn('return Rd(g_twoHandActive)||!g_handPoseValid[0]||!g_handPositionValid[0];',block[:gate+400])
    def test_tmd_palette_owns_camera_pose_before_gun_ride(self):
        # This is the actual draw-dispatch seam. The policy's CPU truth table
        # alone cannot catch a caller which still sends TMD material 1 to
        # RideTargetFor(numVertices) and loses its exact section identity.
        start=CPP.index('static int RideTargetFor(UINT verts,UINT first,UINT triangles) {')
        end=CPP.index('static void BuildGunC(',start)
        route=CPP[start:end]
        self.assertLess(route.index('dualtmd::IndependentTmdOwnsPose('),
                        route.index('FgRidesGun(verts)'))
        self.assertIn('dualtmd::NativeTmdSection(verts,first,triangles)',route)
        self.assertIn('RideTargetFor(numVertices,startIndex,primCount)',CPP)
        self.assertIn('const int independentRegs=t==D3DPT_TRIANGLELIST ? PrepareIndependentLeft(',CPP)

if __name__=='__main__':unittest.main()
