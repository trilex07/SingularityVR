"""Static regression guards for the existing native-animation ownership contract.

These checks do not prove an animation is played or visible in PSVR2.
"""
from pathlib import Path
import unittest

SOURCE = (Path(__file__).resolve().parents[2] /
          'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')


class NativeActionHandsTests(unittest.TestCase):
    def test_native_animation_precedes_destructive_hand_policies(self):
        for signature in (
            'static bool FgIndependentHandsCandidate(UINT verts, UINT first, UINT triangles) {',
            'static bool CustomGlovesEligible() {',
            'static bool FgHidden(IDirect3DDevice9* dev,UINT verts, UINT first, UINT triangles) {',
        ):
            body = SOURCE.split(signature, 1)[1].split('\n}', 1)[0]
            statements = [line.strip() for line in body.splitlines()
                          if line.strip() and not line.strip().startswith('//')]
            self.assertEqual(statements[0], 'if (NativeAnimatedHandsNow()) return false;', signature)

    def test_native_animation_excludes_tmd_triangle_and_bone_surgery(self):
        gate = SOURCE.split('const bool tmdSurgeryOn =', 1)[1].split(';', 1)[0]
        self.assertIn('!NativeAnimatedHandsNow()', gate)
        self.assertIn('if (tmdSurgeryOn && kBoneReg0 >= 0)', SOURCE)
        self.assertIn('if (tmdSurgeryOn && primCount > 0', SOURCE)

    def test_cinema_requires_opt_in_and_native_camera(self):
        self.assertIn('return Rd(g_cutsceneNativeHands) &&', SOURCE)
        self.assertIn('Rd(g_cinematicHandsOwnership) && Rd(g_inCinematic) && VrModeOn()', SOURCE)
        self.assertIn('GetPrivateProfileIntA("Render","CinematicHandsOwnership",0,path)', SOURCE)
        self.assertIn('GetPrivateProfileIntA("Render","CutsceneNativeHands",0,path)', SOURCE)
        self.assertIn('!NativeCinematicHandsNow() && CutsceneHideTrackedForeground(', SOURCE)
        self.assertIn('return !NativeAnimatedHandsNow();', SOURCE)

    def test_scene_probe_is_bounded_and_does_not_classify_by_state_name(self):
        body = SOURCE.split('static void ObserveCinematicSceneProducer() {', 1)[1].split('\n}', 1)[0]
        self.assertIn('if(!Rd(g_cinematicSceneProbe))return;', body)
        self.assertIn('samples>=120', body)
        self.assertIn('now-last<1000', body)
        self.assertNotIn('InterlockedExchange(&g_inCinematic', body)
        self.assertIn('indexedFgSeen=', body)
        self.assertIn('GetPrivateProfileIntA("Diagnostic","CinematicSceneProbe",0,path)', SOURCE)

    def test_event_bridge_requires_exact_local_reflected_bool_layout(self):
        body = SOURCE.split('static void ApplyCinematicEventBridge(void* self,void* frame) {', 1)[1].split('\n}', 1)[0]
        self.assertIn('if(!Rd(g_cinematicEventBridge))return;', body)
        for token in ('"PCOwner"', '"RvPlayerController"', '"Function"', '"BoolProperty"',
                      '"bInCinematicMode"', 'offset==(int)i*4', 'n<16', 'cineevent::Accept'):
            self.assertIn(token, body)
        self.assertNotIn('memcpy(', body)
        self.assertIn('GetPrivateProfileIntA("Render","CinematicEventBridge",0,path)', SOURCE)
        self.assertIn('fnIdx==FnIdx(kFnPlayerCinematicProbeFirst))ApplyCinematicEventBridge(self,Stack)', SOURCE)


if __name__ == '__main__':
    unittest.main()
