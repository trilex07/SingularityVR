"""Drift gate for inspect_locker_progression_features.py pinned addresses."""
import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOL = Path(__file__).resolve().parent / "inspect_locker_progression_features.py"


class TestLockerProgressionFeatures(unittest.TestCase):
    def test_strict_witness(self):
        proc = subprocess.run(
            [sys.executable, str(TOOL), "--strict"],
            capture_output=True,
            text=True,
            cwd=ROOT,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        report = json.loads(proc.stdout)
        self.assertTrue(report["_witness_ok"])
        self.assertEqual(report["vtable_verify"]["vtable_slot_484"]["resolved"], "0xfc4c10")
        self.assertEqual(report["subobject_at_pawn_3d0"]["type"], "RvInventoryManager")


if __name__ == "__main__":
    unittest.main()
