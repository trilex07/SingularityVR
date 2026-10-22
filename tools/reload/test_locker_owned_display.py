"""Offline checks for locker owned display overlay."""
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
PLANNER = REPO / "spikes" / "view_matrix" / "locker_owned_display_planner.h"
CACHE = REPO / "spikes" / "view_matrix" / "locker_owned_display_cache.h"
POLICY_TEST = REPO / "spikes" / "view_matrix" / "tests" / "test-locker-owned-display.cpp"


class LockerOwnedDisplayOffline(unittest.TestCase):
    def test_planner_present(self):
        text = PLANNER.read_text(encoding="utf-8", errors="replace")
        self.assertIn("PlanExtraTiles", text)
        self.assertIn("kMaxExtraTiles", text)

    def test_cache_independent_of_trace(self):
        cache = CACHE.read_text(encoding="utf-8", errors="replace")
        self.assertNotIn("#include \"locker_native_trace", cache)
        self.assertIn("g_lockerDisplayNativeMesh", cache)
        self.assertIn("mEquippedWeapons", cache)

    def test_planner_exe(self):
        exe = POLICY_TEST.with_suffix(".exe")
        if exe.is_file():
            subprocess.check_call([str(exe)], cwd=str(exe.parent))


if __name__ == "__main__":
    unittest.main()
