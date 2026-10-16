"""Narrow source-contract guards, not a substitute for D3D pixels or Steam."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CPP = (ROOT / "spikes/view_matrix/d3d9.cpp").read_text(encoding="utf-8-sig")


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
        self.assertIn("if(g_diagEnabled || Rd(g_desktopMirrorWindow) || Rd(g_desktopMirrorEye))", CPP)
        self.assertIn("desktop_mirror::ReleaseScratch();", CPP)

    def test_reset_guard_enforces_sbs(self):
        self.assertEqual(CPP.count("DecideResetSize("), 1)
        self.assertEqual(CPP.count("if (pp && dev) GuardResetSize(pp);"), 2)
        self.assertIn("pp->BackBufferWidth = d.w", CPP)
        self.assertIn("pp->BackBufferHeight = d.h", CPP)
        self.assertIn("reset-guard:", CPP)

    def test_sample_config(self):
        ini = (ROOT / "spikes/view_matrix/SingularityVR.ini").read_text(encoding="utf-8-sig")
        for key in ("DesktopMirrorEye=2", "DesktopMirrorFill=1", "DesktopMirrorWindow=0", "SteamCaptureBackbuffer=0"):
            self.assertIn(key, ini)


if __name__ == "__main__":
    unittest.main()
