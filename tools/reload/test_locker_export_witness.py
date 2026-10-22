"""Offline UE3 export checks for WeaponLocker audit (rvgame asset list)."""
import unittest
from pathlib import Path

EXPORT = Path(__file__).resolve().parents[2].parent / "research" / "etw" / "controller-features-20261006" / "rvgame-assets.txt"
# repo root is SingularityVR-title-menu-fix; research lives in vr-mods sibling
if not EXPORT.is_file():
    EXPORT = Path(__file__).resolve().parents[3] / "research" / "etw" / "controller-features-20261006" / "rvgame-assets.txt"


class LockerExportWitness(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not EXPORT.is_file():
            raise unittest.SkipTest(f"missing export inventory: {EXPORT}")
        cls.text = EXPORT.read_text(encoding="utf-8", errors="replace")

    def test_weapon_locker_class_and_execs(self):
        self.assertIn("Class RvUIGFxMovie_WeaponLocker", self.text)
        for sym in (
            "Function LoadEquippedWeapons",
            "Function LoadUpgrades",
            "Function equip",
            "Function Upgrade",
            "Function PopMenu",
            "Function Start",
            "ArrayProperty mEquippedWeapons",
            "ArrayProperty mOriginalWeapons",
            "IntProperty mActiveWeaponSlot",
            "IntProperty mRequestedPackages",
        ):
            self.assertIn(sym, self.text, sym)

    def test_no_obvious_page_bool_on_locker_block(self):
        # Export lists WeaponLocker members in a contiguous block before the class line.
        start = self.text.find("NameProperty mOriginalWeapons")
        end = self.text.find("Class RvUIGFxMovie_WeaponLocker")
        self.assertGreater(end, start)
        block = self.text[start:end]
        self.assertNotIn("UpgradePage", block)
        self.assertNotIn("EquipPage", block)
        self.assertNotIn("mCurrentTab", block)

    def test_weapons_available_to_locker_on_inventory_item(self):
        idx = self.text.find("IntProperty mWeaponsAvailableToLocker")
        self.assertGreater(idx, 0)
        window = self.text[max(0, idx - 400) : idx + 200]
        self.assertIn("Class RvInventoryItem", window)

    def test_session_manager_weapons_array(self):
        self.assertIn("ArrayProperty mWeapons", self.text)
        self.assertIn("ScriptStruct WeaponInfo", self.text)


if __name__ == "__main__":
    unittest.main()
