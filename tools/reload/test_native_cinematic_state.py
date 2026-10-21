"""Offline integration guards, NOT evidence of a physical camera fix."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
class NativeCinemaTests(unittest.TestCase):
    def test_default_off(self):
        self.assertIn('GetPrivateProfileIntA("Render","NativeCinematicState",0,path)',SOURCE)
    def test_typed_local_ownership(self):
        body=SOURCE.split('static void RefreshNativeCinematicState() {',1)[1].split('\n}',1)[0]
        for token in ('ReadNativeLocalPlayer()', 'local.controller',
                      '"PlayerController"','"Engine"','"BoolProperty"','"bCinematicMode"',
                      'nativecine::SupportedReader','nativecine::ValidField','visited<4096','now-retry>=1000'):
            self.assertIn(token,body)
        for token in ('memcpy(', 'WriteProcessMemory','FindClassByName','kGObjects','SetCutsceneMode'):
            self.assertNotIn(token,body)
        registered=SOURCE.split('WantedName g_wantNames[] = {',1)[1].split('};',1)[0]
        self.assertIn('{ "PCOwner", -1 }',registered)
        self.assertLess(body.index('g_upChildren!=0x4c'),body.index('ReadNativeLocalPlayer('))
        self.assertNotIn('pawn!=g_playerPawn',body)
    def test_refresh_before_scene_and_hands(self):
        present=SOURCE.split('HRESULT STDMETHODCALLTYPE Hook_Present(',1)[1]
        self.assertLess(present.index('RefreshNativeCinematicState();'),present.index('RefreshNativeRestraintHands();'))
        self.assertLess(present.index('RefreshNativeCinematicState();'),present.index('UpdateSceneModeFrame();'))
if __name__=='__main__':unittest.main()
