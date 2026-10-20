"""Source wiring guards, not live-game alignment or render proof."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class Wiring(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        cls.policy = (ROOT/'spikes/view_matrix/centurion_reload.h').read_text(encoding='utf-8')

    def test_default_off_and_explicit_dependencies(self):
        self.assertIn('GetPrivateProfileIntA("Input","CenturionTrackedCylinder",0,path)', self.source)
        self.assertIn('if(g_centurionTrackedCylinder&&!g_centurionVisualFollowup){g_centurionTrackedCylinder=0;', self.source)
        self.assertIn('g_centurionConfig.circleOpens=g_centurionTrackedCylinder!=0;', self.source)

    def test_legacy_circle_and_radius_not_replaced(self):
        self.assertIn('bool circleOpens=false;', self.policy)
        self.assertIn('c.circleOpens?enter(Phase::Open,Event::Opened):enter(Phase::Unlocked,Event::Unlocked)', self.policy)
        self.assertIn('pouchRadius=.17f', self.policy)
        self.assertIn('insertRadius=.16f', self.policy)

    def test_render_socket_only_published_after_success_and_restore(self):
        self.assertIn('cylinderBase>=0&&SUCCEEDED(dhr)&&SUCCEEDED(nativeRestore)&&', self.source)
        self.assertIn('g_centurionSocket=cylinderReplay.socket;', self.source)
        self.assertLess(self.source.index('const HRESULT nativeRestore=nativeIndices.Restore();'),
                        self.source.index('g_centurionSocket=cylinderReplay.socket;'))

    def test_input_requires_transaction_freshness_and_actual_carried_centre(self):
        self.assertIn('carriedValid&&g_centurionSocket.Current(f.pawn,f.weapon,g_centurionVisualTransaction,now)', self.source)
        self.assertIn('f.insertOffset=carried.p-f.hand;', self.source)
        self.assertIn('f.socketFresh&&Near(f.hand+f.insertOffset,f.socket,c.insertRadius)', self.policy)

    def test_no_private_depth_or_material_changes_in_prepare(self):
        block = self.source.split('static int PrepareCenturionCylinder(', 1)[1].split('HRESULT STDMETHODCALLTYPE Hook_DrawIndexedPrim', 1)[0]
        for forbidden in ('SetTexture(', 'SetRenderTarget(', 'SetDepthStencilSurface(', 'Clear(', 'new ', 'Log('):
            self.assertNotIn(forbidden, block)
        self.assertIn('centuriontracked::Attached(', block)

    def test_preserves_main_hud_reseat(self):
        self.assertIn('struct HudReseatPolicy', self.policy)
        self.assertIn('FindHudObjectExcept', self.source)


if __name__ == '__main__':
    unittest.main()
