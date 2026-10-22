"""Batch success accounting for locker VR loadout acquire (must match acquire.h)."""
import unittest


def batch_outcome(pending_now, pickup_called, owned_before, owned_after, snap):
    success = (pickup_called & owned_after & ~owned_before) if snap else 0
    not_called = pending_now & ~pickup_called
    called_but_missing = pickup_called & ~owned_after
    failed = pending_now & ~success
    return success, not_called, called_but_missing, failed


class AcquireBatchAccounting(unittest.TestCase):
    def test_physical_false_positive_case(self):
        success, not_called, called_but_missing, failed = batch_outcome(
            pending_now=0xE,
            pickup_called=0,
            owned_before=0x1,
            owned_after=0x3,
            snap=True,
        )
        self.assertEqual(success, 0)
        self.assertEqual(not_called, 0xE)
        self.assertEqual(called_but_missing, 0)
        self.assertEqual(failed, 0xE)

    def test_pickup_called_zero_implies_success_zero(self):
        for owned_after in (0, 1, 3, 0xF):
            success, _, _, _ = batch_outcome(0xE, 0, 0x1, owned_after, True)
            self.assertEqual(success, 0)

    def test_success_subset_of_pickup_called(self):
        pending, called, before, after = 0xE, 0xA, 0x1, 0xB
        success, _, _, _ = batch_outcome(pending, called, before, after, True)
        self.assertEqual(success, 0xA)
        self.assertEqual(success & ~called, 0)

    def test_cpp_source_uses_pickup_called_for_success(self):
        from pathlib import Path

        acquire = (Path(__file__).resolve().parents[2] / "spikes/view_matrix/locker_vr_loadout_acquire.h").read_text(
            encoding="utf-8"
        )
        compact = acquire.replace(" ", "")
        self.assertIn("pickupCalled&ownedAfter&~ownedBefore", compact)
        self.assertIn("notCalled=0x%X", acquire)
        self.assertIn("calledButMissing=0x%X", acquire)
        self.assertNotIn("pendingNow&ownedAfter&~ownedBefore", compact)


if __name__ == "__main__":
    unittest.main()
