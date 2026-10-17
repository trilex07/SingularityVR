from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')

class Tests(unittest.TestCase):
    def test_default_off_and_bounded_timeout(self):
        for section, key in [('Input','StartupPrewarm'),('Render','StartupLoadingGate'),('Diagnostic','StartupReadinessProbe')]:
            self.assertIn(f'GetPrivateProfileIntA("{section}","{key}",0,path)', SRC)
        self.assertIn('(std::max)(500,(std::min)(startupTimeout,10000))', SRC)

    def test_reciprocal_reflected_pawn_not_guessed_or_global_scan(self):
        block=SRC.split('static uintptr_t FindPlayerPawn(bool force) {',1)[1].split('if (g_playerPawn &&',1)[0]
        for s in ('g_startupPrewarm&&g_upChildren','PropOffsetInChain(ctlClass,"Pawn")',
                  'PropOffsetInChain(pawnClass,"Controller")','(uint32_t)ownerBits==g_controller','IsLive(pawn)'):
            self.assertIn(s,block)
        for forbidden in ('FindController(', 'kGObjectsTArray', 'Sleep(', 'WriteProcessMemory('):
            self.assertNotIn(forbidden,block)
        self.assertIn('{ "Pawn", -1 }, { "Controller", -1 }',SRC)

    def test_failed_install_not_marked_ready_in_fast_mode(self):
        self.assertIn('hooked = !g_startupPrewarm || (g_origProcessEvent&&g_hookedAt==g_processInternal)',SRC)
        self.assertIn('earlyNames.Due',SRC);self.assertIn('earlyRoute.Due',SRC)

    def test_curtain_is_last_overlay_for_gpu_and_cpu_copies(self):
        overlay=SRC.index('DrawVrContextPrompt(dev);')
        startup=SRC.index('StartupReadinessFrame(dev);',overlay)
        copy=SRC.index('dev->GetBackBuffer(',overlay)
        self.assertLess(overlay,startup);self.assertLess(startup,copy)

    def test_no_additional_engine_input_or_pause_block(self):
        frame=SRC.split('static void StartupReadinessFrame(IDirect3DDevice9* dev){',1)[1].split('HRESULT STDMETHODCALLTYPE Hook_Present',1)[0]
        for s in ('!Rd(g_gamePaused)','!Rd(g_vrUiMenu)','!CutsceneNativeCameraArmed()','g_xrSessionState==XR_SESSION_STATE_FOCUSED'):
            self.assertIn(s,frame)
        for forbidden in ('ClearPad(', 'Sleep(', 'SetCinematicMode(', 'ProcessEvent(', 'WriteProcessMemory('):
            self.assertNotIn(forbidden,frame)
        self.assertIn('notes<64',frame)

if __name__=='__main__': unittest.main()
