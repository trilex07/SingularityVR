"""Narrow source-contract guards, not a substitute for D3D pixels or Steam."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CPP = (ROOT / "spikes/view_matrix/d3d9.cpp").read_text(encoding="utf-8-sig")
MIRROR_H = (ROOT / "spikes/view_matrix/desktop_mirror.h").read_text(encoding="utf-8-sig")
RIFLE_CANDIDATE = (ROOT / "tools/reload/prepare_vr_settings_rifle_test.ps1").read_text(encoding="utf-8-sig")
TUTORIAL = (ROOT / "spikes/view_matrix/vr_tutorial.h").read_text(encoding="utf-8-sig")
RELOAD = (ROOT / "spikes/view_matrix/weapon_reload_dispatch.inl").read_text(encoding="utf-8-sig")


class Contract(unittest.TestCase):
    def test_chord_no_capture(self):
        chord = CPP.split("if (g_recenterChord.Update(l3Held, r3Held)) {", 1)[1].split(
            "const bool r3Released", 1)[0]
        self.assertNotIn("g_captureRequested", chord)
        self.assertIn("g_recenterRequested", chord)
        self.assertIn("capture=disabled", chord)

    def test_f12_reserved(self):
        self.assertNotIn("DebugKey(VK_F12)", CPP)
        self.assertNotIn("GetAsyncKeyState(VK_F12)", CPP)

    def test_order(self):
        present = CPP.split("const bool mirrored = desktop_mirror::Apply", 1)[1]
        self.assertLess(CPP.index("UpdateFromHeadset(s);"), CPP.index("const bool mirrored = desktop_mirror::Apply"))
        self.assertLess(present.index("StampCaptureRt"), present.index("PresentBackbufferScope"))
        self.assertLess(present.index("PresentBackbufferScope"), present.index("g_origPresent(s, a, b, c, d)"))

    def test_default_off(self):
        self.assertIn('GetPrivateProfileIntA("Render", "DesktopMirrorWindow", 0, path)', CPP)
        self.assertIn('GetPrivateProfileIntA("Render", "SteamCaptureBackbuffer", 0, path)', CPP)

    def test_reset_not_diagnostic_only(self):
        self.assertIn("if(g_diagEnabled || Rd(g_desktopMirrorWindow) || Rd(g_desktopMirrorEye) || NextReloadEnabled())", CPP)
        self.assertIn("desktop_mirror::ReleaseScratch();", CPP)

    def test_reset_guard_enforces_sbs(self):
        self.assertEqual(CPP.count("DecideResetSize("), 1)
        self.assertEqual(CPP.count("if (pp && dev) GuardResetSize(pp);"), 2)
        self.assertIn("pp->BackBufferWidth = d.w", CPP)
        self.assertIn("pp->BackBufferHeight = d.h", CPP)
        self.assertIn("reset-guard:", CPP)
        self.assertIn("FUSE BLOWN", CPP)

    def test_window_never_resized(self):
        # MAIN root cause (dumped frame): UE3 binds viewport + Present region
        # to the window size, so SetWindowPos blackens the right eye. The eye
        # fits the visible client region instead; the window stays SBS-sized.
        self.assertNotIn("SetWindowPos", MIRROR_H)
        self.assertNotIn("FitWindow", MIRROR_H)
        self.assertNotIn("WindowBox", MIRROR_H)

    def test_sample_config(self):
        ini = (ROOT / "spikes/view_matrix/SingularityVR.ini").read_text(encoding="utf-8-sig")
        for key in ("DesktopMirrorEye=2", "DesktopMirrorFill=0", "DesktopMirrorWindow=1", "SteamCaptureBackbuffer=1"):
            self.assertIn(key, ini)

    def test_steamvr_capture_alpha_retained_in_rifle_candidate(self):
        self.assertIn("'Render/XrCaptureOpaqueAlpha'='1'", RIFLE_CANDIDATE)
        self.assertIn('GetPrivateProfileIntA("Render","XrCaptureOpaqueAlpha",0,path)', CPP)
        self.assertLess(CPP.index('DrawAmmoPlate(dev);'), CPP.index('PrepareXrCaptureAlpha(dev,g_sharedSurf9);'))

    def test_bilingual_magazine_hint_stays_in_each_eye_and_is_contextual(self):
        self.assertIn('NACI\\xC5\\x9ANIJ O, ABY ZWOLNI', TUTORIAL)
        self.assertIn('PRESS O TO RELEASE THE MAGAZINE', TUTORIAL)
        self.assertIn('if(circleEdge&&next.valid)next.hintUntilMs=now+8000;', RELOAD)
        self.assertIn('DrawNextReloadHint(s);', CPP)
        self.assertIn('const int half=int(desc.Width/2)', CPP)
        self.assertIn('widest+2)*(kGlyphW+1)*px>half*84/100', CPP)


if __name__ == "__main__":
    unittest.main()
