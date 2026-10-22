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
WITNESS_H = REPO / "spikes" / "view_matrix" / "locker_native_trace_witness.h"
WITNESS_TEST = REPO / "spikes" / "view_matrix" / "tests" / "test-locker-native-trace-witness.cpp"


def allow_outer_exec_witness(reenter_depth_after_increment: int) -> bool:
    return reenter_depth_after_increment == 1


def buggy_log_enter_refused(reenter_depth_after_increment: int) -> bool:
    return reenter_depth_after_increment > 0


def log_enter_permitted(reenter_depth_after_increment: int) -> bool:
    return reenter_depth_after_increment == 1


def simulate_log_enter_after_detour_increment(*, armed: bool, use_buggy_guard: bool) -> bool:
    depth = 1
    if not armed:
        return False
    if use_buggy_guard and buggy_log_enter_refused(depth):
        return False
    return log_enter_permitted(depth)


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

    def test_page_exec_hooks_and_ctx(self):
        self.assertIn("kFnLoadUpgrades", self.text)
        self.assertIn("kFnUpgrade", self.text)
        self.assertIn("LogEventContext", self.text)
        self.assertIn("g_offActiveSlot", self.text)
        self.assertIn("g_offRequestedPackages", self.text)
        self.assertIn("locker avail witness", (REPO / "spikes/view_matrix/locker_availability_witness.h").read_text(encoding="utf-8"))

    def test_page_methods_offline_witness(self):
        tool = REPO / "tools/reload/inspect_locker_page_methods.py"
        exe = Path(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\Singularity.exe")
        if not CAPTURE.is_file() or not exe.is_file():
            self.skipTest("capture or exe missing")
        proc = subprocess.run(
            [sys.executable, str(tool), "--exe", str(exe), "--text", str(CAPTURE)],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        self.assertIn("0xf87c80", proc.stdout.lower())
        self.assertIn("0xf87e60", proc.stdout.lower())

    def test_first_level_exec_witness_not_blocked_by_reenter_guard(self):
        self.assertTrue(allow_outer_exec_witness(1))
        self.assertTrue(log_enter_permitted(1))
        self.assertTrue(buggy_log_enter_refused(1))
        self.assertTrue(
            simulate_log_enter_after_detour_increment(armed=True, use_buggy_guard=False),
            "first-level ENTER must run after detour ++t_reenter",
        )
        self.assertFalse(
            simulate_log_enter_after_detour_increment(armed=True, use_buggy_guard=True),
            "regression: t_reenter>0 guard silences first-level ENTER",
        )
        self.assertFalse(allow_outer_exec_witness(2))

    def test_header_uses_witness_not_buggy_reenter_guard(self):
        self.assertTrue(WITNESS_H.is_file())
        enter_body = re.search(r"inline void LogEnter\([^)]*\)\s*\{", self.text)
        self.assertIsNotNone(enter_body)
        snippet = self.text[enter_body.start() : enter_body.start() + 420]
        self.assertNotIn("t_reenter > 0", snippet)
        self.assertIn("LogEnterPermitted", snippet)

    def test_prefocus_start_and_load_caller_witness(self):
        self.assertIn("TryArmSessionFromStart", self.text)
        self.assertIn("pre-focus Start", self.text)
        self.assertIn("_ReturnAddress", self.text)
        self.assertIn("callerRet=", self.text)

    def test_witness_cpp_unit(self):
        if not WITNESS_TEST.is_file():
            self.skipTest("witness test source missing")
        import shutil
        import tempfile

        cl = shutil.which("cl")
        if not cl:
            self.skipTest("cl.exe not on PATH")
        with tempfile.TemporaryDirectory() as td:
            exe = Path(td) / "test-locker-native-trace-witness.exe"
            src = str(WITNESS_TEST)
            inc = str(REPO / "spikes" / "view_matrix")
            cmd = ["cl", "/nologo", "/EHsc", "/O2", f"/I{inc}", src, f"/Fe:{exe}"]
            proc = subprocess.run(cmd, capture_output=True, text=True, cwd=td)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            run = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertIn("PASS", run.stdout)

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
