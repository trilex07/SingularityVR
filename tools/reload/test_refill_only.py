import struct
import unittest
from inspect_refill_only import fingerprint, void_slot
from pathlib import Path


class Tests(unittest.TestCase):
    def test_fingerprint_masks_only_one_rel32(self):
        raw = bytearray(64);raw[13] = 0xe8
        expected = fingerprint(raw, 14)
        for i in range(len(raw)):
            bad = bytearray(raw);bad[i] ^= 1
            if i == 13:
                with self.assertRaises(ValueError): fingerprint(bad, 14)
            else:
                self.assertEqual(fingerprint(bad, 14) == expected, 14 <= i < 18)
        for rel in (-1, 0, 63, 65):
            with self.assertRaises(ValueError): fingerprint(raw, rel)

    def test_void_not_int_wrapper(self):
        raw = bytearray(53);raw[:9] = bytes.fromhex('558bec8b4508ff4018')
        raw[36:40] = bytes.fromhex('8b068b90');struct.pack_into('<I', raw, 40, 0x4c8)
        raw[44:] = bytes.fromhex('8bceffd25e5dc20800')
        self.assertEqual(void_slot(raw), 0x4c8)
        for at in (0, 8, 36, 44, 48, 52):
            bad = bytearray(raw);bad[at] ^= 1
            with self.assertRaises(ValueError): void_slot(bad)
        with self.assertRaises(ValueError): void_slot(raw[:-1])

    def test_runtime_branch_does_not_forward_or_invoke_interpreter(self):
        src = (Path(__file__).resolve().parents[2]/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        branch = src.split('if(g_centurionRefillOnly&&decision==', 1)[1].split('// This is forwarding', 1)[0]
        self.assertIn('centurionrefill::Commit', branch)
        self.assertIn('Result::Fault)fault=true', branch)
        self.assertNotIn('g_origCenturionAttemptReload(', branch)
        self.assertNotIn('ProcessEvent(', branch)
        self.assertIn('"CenturionRefillOnly",0,path', src)

    def test_preflight_is_bounded_and_cannot_transfer(self):
        src = (Path(__file__).resolve().parents[2]/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        probe = src.split('if(g_centurionRefillOnly&&decision==centurionnative::Decision::Block', 1)[1].split('if(g_centurionPhysicalPolish', 1)[0]
        self.assertIn('probes<3&&now>=nextProbe', probe)
        self.assertIn('nextProbe=now+1000', probe)
        self.assertIn('ResolveCenturionRefill(self,unused,&audit)', probe)
        for call in ('centurionrefill::Commit(', '.Refill(', '.Arm(', 'g_origCenturionAttemptReload('):
            self.assertNotIn(call, probe)

    def test_failed_resolver_never_publishes_a_callable_result(self):
        src = (Path(__file__).resolve().parents[2]/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        resolver = src.split('static bool ResolveCenturionRefill(', 1)[1].split('static void LogCenturionRefillAudit', 1)[0]
        self.assertIn('report.stage="ready";result=cached;return true', resolver)
        for reason in ('wrapper-abi','field-budget','missing-bindings','method-fingerprint','inventory-cast-target'):
            self.assertIn('reject("'+reason+'")', resolver)
        self.assertIn('if(field)return reject("field-budget")', resolver)
        self.assertIn('centurionrefill::Audited', resolver) # strict checks NOT relaxed


if __name__ == '__main__': unittest.main()
