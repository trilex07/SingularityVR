"""Policy checks for locker_availability_trace.h (no game runtime)."""
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
HEADER = REPO / "spikes" / "view_matrix" / "locker_availability_trace.h"
D3D9 = (REPO / "spikes" / "view_matrix" / "d3d9.cpp").read_text(encoding="utf-8", errors="replace")
UI = (REPO / "spikes" / "view_matrix" / "native_ui_input.h").read_text(encoding="utf-8", errors="replace")


class LockerAvailabilityTracePolicy(unittest.TestCase):
    def setUp(self):
        self.text = HEADER.read_text(encoding="utf-8", errors="replace")

    def test_separate_from_locker_native_trace(self):
        self.assertNotIn("locker_native_trace.h", self.text)
        self.assertNotIn("namespace LockerNativeTrace", self.text)
        self.assertNotIn("LoadUpgrades", self.text)
        self.assertNotIn("PopMenu", self.text)
        self.assertNotIn("g_origStart", self.text)

    def test_minimal_equip_hook_only(self):
        self.assertIn("kFnEquip = 0x00F00F50u", self.text)
        self.assertIn("using EquipFn", self.text)
        self.assertIn("ret 0x0C", self.text)
        self.assertEqual(self.text.count("MH_CreateHook(reinterpret_cast<void*>(kFnEquip)"), 1)

    def test_read_only_no_mutations(self):
        forbidden = (
            "0xFD5BA0",
            "0xFD5AC0",
            "EnableAllWeaponsForLocker",
            "NativeOwnedInventory::Poll",
            "NativeOwnedInventory::Refresh",
            "MH_CreateHook(reinterpret_cast<void*>(kFnStart",
            "LoadUpgrades",
            "Upgrade",
        )
        for token in forbidden:
            self.assertNotIn(token, self.text, token)

    def test_catalog_and_mask_offsets(self):
        self.assertIn("0x01CF394C", self.text)
        self.assertIn("kCatalogStride = 0x50u", self.text)
        self.assertIn("kMgrNativeMaskOff = 0x3E4u", self.text)
        self.assertIn("kPawnInvManager = 0x3D0u", self.text)

    def test_catalog_row24_byte_read(self):
        self.assertIn("uint8_t row24", self.text)
        self.assertIn("SafeRange(row + 0x24, 1)", self.text)
        self.assertIn("reinterpret_cast<const uint8_t*>(row + 0x24)", self.text)
        self.assertNotIn("reinterpret_cast<const int*>(row + 0x24)", self.text)

    def test_inventory_manager_identity_before_mask(self):
        self.assertIn("kInventoryManagerVtable = 0x019CB480u", self.text)
        self.assertIn("inventory manager identity refused", self.text)
        self.assertIn("g_managerValid", self.text)
        self.assertIn("nativeProgressionOrdinary=UNKNOWN", self.text)
        # +0x3E4 read must not precede vtable validation in ResolveInventoryManager
        resolve = self.text.split("inline uintptr_t ResolveInventoryManager", 1)[1].split(
            "inline void ResetOrdinaryRows", 1
        )[0]
        self.assertIn("kInventoryManagerVtable", resolve)
        self.assertNotIn("kMgrNativeMaskOff", resolve)
        open_body = self.text.split("inline void LogSessionOpen", 1)[1].split("inline void LogSessionClose", 1)[0]
        self.assertIn("ResolveInventoryManager(true)", open_body)
        self.assertLess(open_body.index("ResolveInventoryManager"), open_body.index("kMgrNativeMaskOff"))

    def test_log_prefixes(self):
        self.assertIn("locker avail: path=", self.text)
        self.assertIn("locker avail equip:", self.text)
        self.assertIn("locker avail summary:", self.text)

    def test_ini_and_ui_wiring(self):
        self.assertIn("LockerAvailabilityTrace", D3D9)
        self.assertIn("g_lockerAvailabilityTrace", D3D9)
        self.assertIn("LockerAvailabilityTrace::OnFocus", UI)
        self.assertIn("LockerAvailabilityTrace::OnDefocus", UI)


if __name__ == "__main__":
    unittest.main()
