"""Regression tests after PSVR2 E2E partial (UI scale, focus init, factory gate)."""
import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
LOADOUT_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout.h"
IDENTITY_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout_identity.h"
NATIVE_UI = REPO / "spikes" / "view_matrix" / "native_ui_input.h"
D3D9 = REPO / "spikes" / "view_matrix" / "d3d9.cpp"


class E2eRegression(unittest.TestCase):
    def test_weapon_locker_focus_initializes_equip_page(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertIn("void OnFocus(", body)
        self.assertIn("InterlockedExchange(&g_pageSignal, 1)", body.split("OnFocus", 1)[1].split("OnDefocus", 1)[0])

    def test_native_ui_calls_loadout_on_focus(self):
        native = NATIVE_UI.read_text(encoding="utf-8")
        block = native.split('RvUIGFxMovie_WeaponLocker"', 1)[1].split("NewGame:", 1)[0]
        self.assertIn("LockerVrLoadout::OnFocus", block)

    def test_defocus_clears_page_and_publish(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        defocus = body.split("OnDefocus()", 1)[1].split("OnFocus", 1)[0]
        self.assertIn("g_pageSignal", defocus)
        self.assertIn("Publish::Clear", defocus)
        self.assertNotIn("g_acquireRequested", defocus)
        self.assertNotIn("g_postPopAcquireMask", defocus)

    def test_deferred_acquire_consumes_post_pop_request(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        deferred = body.split("TryRunDeferredAcquire()", 1)[1].split("inline void OnDefocus", 1)[0]
        self.assertIn("g_acquireRequested", deferred)
        self.assertIn("g_postPopAcquireMask", deferred)
        self.assertIn("ApplyPendingMask", deferred)
        schedule = body.split("OnPopMenuScheduleAcquire()", 1)[1].split("TryRunDeferredAcquire", 1)[0]
        self.assertIn("g_acquireRequested", schedule)
        self.assertIn("g_postPopAcquireMask", schedule)

    def _draw_body(self):
        text = D3D9.read_text(encoding="utf-8", errors="replace")
        marker = "static void DrawLockerVrLoadout(IDirect3DDevice9* dev) {"
        return text.rsplit(marker, 1)[1].split("void DrawWeaponWheel", 1)[0]

    def test_font_scale_bounded_not_backbuffer_height(self):
        draw = self._draw_body()
        self.assertIn("LockerVrLoadout::Layout::kFontPx", draw)
        self.assertNotIn("d.Height * .016f", draw)
        self.assertNotIn("d.Height * .034f", draw)

    def test_queued_button_label_distinct_from_equip_all(self):
        draw = self._draw_body()
        compact = re.sub(r"\s+", "", draw)
        self.assertIn('uiQueued?"QUEUED":"EQUIPALL"', compact)
        self.assertIn("ALL OWNED", draw)

    def test_gamesystem_vtable_rva_from_image_base(self):
        ident = IDENTITY_H.read_text(encoding="utf-8")
        self.assertIn("kExpectedRvGameSystemVtableRvaFromImage", ident)
        self.assertIn("ModuleBaseFromTextLo", ident)
        self.assertNotIn("kExpectedRvGameSystemVtableOffsetFromCaptureText", ident)

    def test_load_upgrades_and_load_hooks_preserved(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertIn("g_pageSignal, 2", body)
        self.assertIn("g_pageSignal, 1", body)


if __name__ == "__main__":
    unittest.main()
