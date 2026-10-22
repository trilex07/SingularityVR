"""Offline checks for locker owned display overlay."""
import re
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
PLANNER = REPO / "spikes" / "view_matrix" / "locker_owned_display_planner.h"
SESSION = REPO / "spikes" / "view_matrix" / "locker_owned_display_session.h"
CACHE = REPO / "spikes" / "view_matrix" / "locker_owned_display_cache.h"
D3D9 = REPO / "spikes" / "view_matrix" / "d3d9.cpp"
POLICY_TEST = REPO / "spikes" / "view_matrix" / "tests" / "test-locker-owned-display.cpp"


class LockerOwnedDisplayOffline(unittest.TestCase):
    def test_planner_present(self):
        text = PLANNER.read_text(encoding="utf-8", errors="replace")
        self.assertIn("PlanExtraTiles", text)
        self.assertIn("kMaxExtraTiles", text)

    def test_cache_independent_of_trace(self):
        cache = CACHE.read_text(encoding="utf-8", errors="replace")
        self.assertNotIn('#include "locker_native_trace', cache)
        self.assertIn("g_lockerDisplayNativeMesh", cache)
        self.assertIn("g_offEquipped", cache)
        self.assertIn("SameLockerFocusSession", cache)
        self.assertIn("RefreshNativeMeshesFromCachedOffset", cache)
        self.assertNotIn("RefreshNativeMeshes(movie, cls)", cache)
        on_focus = re.search(r"inline void OnFocus\(.*?\n\}", cache, re.S)
        self.assertIsNotNone(on_focus)
        body = on_focus.group(0)
        self.assertIn("SameLockerFocusSession", body)
        self.assertIn("return;", body)
        self.assertNotIn("g_nextRefresh = 0", body)
        tick = re.search(r"inline void TickWhileActive\(.*?\n\}", cache, re.S)
        self.assertIsNotNone(tick)
        self.assertNotIn("Property(", tick.group(0))

    def test_focus_transition_only_refresh(self):
        cache = CACHE.read_text(encoding="utf-8", errors="replace")
        self.assertIn("NativeOwnedInventory::Refresh()", cache)
        transition = cache.split("NativeOwnedInventory::Refresh()")[0]
        self.assertIn("SameLockerFocusSession", transition)

    def test_render_gates_and_rt_restore(self):
        draw = D3D9.read_text(encoding="utf-8", errors="replace")
        start = draw.find("static void DrawLockerOwnedDisplay(IDirect3DDevice9* dev) {")
        self.assertGreater(start, 0)
        chunk = draw[start : start + 4500]
        self.assertIn("g_weaponWheelOpen", chunk)
        self.assertIn("g_origSetRenderTarget(dev, 0, old)", chunk)

    def test_session_helpers(self):
        text = SESSION.read_text(encoding="utf-8", errors="replace")
        self.assertIn("SameLockerFocusSession", text)
        self.assertIn("LockerFocusTransition", text)

    def test_planner_exe(self):
        exe = POLICY_TEST.with_suffix(".exe")
        if exe.is_file():
            subprocess.check_call([str(exe)], cwd=str(exe.parent))


if __name__ == "__main__":
    unittest.main()
