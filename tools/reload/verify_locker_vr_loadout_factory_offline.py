"""Offline proof: GGameSystem vtable+0x20C factory matches audited native chains.

Reads archived Singularity.text-memory.bin + owned PE .rdata. No game launch.
"""
from __future__ import annotations

import hashlib
import struct
import unittest
from pathlib import Path

import capstone

from inspect_native_reload import PE

REPO = Path(__file__).resolve().parents[2]
TEXT_PATH = REPO.parent / "research" / "etw" / "capture-20261006-205508" / "Singularity.text-memory.bin"
EXE_CANDIDATES = [
    REPO.parent / "test-games" / "assets" / "Singularity" / "Binaries" / "Singularity.exe",
    REPO / "build" / "next-weapon-reload" / "asset-lease-fixture-c656fb9b0d1841e3b0e462d4349e9ae9"
    / "steamapps" / "common" / "Singularity" / "Binaries" / "Singularity.exe",
]

TEXT_BASE = 0x402000
IMAGE_BASE = 0x400000
PROLOGUE_BYTES = 16
FNV_OFFSET = 1469598103934665603
FNV_PRIME = 1099511628211

# Offline pins (must match locker_vr_loadout_identity.h)
EXPECTED_FACTORY_VA = 0xF85E30
EXPECTED_FACTORY_RVA = EXPECTED_FACTORY_VA - IMAGE_BASE
EXPECTED_FACTORY_FINGERPRINT = 0x293E6200F76D1A77
EXPECTED_GS_VTABLE_VA = 0x1BCF9C4
GET_FEATURES_VA = 0xF55B30
VTABLE_SLOT_FACTORY = 0x20C
VTABLE_SLOT_GET_FEATURES = 0x1F0

CHAIN_SITES = (
    (0xF03460, "mission_unlock_0xF03450"),
    (0x11B4B7E, "duplicate_pickup_0x11B4B6E"),
)


def fnv1a64_prologue(text: bytes, va: int) -> int:
    off = va - TEXT_BASE
    chunk = text[off : off + PROLOGUE_BYTES]
    h = FNV_OFFSET
    for b in chunk:
        h ^= b
        h = (h * FNV_PRIME) & ((1 << 64) - 1)
    return h


def resolve_exe() -> Path:
    for p in EXE_CANDIDATES:
        if p.is_file():
            return p
    raise FileNotFoundError("Singularity.exe not found for offline factory verify")


def disasm_has_factory_chain(text: bytes, add_va: int) -> bool:
    """At add_va: add reg, 0x20c; call 0x4affd0; load [reg]; indirect call."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    insns = list(md.disasm(text[add_va - TEXT_BASE : add_va - TEXT_BASE + 24], add_va))
    for i, ins in enumerate(insns):
        if ins.address != add_va or ins.mnemonic != "add" or "0x20c" not in ins.op_str:
            continue
        tail = insns[i + 1 : i + 8]
        if len(tail) < 4:
            continue
        if tail[0].mnemonic != "call" or "0x4affd0" not in tail[0].op_str:
            continue
        if tail[1].mnemonic not in ("mov", "movzx"):
            continue
        indirect = next((t for t in tail[2:] if t.mnemonic == "call"), None)
        if not indirect or indirect.op_str not in ("eax", "edx"):
            continue
        return True
    return False


def gate_accepts(text: bytes, image_base: int, live_fn: int) -> bool:
    expected = image_base + EXPECTED_FACTORY_RVA
    if live_fn != expected:
        return False
    if fnv1a64_prologue(text, live_fn) != EXPECTED_FACTORY_FINGERPRINT:
        return False
    return True


class FactoryOfflineProof(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = TEXT_PATH.read_bytes()
        cls.exe = resolve_exe()
        cls.pe = PE(cls.exe.read_bytes())

    def test_archived_text_sha(self):
        digest = hashlib.sha256(self.text).hexdigest()
        self.assertEqual(
            digest,
            "858f2fc33b1c99b3a4a27a817a11205da66414ee1b030a03df821f1ad6a0c78a",
        )

    def test_native_chains_use_vtable_plus_20c_before_append_fname(self):
        for va, label in CHAIN_SITES:
            with self.subTest(chain=label):
                self.assertTrue(disasm_has_factory_chain(self.text, va), va)

    def test_vtable_anchor_and_factory_slot(self):
        get_features = struct.unpack(
            "<I", self.pe.read(EXPECTED_GS_VTABLE_VA + VTABLE_SLOT_GET_FEATURES, 4)
        )[0]
        self.assertEqual(get_features, GET_FEATURES_VA)
        fn = struct.unpack("<I", self.pe.read(EXPECTED_GS_VTABLE_VA + VTABLE_SLOT_FACTORY, 4))[0]
        self.assertEqual(fn, EXPECTED_FACTORY_VA)
        self.assertGreaterEqual(EXPECTED_FACTORY_VA, TEXT_BASE)
        self.assertLess(EXPECTED_FACTORY_VA, TEXT_BASE + len(self.text))

    def test_factory_in_text_and_fingerprint(self):
        off = EXPECTED_FACTORY_VA - TEXT_BASE
        prologue = self.text[off : off + PROLOGUE_BYTES]
        self.assertEqual(prologue[:4], b"\x55\x8b\xec\x6a")
        self.assertEqual(fnv1a64_prologue(self.text, EXPECTED_FACTORY_VA), EXPECTED_FACTORY_FINGERPRINT)

    def test_runtime_gate_fail_closed_on_wrong_target(self):
        good = gate_accepts(self.text, IMAGE_BASE, EXPECTED_FACTORY_VA)
        self.assertTrue(good)
        self.assertFalse(gate_accepts(self.text, IMAGE_BASE, EXPECTED_FACTORY_VA + 4))
        self.assertFalse(gate_accepts(self.text, IMAGE_BASE, 0xF23DF0))  # WeaponLocker +0x20C, not GS factory


if __name__ == "__main__":
    unittest.main()
