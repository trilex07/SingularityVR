"""Static/threading contract tests for locker VR loadout hardening."""
import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
D3D9 = REPO / "spikes" / "view_matrix" / "d3d9.cpp"
LOADOUT_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout.h"
ACQUIRE_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout_acquire.h"
PUBLISH_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout_publish.h"
NATIVE_UI = REPO / "spikes" / "view_matrix" / "native_ui_input.h"


class HardeningContract(unittest.TestCase):
    def test_draw_uses_published_masks_only(self):
        text = D3D9.read_text(encoding="utf-8", errors="replace")
        marker = "static void DrawLockerVrLoadout(IDirect3DDevice9* dev) {"
        draw = text.rsplit(marker, 1)[1].split("void DrawWeaponWheel", 1)[0]
        self.assertIn("LockerVrLoadoutPublish::Available()", draw)
        self.assertIn("LockerVrLoadoutPublish::Owned()", draw)
        self.assertNotIn("AvailableOrdinaryMask", draw)
        self.assertNotIn("AvailableMask()", draw)

    def test_xr_menu_uses_published_and_consumes_click(self):
        text = D3D9.read_text(encoding="utf-8", errors="replace")
        block = text.split("if(uiMenu) {", 1)[1].split("} else {", 1)[0]
        self.assertIn("LockerVrLoadoutPublish::CanInteract()", block)
        self.assertIn("lockerConsumed", block)
        self.assertIn("selectPressed&&!lockerConsumed", block.replace(" ", ""))
        self.assertNotIn("AvailableMask", block)

    def test_popmenu_schedules_not_mutates(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertIn("OnPopMenuScheduleAcquire", body)
        self.assertIn("g_acquireRequested", body)
        self.assertNotIn("ApplyPendingMask", body.split("OnPopMenuScheduleAcquire", 1)[1].split("TryRunDeferredAcquire", 1)[0])

    def test_deferred_acquire_on_poll_engine(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertIn("TryRunDeferredAcquire", body)
        self.assertIn("PollEngine", body)
        self.assertIn("g_vrUiWeaponLocker", body)
        native = NATIVE_UI.read_text(encoding="utf-8")
        self.assertIn("LockerVrLoadout::PollEngine()", native)

    def test_archetype_class_has_not_strcmp_exact(self):
        body = ACQUIRE_H.read_text(encoding="utf-8")
        self.assertIn("ClassHas(cls, \"RvWeaponShared\")", body)
        self.assertNotIn("strcmp(name, \"RvWeaponShared\")", body)

    def test_batch_snapshot_now(self):
        body = ACQUIRE_H.read_text(encoding="utf-8")
        self.assertIn("SnapshotNow()", body)
        self.assertIn("acquire batch:", body)
        self.assertNotIn("pickup=success", body)

    def test_factory_identity_pinned(self):
        body = ACQUIRE_H.read_text(encoding="utf-8")
        self.assertIn("AppendFNameIdentityOk", body)
        self.assertIn("FactoryIdentityOk", body)
        self.assertNotIn("F69590", body)

    def test_tile_queue_or_semantics(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertIn("prev |=", body)
        self.assertIn("unqueued", body)

    def test_publish_header_exists(self):
        self.assertTrue(PUBLISH_H.is_file())
        pub = PUBLISH_H.read_text(encoding="utf-8")
        self.assertIn("g_publishedAvailableMask", pub)

    def test_derived_weapon_class_policy(self):
        def class_has(chain, needle):
            return any(needle in part for part in chain)

        self.assertTrue(class_has(["RvWeaponShared_Shotgun", "RvWeaponShared"], "RvWeaponShared"))
        self.assertFalse(class_has(["RvWeaponHeavy"], "RvWeaponShared"))


if __name__ == "__main__":
    unittest.main()
