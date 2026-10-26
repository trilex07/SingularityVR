"""Unit tests for final audio campaign resolution."""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
RESOLUTION_PATH = ROOT / "docs/handoffs/final-audio-campaign-resolution-20261026.json"
PLAN_PATH = ROOT / "docs/handoffs/final-audio-campaign-plan-20261026.json"


class CampaignResolutionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(RESOLUTION_PATH, "r", encoding="utf-8") as f:
            cls.resolution = json.load(f)
        with open(PLAN_PATH, "r", encoding="utf-8") as f:
            cls.plan = json.load(f)

    def test_schema_and_approval_flags(self):
        self.assertEqual(self.resolution["schema"], "singularityvr-audio-approval-v1")
        self.assertTrue(self.resolution["listening_approved"])
        self.assertTrue(self.resolution["rights_approved"])
        self.assertTrue(bool(self.resolution["approved_by"]))
        self.assertTrue(bool(self.resolution["listening_evidence"]))
        self.assertTrue(bool(self.resolution["rights_evidence"]))

    def test_counts_and_completeness(self):
        summary = self.resolution["summary"]
        self.assertEqual(summary["total_selected_takes"], 1258)
        self.assertEqual(summary["story_takes"], 1155)
        self.assertEqual(summary["skipped_takes"], 97)
        self.assertEqual(summary["vignette_takes"], 6)
        self.assertEqual(summary["resolved_choices"], 103)
        self.assertEqual(summary["uncovered_rows_retained_native"], 5)
        self.assertEqual(summary["unmatched_excluded_artifacts"], 1)

    def test_all_103_choices_resolved_to_curated(self):
        resolved = self.resolution["resolved_choices"]
        self.assertEqual(len(resolved), 103)
        for choice in resolved:
            sel = choice["selected_take"]
            self.assertIn(sel["pack"], ("skipped", "vignette"))
            rej = choice["rejected_take"]
            self.assertTrue(rej["id"].startswith("story/"))

    def test_no_duplicate_identities(self):
        seen = set()
        for take in self.resolution["selected"]:
            key = (take["package"].casefold(), take["asset"])
            self.assertNotIn(key, seen)
            seen.add(key)
        self.assertEqual(len(seen), 1258)

    def test_all_5_uncovered_rows_retained_native(self):
        uncovered = self.resolution["uncovered_lookup_rows"]
        self.assertEqual(len(uncovered), 5)
        for row in uncovered:
            self.assertEqual(row["disposition"], "RETAIN_ACCEPTED_NATIVE")

    def test_unmatched_temporary_artifact_excluded(self):
        unmatched = self.resolution["unmatched"]
        self.assertEqual(len(unmatched), 1)
        self.assertEqual(unmatched[0]["id"], "story/ae99b20311bcf364.wav.tmp.wav")
        self.assertEqual(unmatched[0]["disposition"], "EXCLUDE_TEMPORARY_ARTIFACT")


if __name__ == "__main__":
    unittest.main()
