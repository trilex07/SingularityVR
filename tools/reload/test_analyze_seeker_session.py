import unittest

from analyze_seeker_session import analyze


LAUNCH = ("Seeker launch: native archetype=0010 origin=(10.000 20.000 30.000) "
    "direction=(0.00000 1.00000 0.00000) barrel=(0.00000 1.00000 0.00000) "
    "expectedOrigin=(10.000 20.000 30.000); native autoaim/guidance retained")
RESULT = ("Seeker projectile result: object=0020 locationValid=1 velocityValid=1 rotationValid=1 "
    "location=(10.000 20.000 30.000) velocity=(0.00000 5000.00000 0.00000) rotation=0/0/0 "
    "inputOrigin=(10.000 20.000 30.000) inputDirection=(0.00000 1.00000 0.00000)")


class SeekerEvidenceTests(unittest.TestCase):
    def test_no_seeker_is_not_pass(self):
        report = analyze("Seeker back gate: stage=no-owned-seeker pawn=123 count=2")
        self.assertFalse(report["coverage"]["native_selection_equipped_observed"])
        self.assertEqual(report["launches"], [])

    def test_ready_is_neither_grip_nor_equip(self):
        report = analyze("Seeker back gate: stage=ready\n"
            "Seeker back grip: gate=outside availablePawn=00AB distance=0.588 radius=0.160")
        self.assertTrue(report["coverage"]["owned_and_hook_ready_observed"])
        self.assertFalse(report["coverage"]["accepted_back_zone_edge_observed"])
        self.assertFalse(report["coverage"]["native_selection_equipped_observed"])
        self.assertEqual(report["grip_distances"][0]["distance_m"], .588)

    def test_request_is_not_equipped(self):
        report = analyze("Seeker back gate: stage=selection-requested\n"
            "Seeker back gate: stage=selection-timeout")
        self.assertTrue(report["coverage"]["native_selection_requested_observed"])
        self.assertFalse(report["coverage"]["native_selection_equipped_observed"])

    def test_separate_retention_and_equip(self):
        report = analyze("Seeker back slot: retained owned item=0010\n"
            "Seeker back gate: stage=selection-equipped")
        self.assertTrue(report["coverage"]["native_retention_hook_observed"])
        self.assertTrue(report["coverage"]["native_selection_equipped_observed"])

    def test_native_post_spawn_vectors(self):
        report = analyze(LAUNCH + "\n" + RESULT)
        result = report["native_results"][0]
        self.assertEqual(result["spawn_location_error_uu"], 0)
        self.assertEqual(result["initialized_speed_uu_per_s"], 5000)
        self.assertEqual(result["initialized_velocity_vs_input_deg"], 0)
        self.assertEqual(result["launch_line"], 1)

    def test_pre_spawn_alone_never_proves_native_launch(self):
        report = analyze(LAUNCH)
        self.assertFalse(report["coverage"]["post_native_initialized_launch_observed"])
        self.assertEqual(report["unpaired_launch_lines"], [1])

    def test_null_spawn_and_mismatched_result_do_not_inherit(self):
        report = analyze(LAUNCH + "\nSeeker projectile result: object=00000000 no readable native Projectile")
        self.assertFalse(report["coverage"]["post_native_initialized_launch_observed"])
        report = analyze(LAUNCH + "\n" + RESULT.replace("inputOrigin=(10.000", "inputOrigin=(11.000"))
        self.assertIsNone(report["native_results"][0]["launch_line"])

    def test_orphan_result_never_paired_with_future_launch(self):
        report = analyze(RESULT + "\n" + LAUNCH)
        self.assertFalse(report["coverage"]["post_native_initialized_launch_observed"])
        self.assertEqual(report["unpaired_launch_lines"], [2])

    def test_spread_does_not_become_aim_failure(self):
        report = analyze(LAUNCH.replace("barrel=(0.00000 1.00000 0.00000)",
            "barrel=(0.10000 0.99500 0.00000)") + "\n" + RESULT)
        self.assertGreater(report["launches"][0]["native_spread_from_barrel_deg"], 5)
        self.assertNotIn("accuracy_pass", report)

    def test_eye_count_and_state_restore_are_required(self):
        template = "Seeker glove completed: frame=7 eyes={} draw=00000000 ibRestore={} paletteRestore=00000000"
        for eyes, hr in ((1, "00000000"), (2, "8876086C")):
            self.assertFalse(analyze(template.format(eyes, hr))["coverage"]["two_eye_glove_submission_observed"])
        report = analyze(template.format(2, "00000000"))
        self.assertTrue(report["coverage"]["two_eye_glove_submission_observed"])
        self.assertIn("PENDING", report["coverage"]["physical_visibility_and_impact"])


if __name__ == "__main__":
    unittest.main()
