import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class RifleReloadCandidateConfigTests(unittest.TestCase):
    def test_candidate_enables_shared_layout_and_all_three_weapons(self):
        source = (ROOT / "tools/reload/prepare_vr_settings_rifle_test.ps1").read_text(
            encoding="utf-8"
        )
        for key in (
            "ReloadControlLayout",
            "ValkyrieManualReload",
            "VolkManualReload",
            "KasimovManualReload",
            "ReloadRefillOnly",
            "ReloadNativeMagazine",
        ):
            self.assertIn(f"'Input/{key}'='1'", source, key)

    def test_rifle_reload_fsm_requires_shared_layout(self):
        source = (ROOT / "spikes/view_matrix/d3d9.cpp").read_text(encoding="utf-8")
        self.assertIn(
            "g_nextReload[i]=g_reloadControlLayout&&GetPrivateProfileIntA",
            source,
        )
        self.assertIn(
            "UpdateNextReload(reloadLayout&&reloadInputContext",
            source,
        )

    def test_change_plan_omits_unchanged_accepted_keys(self):
        source = (ROOT / "tools/reload/prepare_vr_settings_rifle_test.ps1").read_text(
            encoding="utf-8"
        )
        self.assertIn("[StringComparison]::Ordinal", source)
        self.assertIn("continue", source)

    def test_restore_lease_is_bounded_and_never_kills_the_game(self):
        source = (ROOT / "tools/reload/watch_rifle_runtime_restore.ps1").read_text(
            encoding="utf-8"
        )
        self.assertIn("LaunchWindowSeconds=600", source)
        self.assertIn("deploy_next_weapon_test.ps1", source)
        self.assertIn("-Action Rollback", source)
        self.assertNotIn("Stop-Process", source)
        self.assertNotIn("Start-Process", source)


if __name__ == "__main__":
    unittest.main()
