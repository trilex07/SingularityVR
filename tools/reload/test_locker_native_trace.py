"""Policy + ABI offline checks for locker_native_trace.h (no game runtime)."""
import re
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
HEADER = REPO / "spikes" / "view_matrix" / "locker_native_trace.h"
CAPTURE = REPO.parent / "research" / "etw" / "capture-20261006-205508" / "Singularity.text-memory.bin"
ABI_TOOL = REPO / "tools" / "reload" / "inspect_locker_abi.py"


class LockerNativeTracePolicy(unittest.TestCase):
    def setUp(self):
        self.text = HEADER.read_text(encoding="utf-8", errors="replace")

    def test_no_inventory_refresh_in_hooks(self):
        forbidden = ("NativeOwnedInventory::Refresh", "NativeOwnedInventory::SnapshotNow")
        for token in forbidden:
            self.assertNotIn(token, self.text, f"forbidden in trace header: {token}")

    def test_separate_abi_typedefs(self):
        self.assertIn("using StartFn", self.text)
        self.assertIn("using FrameFn", self.text)
        self.assertIn("using EquipFn", self.text)
        self.assertNotIn("using ExecFn", self.text)

    def test_atomic_rollback(self):
        self.assertIn("RollbackAllHooks", self.text)
        self.assertIn("install aborted", self.text)

    def test_cached_array_offsets(self):
        self.assertIn("g_offEquipped", self.text)
        self.assertIn("CacheMovieLayout", self.text)
        self.assertNotIn('LogFNameList("ENTER"', self.text)

    def test_disabled_has_no_hook_path_when_ini_off(self):
        self.assertIn("if (!Enabled()) return false;", self.text)
        self.assertIn("TryInstallEarly()", self.text)

    def test_null_focus_defocus(self):
        ui = (REPO / "spikes" / "view_matrix" / "native_ui_input.h").read_text(encoding="utf-8", errors="replace")
        self.assertIn("(!className[0] || strcmp(className, \"RvUIGFxMovie_WeaponLocker\"))", ui)

    def test_caps_declared(self):
        for cap in ("kMaxEvents", "kMaxPerMethod", "kMaxPickupNotes", "kMaxFNameLines", "kMaxInventoryLines"):
            self.assertIn(cap, self.text)

    def test_abi_witness_matches_capture(self):
        if not CAPTURE.is_file():
            self.skipTest("Singularity.text capture missing")
        proc = subprocess.run(
            [sys.executable, str(ABI_TOOL), "--text", str(CAPTURE), "--strict"],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        self.assertIn('"witness_ok": true', proc.stdout)


if __name__ == "__main__":
    unittest.main()
