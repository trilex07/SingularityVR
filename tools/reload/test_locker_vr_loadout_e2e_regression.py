"""Regression tests after PSVR2 E2E partial (UI scale, focus init, factory gate)."""
import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
LOADOUT_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout.h"
LAYOUT_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout_layout.h"
IDENTITY_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout_identity.h"
NATIVE_UI = REPO / "spikes" / "view_matrix" / "native_ui_input.h"
D3D9 = REPO / "spikes" / "view_matrix" / "d3d9.cpp"


class E2eRegression(unittest.TestCase):
    def test_weapon_locker_focus_initializes_equip_page(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertIn("void OnFocus(", body)
        self.assertIn("InterlockedExchange(&g_pageSignal, 1)", body.split("OnFocus", 1)[1].split("OnDefocus", 1)[0])

    def test_native_ui_calls_loadout_on_focus(self):
        native = NATIVE_UI.read_text(encoding="utf-8")
        block = native.split('RvUIGFxMovie_WeaponLocker"', 1)[1].split("NewGame:", 1)[0]
        self.assertIn("LockerVrLoadout::OnFocus", block)

    def test_defocus_clears_page_and_publish(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        defocus = body.split("OnDefocus()", 1)[1].split("OnFocus", 1)[0]
        self.assertIn("g_pageSignal", defocus)
        self.assertIn("Publish::Clear", defocus)
        self.assertNotIn("g_acquireRequested", defocus)
        self.assertNotIn("g_postPopAcquireMask", defocus)

    def test_deferred_acquire_consumes_post_pop_request(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        deferred = body.split("TryRunDeferredAcquire()", 1)[1].split("inline void OnDefocus", 1)[0]
        self.assertIn("g_acquireRequested", deferred)
        self.assertIn("g_postPopAcquireMask", deferred)
        self.assertIn("ApplyPendingMask", deferred)
        schedule = body.split("OnPopMenuScheduleAcquire()", 1)[1].split("TryRunDeferredAcquire", 1)[0]
        self.assertIn("g_acquireRequested", schedule)
        self.assertIn("g_postPopAcquireMask", schedule)

    def _draw_body(self):
        text = D3D9.read_text(encoding="utf-8", errors="replace")
        marker = "static void DrawLockerVrLoadout(IDirect3DDevice9* dev) {"
        return text.rsplit(marker, 1)[1].split("void DrawWeaponWheel", 1)[0]

    def test_font_scale_bounded_not_backbuffer_height(self):
        draw = self._draw_body()
        self.assertIn("LockerVrLoadout::Layout::kFontPx", draw)
        self.assertNotIn("d.Height * .016f", draw)
        self.assertNotIn("d.Height * .034f", draw)

    def test_queued_button_label_distinct_from_equip_all(self):
        draw = self._draw_body()
        compact = re.sub(r"\s+", "", draw)
        self.assertIn('uiQueued?"QUEUED":"EQUIPALL"', compact)
        self.assertIn("ALL OWNED", draw)

    def test_gamesystem_vtable_rva_from_image_base(self):
        ident = IDENTITY_H.read_text(encoding="utf-8")
        self.assertIn("kExpectedRvGameSystemVtableRvaFromImage", ident)
        self.assertIn("ImageBase()", ident)
        self.assertIn("FactoryIdentityOk", ident.split("GameSystemObjectIdentityOk", 1)[1])
        self.assertNotIn("ModuleBaseFromTextLo", ident)
        self.assertNotIn("kExpectedRvGameSystemVtableOffsetFromCaptureText", ident)

    def test_focus_requests_owned_refresh(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        focus = body.split("OnFocus", 1)[1].split("struct ExecHooks", 1)[0]
        self.assertIn("NativeOwnedInventory::Refresh()", focus)
        self.assertIn("NativeOwnedInventory::SnapshotNow()", focus)

    def test_snapshot_clears_valid_at_start(self):
        inv = (REPO / "spikes" / "view_matrix" / "native_owned_inventory.h").read_text(encoding="utf-8")
        snap = inv.split("static bool Snapshot()", 1)[1].split("static void Report", 1)[0]
        self.assertIn("InterlockedExchange(&snapshotValid,0)", snap.replace(" ", ""))

    def test_native_local_player_pawn_fallback(self):
        inv = (REPO / "spikes" / "view_matrix" / "native_owned_inventory.h").read_text(encoding="utf-8")
        self.assertIn("EngineNativeLocalPlayerPawn()", inv)

    def test_ready_transition_log(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertIn("locker vr loadout ready:", body)
        self.assertIn("EngineLoadoutNotReadyReason", body)
        runtime = (REPO / "spikes/view_matrix/locker_vr_loadout_runtime.h").read_text(encoding="utf-8")
        self.assertIn("AvailabilityBlockReason", runtime)
        avail = runtime.split("AvailabilityBlockReason", 1)[1].split("AvailabilitySourcesReady", 1)[0]
        self.assertIn("SnapshotBoundManager", avail)

    def test_availability_manager_uses_image_relative_vtable(self):
        runtime = (REPO / "spikes/view_matrix/locker_vr_loadout_runtime.h").read_text(encoding="utf-8")
        self.assertIn("ExpectedInventoryManagerVtable", runtime)
        self.assertIn("RelocatedDataAddress", runtime)

    def test_snapshot_manager_exposed(self):
        inv = (REPO / "spikes/view_matrix/native_owned_inventory.h").read_text(encoding="utf-8")
        self.assertIn("SnapshotManager()", inv)
        self.assertIn("SnapshotValid()?manager:0", inv.replace(" ", ""))

    def test_ready_and_native_mask_share_snapshot_manager(self):
        runtime = (REPO / "spikes/view_matrix/locker_vr_loadout_runtime.h").read_text(encoding="utf-8")
        self.assertIn("SnapshotBoundManager", runtime)
        avail = runtime.split("AvailabilityBlockReason", 1)[1].split("AvailabilitySourcesReady", 1)[0]
        mask = runtime.split("ReadNativeProgressionMask", 1)[1].split("AvailabilityBlockReason", 1)[0]
        self.assertIn("SnapshotBoundManager", avail)
        self.assertIn("SnapshotBoundManager", mask)
        self.assertNotIn("ResolveInventoryManager(pawn)", avail)

    def test_acquire_uses_gamesystem_instance_only(self):
        acquire = (REPO / "spikes/view_matrix/locker_vr_loadout_acquire.h").read_text(encoding="utf-8")
        self.assertIn("GameSystemInstance()", acquire)
        self.assertNotIn("Pointer(LockerVrLoadoutRuntime::kGGameSystemPtr)", acquire)
        self.assertNotIn("kGGameSystemPtr)", acquire.replace(" ", ""))

    def test_runtime_pinned_data_addresses_relocated(self):
        runtime = (REPO / "spikes/view_matrix/locker_vr_loadout_runtime.h").read_text(encoding="utf-8")
        self.assertIn("RelocatedDataAddress(kDefaultEmptyFString)", runtime)
        self.assertNotIn(
            "reinterpret_cast<const void*>(kDefaultEmptyFString)",
            runtime.replace(" ", ""),
        )
        gs_block = runtime.split("GameSystemInstance()", 1)[0]
        self.assertNotIn("Pointer(kGGameSystemPtr)", runtime.split("FStringPtrAtCatalogRowSecondary", 1)[0])

    def test_loading_ui_no_large_panel(self):
        draw = self._draw_body()
        self.assertIn("VR LOADOUT ...", draw)
        idx = draw.index("if (!dataReady)")
        loading = draw[idx : idx + 400]
        self.assertNotIn("0xAA101820", loading)

    def test_panel_clamped_in_eye_space(self):
        text = D3D9.read_text(encoding="utf-8", errors="replace")
        self.assertIn("locker_loadout_layout::ComputeLayout", text)
        self.assertIn("ClampEyePanel", LAYOUT_H.read_text(encoding="utf-8"))

    def test_render_and_hit_share_layout_helper(self):
        text = D3D9.read_text(encoding="utf-8", errors="replace")
        self.assertIn("LockerLoadoutBuildLayoutForEye", text)
        self.assertIn("LockerLoadoutScreen::HitCanvas", text)
        draw = self._draw_body()
        self.assertIn("LockerLoadoutBuildLayoutForEye", draw)

    def test_uv_hit_constants_not_authoritative(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertNotIn("HitEquipAll", body)
        self.assertNotIn("HitTileIndex", body)
        self.assertNotIn("kBtnU", body)
        self.assertNotIn("kCu", body)
        self.assertIn("LockerLoadoutScreen::HitCanvas", body)

    def test_hit_canvas_uses_screen_projection(self):
        text = D3D9.read_text(encoding="utf-8", errors="replace")
        block = text.split("LockerLoadoutScreen::HitCanvas", 1)[1].split("static void DrawLockerVrLoadout", 1)[0]
        self.assertIn("ProjectCanvasUvToScreen", block)
        self.assertIn("HitTestPointer", block)

    def test_can_interact_gated_on_data_ready(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        pub = body.split("PublishFromEngine", 1)[1].split("OnPopMenuScheduleAcquire", 1)[0]
        self.assertIn("EngineLoadoutPawn", pub)
        self.assertNotIn("g_playerPawn", pub)
        self.assertIn("Publish(0, 0, 0, false, uiQueued, false)", pub)

    def test_loadout_uses_snapshot_pawn_not_global(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertIn("EngineLoadoutPawn()", body)
        self.assertIn("NativeOwnedInventory::SnapshotPawn()", body)
        ready = body.split("EngineLoadoutDataReady", 1)[1].split("PublishFromEngine", 1)[0]
        self.assertNotIn("g_playerPawn", ready)

    def test_deferred_acquire_uses_snapshot_pawn(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        deferred = body.split("TryRunDeferredAcquire()", 1)[1].split("inline void OnDefocus", 1)[0]
        self.assertIn("EngineLoadoutPawn()", deferred)
        self.assertNotIn("g_playerPawn", deferred)

    def test_snapshot_valid_separate_from_ready(self):
        inv = (REPO / "spikes" / "view_matrix" / "native_owned_inventory.h").read_text(encoding="utf-8")
        self.assertIn("snapshotValid", inv)
        self.assertIn("InterlockedExchange(&snapshotValid,1)", inv.replace(" ", ""))
        self.assertIn("Ready(){returnMask()!=0;}", inv.replace(" ", ""))
        self.assertIn("SnapshotPawn()", inv)
        self.assertIn("SnapshotValid(){returnInterlockedCompareExchange(&snapshotValid", inv.replace(" ", ""))

    def test_tile_geometry_no_overlap(self):
        layout = LAYOUT_H.read_text(encoding="utf-8")
        compact = layout.replace(" ", "")
        self.assertIn("kIconHalfPx=42.f", compact)
        self.assertIn("kTileSpacingXPx=110.f", compact)
        self.assertIn("kTileSpacingYPx=105.f", compact)
        self.assertGreater(110, 2 * 42)
        self.assertGreater(105, 2 * 42)

    def test_panel_anchor_uses_accepted_adjacent_projection(self):
        layout = LAYOUT_H.read_text(encoding="utf-8")
        # The accepted visibility fix replaced the old absolute 0.93 anchor.
        # Actual stereo/hit geometry is exercised by the C++ layout fixture.
        self.assertIn("ProjectAdjacentAnchor(", layout)
        self.assertIn("kNativeMenuGapPx", layout)
        self.assertIn("else return false", layout)
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertNotIn("kCu = 0.84f", body.replace(" ", ""))
        self.assertNotIn("kCu=", body.split("namespace Layout", 1)[1].split("} // namespace Layout", 1)[0])

    def test_draw_shows_loading_without_data_ready(self):
        draw = self._draw_body()
        self.assertIn("DataReady()", draw)
        self.assertIn("IsEquipPage()", draw)
        self.assertIn("VR LOADOUT ...", draw)

    def test_load_upgrades_and_load_hooks_preserved(self):
        body = LOADOUT_H.read_text(encoding="utf-8")
        self.assertIn("g_pageSignal, 2", body)
        self.assertIn("g_pageSignal, 1", body)


if __name__ == "__main__":
    unittest.main()
