"""Pin GetItemAt / TransferData offline witnesses (drift fails CI)."""
import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOL = Path(__file__).resolve().parent / "inspect_use_station_methods.py"


class TestUseStationMethods(unittest.TestCase):
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
        self.assertEqual(
            report["script_wrappers"]["intURvUIGFxMovie_BarisovStationexecGetItemAt"]["resolved"],
            "0xf6e5a0",
        )
        self.assertEqual(report["vtable_impl"]["GetItemAt"]["va"], "0xf23b60")


if __name__ == "__main__":
    unittest.main()
