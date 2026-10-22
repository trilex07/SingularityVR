"""Integration wiring guards complement the executable layout/input tests."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "spikes/view_matrix/d3d9.cpp").read_text(encoding="utf-8")
OVERLAY = (ROOT / "spikes/view_matrix/vr_menu_overlay.h").read_text(encoding="utf-8")


class SettingsPanelTests(unittest.TestCase):
    def test_aggregate_trigger_and_right_hand_selection(self):
        self.assertIn("ReadPanelTrigger(g_actFire, pollOff)", SOURCE)
        self.assertIn("ReadPanelTrigger(g_actAltFire, pollOff)", SOURCE)
        self.assertIn("panelFireHand == 1 ? panelRt : panelLt", SOURCE)
        self.assertIn("panelClick.Update(true, panelPointerHeld)", SOURCE)
        self.assertIn("panelClick.Update(false, false)", SOURCE)

    def test_shared_measured_backbuffer_layout(self):
        self.assertEqual(SOURCE.count("settingspanel::Make("), 2)
        self.assertIn("(int)panelDesc.Width,(int)panelDesc.Height", SOURCE)
        self.assertIn("width=(int)Rd(g_panelBackW),height=(int)Rd(g_panelBackH)", SOURCE)
        self.assertIn("dev->SetViewport(&panelOldVp)", SOURCE)
        self.assertIn("settingspanel::RowAt(layout,height,screenU,screenV)", SOURCE)
        self.assertIn("VrUiProjectEye(eye,A,a)", OVERLAY)  # beam starts in physical space
        self.assertIn("VrUiProjectPointerPoint(eye,B,b)", OVERLAY)
        self.assertIn("VrUiProjectPointerPoint(eye,point,projected)", OVERLAY)
        self.assertIn("if(!Rd(g_panelOpen))return VrUiProjectEye", OVERLAY)

    def test_minimal_settings_do_not_save_hidden_calibration(self):
        self.assertIn("kMinimalRoot[]", SOURCE)
        self.assertIn("kMinimalControls[]", SOURCE)
        self.assertIn("page != 0 && page != 1 && page != 10 && page != 11", SOURCE)
        self.assertIn("PanelSaveRow(PanelIdForPage(page, i), path)", SOURCE)

    def test_optional_hint_gate_does_not_change_fsm(self):
        start = SOURCE.index("static void DrawNextReloadHint(IDirect3DDevice9* dev)")
        self.assertIn("if (!Rd(g_reloadHintsEnabled)) return;", SOURCE[start:start + 180])
        self.assertIn('WritePrivateProfileStringA("Input", "ReloadHints", v, path)', SOURCE)
        self.assertIn('GetPrivateProfileIntA("Input","ReloadHints",0,path)', SOURCE)
        self.assertIn('GetPrivateProfileIntA("Render","VrSettingsMinimal",0,path)', SOURCE)

    def test_session_only_test_preserves_exact_ini_restore_lease(self):
        save = SOURCE[SOURCE.index("void PanelSave() {"):SOURCE.index("void PanelRowText(")]
        self.assertLess(save.index("if (g_vrSettingsSessionOnly)"), save.index("IniPath(path)"))
        self.assertIn('GetPrivateProfileIntA("Diagnostic","VrSettingsSessionOnly",0,path)', SOURCE)
        config = (ROOT / "tools/reload/prepare_vr_settings_rifle_test.ps1").read_text(encoding="utf-8")
        self.assertIn("$targets['Diagnostic/VrSettingsSessionOnly']='1'", config)

    def test_tmd_help_matches_shipped_sense_actions(self):
        self.assertIn('"/user/hand/left/input/trigger/value",     "Impulse"', SOURCE)
        self.assertIn('"/user/hand/left/input/x/click",           "AgeYoung"', SOURCE)
        self.assertIn('"L2: IMPULSE (LEFT TRIGGER)"', SOURCE)
        self.assertIn('"LEFT SQUARE: AGE / RENOVATE"', SOURCE)
        self.assertIn('case PR_TMD_HELP_IMPULSE: case PR_TMD_HELP_AGE: case PR_TMD_HELP_RULES:', SOURCE)

    def test_centurion_probe_does_not_write_a_new_shot_direction(self):
        start = SOURCE.index('if(g_centurionAimProbe&&am==11')
        end = SOURCE.index('if(g_sniperOpticalAccuracy)NativeScopeShotReceipt', start)
        audit = SOURCE[start:end]
        self.assertIn('Rd(g_gunVerts)==5799', audit)
        self.assertIn('LegacyGunAimDir(expected)', audit)
        self.assertIn('BuildGunC(C)', audit)
        self.assertIn('spread/recoil NOT removed', audit)
        self.assertNotIn('RotateTrace(', audit)
        self.assertNotIn('weapontrace::Align(', audit)
        self.assertIn('GetPrivateProfileIntA("Diagnostic","CenturionAimProbe",0,path)', SOURCE)


if __name__ == "__main__":
    unittest.main()
