"""Regression: GGameSystem resolution matches native GetGameSystem and factory identity gate."""
from __future__ import annotations

import struct
import unittest
from pathlib import Path

import capstone

REPO = Path(__file__).resolve().parents[2]
TEXT_PATH = REPO.parent / "research" / "etw" / "capture-20261006-205508" / "Singularity.text-memory.bin"
RUNTIME_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout_runtime.h"
IDENTITY_H = REPO / "spikes" / "view_matrix" / "locker_vr_loadout_identity.h"

IMAGE_BASE = 0x400000
G_GAME_SYSTEM = 0x01CF394C
GET_GAME_SYSTEM_FN = 0xEFA070
VTABLE_VA = 0x019CDBF8
FACTORY_VA = 0x00FD2320
FACTORY_FINGERPRINT = 0xF2B5EBD13E8B220F


class GameSystemResolution(unittest.TestCase):
    def test_runtime_uses_native_getter_not_raw_global_deref(self):
        runtime = RUNTIME_H.read_text(encoding="utf-8")
        self.assertIn("kFnGetGameSystem", runtime)
        self.assertIn("GetGameSystemFn", runtime)
        self.assertNotIn(
            "return PauseRttProbe::Pointer(RelocatedDataAddress(kGGameSystemPtr))",
            runtime.replace(" ", ""),
        )

    def test_identity_uses_factory_slot_and_vtable_verification(self):
        ident = IDENTITY_H.read_text(encoding="utf-8")
        body = ident.split("GameSystemObjectIdentityOk", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("FactoryIdentityOk", body)
        self.assertIn("kGameSystemFactoryVtableOff", body)
        self.assertIn("kExpectedRvGameSystemVtableRvaFromImage", body)

    def test_availability_uses_catalog_source_not_raw_mutation(self):
        ident = IDENTITY_H.read_text(encoding="utf-8")
        self.assertIn("GameSystemCatalogSourceOk", ident)
        runtime = RUNTIME_H.read_text(encoding="utf-8")
        avail = runtime.split("AvailabilityBlockReason", 1)[1].split("AvailabilitySourcesReady", 1)[0]
        self.assertIn("GameSystemCatalogSourceOk", avail)
        acquire = (REPO / "spikes" / "view_matrix" / "locker_vr_loadout_acquire.h").read_text(encoding="utf-8")
        self.assertIn("GameSystemObjectIdentityOk", acquire)

    def test_psvr2_b3bec91_witness_values_match_pe_vtables(self):
        """2026-10-22 PSVR2 @ b3bec91: live gs vtable was 0x019CDBF8, not pinned 0x01BCF9C4."""
        from inspect_native_reload import PE

        exe_candidates = [
            Path(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\Singularity.exe"),
            REPO.parent / "test-games" / "assets" / "Singularity" / "Binaries" / "Singularity.exe",
        ]
        exe = next((p for p in exe_candidates if p.is_file()), None)
        if exe is None:
            self.skipTest("Singularity.exe missing")
        pe = PE(exe.read_bytes())
        live_vt = 0x019CDBF8
        pinned_vt = 0x01BCF9C4
        live_fn = struct.unpack("<I", pe.read(live_vt + 0x20C, 4))[0]
        pinned_fn = struct.unpack("<I", pe.read(pinned_vt + 0x20C, 4))[0]
        self.assertEqual(live_fn, 0x00FD2320)
        self.assertEqual(pinned_fn, 0x00F85E30)
        self.assertNotEqual(live_vt, pinned_vt)

    def test_native_getgamesystem_loads_relocated_global_immediate(self):
        if not TEXT_PATH.is_file():
            self.skipTest("archived text capture missing")
        text = TEXT_PATH.read_bytes()
        text_base = 0x402000
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        code = text[GET_GAME_SYSTEM_FN - text_base : GET_GAME_SYSTEM_FN - text_base + 8]
        insns = list(md.disasm(code, GET_GAME_SYSTEM_FN))
        self.assertGreaterEqual(len(insns), 1)
        first = insns[0]
        self.assertEqual(first.mnemonic, "mov")
        self.assertIn("eax", first.op_str)
        self.assertIn(hex(G_GAME_SYSTEM), first.op_str)

    def test_offline_vtable_factory_slot_matches_audit(self):
        from inspect_native_reload import PE

        exe_candidates = [
            Path(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\Singularity.exe"),
            REPO.parent / "test-games" / "assets" / "Singularity" / "Binaries" / "Singularity.exe",
        ]
        exe = next((p for p in exe_candidates if p.is_file()), None)
        if exe is None:
            self.skipTest("Singularity.exe not available for vtable slot read")
        pe = PE(exe.read_bytes())
        fn = struct.unpack("<I", pe.read(VTABLE_VA + 0x20C, 4))[0]
        self.assertEqual(fn, FACTORY_VA)


if __name__ == "__main__":
    unittest.main()
