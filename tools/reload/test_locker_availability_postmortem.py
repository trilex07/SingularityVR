"""Offline policy tests for availability witness postmortem tools."""
import json
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
TOOLS = REPO / "tools" / "reload"
DOC = REPO / "docs" / "weapon-locker-availability-witness-postmortem-20261022.md"
WITNESS_H = REPO / "spikes" / "view_matrix" / "locker_availability_trace.h"


class PostmortemOffline(unittest.TestCase):
    def test_fd5ac0_tool_runs(self):
        out = subprocess.check_output([sys.executable, str(TOOLS / "analyze_catalog_row_fd5ac0.py")], text=True)
        data = json.loads(out)
        self.assertIn("FString primary identity", data["runtime_fields_proven"]["+0x00/+0x04"])
        self.assertTrue(data["prologue_checks"]["stride_0x50_add_edi"])

    def test_mweapons_decode_ordinary_indices(self):
        out = subprocess.check_output([sys.executable, str(TOOLS / "decode_gamesystem_mweapons_default.py")], text=True)
        data = json.loads(out)
        chk = data["ordinary_index_check"]
        self.assertEqual(chk["wp_SP_ARPistol"]["expected_index"], 0)
        self.assertEqual(chk["wp_SP_ARSniperRifle"]["expected_index"], 5)
        self.assertEqual(data["default_array_count"], 17)

    def test_preview_navigation_classifies_equip(self):
        out = subprocess.check_output(
            [sys.executable, str(TOOLS / "inspect_locker_preview_navigation.py")], text=True
        )
        data = json.loads(out)
        self.assertEqual(data["equip_at_f00f50"]["classification"].split()[0], "COMMIT")
        self.assertEqual(data["equip_at_f00f50"]["direct_call_sites_in_text"], [])

    def test_doc_and_witness_correction(self):
        self.assertTrue(DOC.is_file())
        doc = DOC.read_text(encoding="utf-8")
        self.assertIn("INCONCLUSIVE", doc)
        self.assertIn("equip @ 0xF00F50", doc)
        self.assertIn("4409F0", doc)
        self.assertIn("ReadFStringAscii(row+0)", doc)


if __name__ == "__main__":
    unittest.main()
