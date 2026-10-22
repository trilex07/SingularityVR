"""Tests for WeaponLocker GFx offline scan."""
import json
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
TOOL = REPO / "tools/reload/inspect_weapon_locker_gfx.py"
UI = Path(r"C:\Users\Trilex\vr-mods\pause-re-lab\UI_SinglePlayer_SF.u")
RV = Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\RvGame.decompressed.xxx")


class WeaponLockerGfxScan(unittest.TestCase):
    def test_hud_weapon_names_map_present(self):
        if not UI.is_file() or not RV.is_file():
            self.skipTest("offline packages missing")
        proc = subprocess.run(
            [sys.executable, str(TOOL), "--ui-u", str(UI), "--rvgame", str(RV)],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        data = json.loads(proc.stdout)
        hud = next(m for m in data["movies"] if m["label"] == "RvGame/HUD")
        self.assertIn("constants.WeaponNames", hud["needle_hits"])
        self.assertIn("armachinegun", hud["needle_hits"])
        self.assertNotIn("LoadEquippedWeapons", hud["needle_hits"])


if __name__ == "__main__":
    unittest.main()
