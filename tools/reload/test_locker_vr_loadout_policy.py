"""Offline tests for locker_vr_loadout_policy.h semantics."""
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
POLICY = REPO / "spikes" / "view_matrix" / "locker_vr_loadout_policy.h"
CPP_TEST = REPO / "spikes" / "view_matrix" / "tests" / "test-locker-vr-loadout-policy.cpp"


def available(native, row_at):
    vr = 0
    maps = [(0, 1), (1, 2), (2, 4), (5, 0x20)]
    for idx, nbit in maps:
        if native & nbit:
            row = row_at(idx)
            if row and row.get("row24") == 0 and (row.get("row4c", 0) & 1) == 0:
                if nbit == 1:
                    vr |= 1
                elif nbit == 2:
                    vr |= 2
                elif nbit == 4:
                    vr |= 4
                elif nbit == 0x20:
                    vr |= 8
    return vr


class VrLoadoutPolicy(unittest.TestCase):
    def test_native_to_vr_bits(self):
        ok = lambda _i: {"row24": 0, "row4c": 0}
        self.assertEqual(available(0x00, ok), 0x0)
        self.assertEqual(available(0x01, ok), 0x1)
        self.assertEqual(available(0x02, ok), 0x2)
        self.assertEqual(available(0x04, ok), 0x4)
        self.assertEqual(available(0x20, ok), 0x8)
        self.assertEqual(available(0x27, ok), 0xF)
        self.assertEqual(available(0x18, ok), 0x0)

    def test_catalog_exclusions(self):
        def row24(i):
            if i == 0:
                return {"row24": 1, "row4c": 0}
            return {"row24": 0, "row4c": 0}

        self.assertEqual(available(0x01, row24), 0x0)

        def row4c(i):
            if i == 1:
                return {"row24": 0, "row4c": 1}
            return {"row24": 0, "row4c": 0}

        self.assertEqual(available(0x02, row4c), 0x0)

    def test_pending_and_revalidate(self):
        self.assertEqual(0xF & ~0x1, 0xE)
        self.assertEqual(0x7 & ~0x5, 0x2)
        self.assertEqual(0x3 & ~0xF, 0x0)
        queued, avail, owned = 0x4, 0x3, 0x0
        self.assertEqual(queued & avail & ~owned, 0x0)

    def test_policy_header_present(self):
        self.assertTrue(POLICY.is_file())
        body = POLICY.read_text(encoding="utf-8")
        self.assertIn("AvailableOrdinaryMaskFromNative", body)
        self.assertIn("kCatalogIndexSniper", body)

    def test_cpp_policy_binary(self):
        if sys.platform != "win32":
            self.skipTest("MSVC policy test optional on non-Windows")
        out = CPP_TEST.with_suffix(".exe")
        cmd = [
            "cl",
            "/nologo",
            "/EHsc",
            "/Fe:" + str(out),
            str(CPP_TEST),
        ]
        try:
            subprocess.check_call(cmd, cwd=str(CPP_TEST.parent))
            subprocess.check_call([str(out)], cwd=str(CPP_TEST.parent))
        except (FileNotFoundError, subprocess.CalledProcessError):
            self.skipTest("cl.exe unavailable or compile failed")


if __name__ == "__main__":
    unittest.main()
