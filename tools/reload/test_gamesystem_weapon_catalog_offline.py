"""Tests for dump_gamesystem_weapon_catalog_offline.py."""
import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOL = Path(__file__).resolve().parent / "dump_gamesystem_weapon_catalog_offline.py"


class TestGameSystemWeaponCatalogOffline(unittest.TestCase):
    def test_ordinary_weapon_indices(self):
        proc = subprocess.run(
            [sys.executable, str(TOOL)],
            capture_output=True,
            text=True,
            cwd=ROOT,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        report = json.loads(proc.stdout)
        o = report["ordinary_weapons_inferred"]
        self.assertEqual(o["wp_SP_ARPistol"]["catalog_index"], 0)
        self.assertEqual(o["wp_SP_ARPistol"]["native_mask"], "0x1")
        self.assertEqual(o["wp_SP_ARMachineGun"]["catalog_index"], 1)
        self.assertEqual(o["wp_SP_ARMachineGun"]["native_mask"], "0x2")
        self.assertEqual(o["wp_SP_ARShotgun"]["catalog_index"], 2)
        self.assertEqual(o["wp_SP_ARShotgun"]["native_mask"], "0x4")
        self.assertEqual(o["wp_SP_ARSniperRifle"]["catalog_index"], 5)
        self.assertEqual(o["wp_SP_ARSniperRifle"]["native_mask"], "0x20")
        self.assertEqual(len(report["catalog_rows_by_blob_path_order"]), 9)


if __name__ == "__main__":
    unittest.main()
