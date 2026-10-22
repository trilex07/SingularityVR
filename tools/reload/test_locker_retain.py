"""Offline checks for locker retain Phase F (policy + destroy ABI witness)."""
import json
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
HEADER = REPO / "spikes" / "view_matrix" / "locker_retain_ordinary.h"
POLICY = REPO / "spikes" / "view_matrix" / "locker_retain_policy.h"
CAPTURE = REPO.parent / "research" / "etw" / "capture-20261006-205508" / "Singularity.text-memory.bin"
ABI_TOOL = REPO / "tools" / "reload" / "inspect_locker_destroy_abi.py"
POLICY_TEST = REPO / "spikes" / "view_matrix" / "tests" / "test-locker-retain-policy.cpp"


class LockerRetainOffline(unittest.TestCase):
    def test_no_snapshot_in_hook(self):
        text = HEADER.read_text(encoding="utf-8", errors="replace")
        for token in ("SnapshotNow", "Snapshot(", "Mask()"):
            self.assertNotIn(token, text, f"forbidden in retain hook: {token}")
        self.assertIn("Refresh()", text)

    def test_policy_constants(self):
        pol = POLICY.read_text(encoding="utf-8", errors="replace")
        self.assertIn("0x00F1805E", pol)
        self.assertIn("0x00A05790", pol)
        self.assertIn("EstablishedThreadMatches", pol)
        self.assertIn("ManagerMatches", pol)
        self.assertNotIn("if (!establishedThread) return true", pol.replace(" ", ""))

    def test_detour_never_calls_null_orig(self):
        text = HEADER.read_text(encoding="utf-8", errors="replace")
        self.assertIn("ForwardDestroy", text)
        self.assertNotIn("if (!Enabled() || !g_origDestroy) return g_origDestroy", text)
        self.assertIn("if (!g_origDestroy)", text)

    def test_destroy_abi_witness_strict(self):
        self.assertTrue(CAPTURE.is_file(), "archived .text capture required")
        out = subprocess.check_output(
            [sys.executable, str(ABI_TOOL), "--text", str(CAPTURE), "--text-base", "0x402000", "--strict"],
            text=True,
        )
        report = json.loads(out)
        self.assertTrue(report["_witness_ok"])
        self.assertEqual(report["popmenu_destroy"]["return_va"], "0xf1805e")
        self.assertFalse(report["destroyItem"]["popmenu_uses_return"])

    def test_policy_exe_builds(self):
        if not POLICY_TEST.is_file():
            self.skipTest("policy test source missing")
        # Compiled by CI/local build script; run if exe already built next to test.
        exe = POLICY_TEST.with_suffix(".exe")
        if exe.is_file():
            subprocess.check_call([str(exe)], cwd=str(exe.parent))


if __name__ == "__main__":
    unittest.main()
