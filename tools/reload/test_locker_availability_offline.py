"""Offline tests for locker availability contract tool."""
import json
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
TOOL = REPO / "tools/reload/inspect_locker_availability_offline.py"
EXPORT = REPO.parent / "research/etw/controller-features-20261006/rvgame-assets.txt"
if not EXPORT.is_file():
    EXPORT = Path(__file__).resolve().parents[3] / "research/etw/controller-features-20261006/rvgame-assets.txt"
TEXT = REPO.parent / "research/etw/capture-20261006-205508/Singularity.text-memory.bin"
EXE = Path(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\Singularity.exe")


class LockerAvailabilityOffline(unittest.TestCase):
    def test_inspect_emits_game_system_path(self):
        if not EXPORT.is_file() or not TEXT.is_file() or not EXE.is_file():
            self.skipTest("capture, export or exe missing")
        proc = subprocess.run(
            [
                sys.executable,
                str(TOOL),
                "--exe",
                str(EXE),
                "--text",
                str(TEXT),
                "--export",
                str(EXPORT),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        data = json.loads(proc.stdout)
        self.assertEqual(data["globals"]["GGameSystem"], "0x01CF394C")
        self.assertEqual(data["export"]["m_weapons_owner_class"], "RvGameSystem")
        self.assertEqual(data["native_bodies"]["equip_slot_write"], "0x00F00F50")
        load_va = int(data["weapon_locker_vtable_0x019A0948"]["LoadEquippedWeapons"]["va"], 16)
        self.assertEqual(load_va, 0xF23840)


if __name__ == "__main__":
    unittest.main()
