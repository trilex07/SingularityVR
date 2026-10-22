"""Real-binary evidence test for the TMD haptics native event source.

Disassembles the OWNED original executable's archived .text (skipped, not passed, if the private
inputs are absent) and checks the facts native_tmd_haptics.h relies on: body fingerprints, the
exec-wrapper -> direct-thiscall relationship, `ret 4`, and the early-out/return structure of the
Start path. Also verifies the header constants match what was measured.
"""
import os, re, sys, unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / 'spikes/view_matrix/native_tmd_haptics.h').read_text()
TEXT = Path(os.environ.get('SINGULARITY_TEXT', ROOT.parent / 'research/etw/capture-20261006-205508/Singularity.text-memory.bin'))
B = 0x402000

def fnv(b):
    h = 0xcbf29ce484222325
    for x in b:
        h = ((h ^ x) * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
    return h

def const(name):
    m = re.search(name + r'\s*=\s*(0x[0-9A-Fa-f]+)', HEADER)
    return int(m.group(1), 16)

class HeaderContract(unittest.TestCase):
    def test_constants_present(self):
        self.assertEqual(const('kStartAddr'), 0x100D790)
        self.assertEqual(const('kStopAddr'), 0x100AEC0)
        self.assertIn('kStartSize = 248', HEADER); self.assertIn('kStopSize = 194', HEADER)

    def test_detours_forward_original_first(self):
        start = HEADER[HEADER.index('Start(void* self'):HEADER.index('static int __fastcall Stop')]
        self.assertLess(start.index('originalStart('), start.index('Note('))
        self.assertIn('return r;', start)

@unittest.skipUnless(TEXT.exists(), 'archived .text not available')
class BinaryEvidence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = TEXT.read_bytes()
        from capstone import Cs, CS_ARCH_X86, CS_MODE_32
        cls.md = Cs(CS_ARCH_X86, CS_MODE_32)

    def body(self, a, n):
        return self.text[a - B:a - B + n]

    def test_fingerprints_match_header(self):
        self.assertEqual(fnv(self.body(0x100D790, 248)), const('kStartHash'))
        self.assertEqual(fnv(self.body(0x100AEC0, 194)), const('kStopHash'))
        # Wrapper fingerprints are the ones already pinned by tools/fx/audit_deadlock_backend.py
        self.assertEqual(fnv(self.body(0xF5D370, 96)), 0x7B8C05DBDEEC95C5)
        self.assertEqual(fnv(self.body(0xF5D3D0, 96)), 0xD5110E0A85330314)

    def test_wrappers_call_the_hooked_methods_directly(self):
        for wrapper, target in ((0xF5D370, 0x100D790), (0xF5D3D0, 0x100AEC0)):
            calls = [int(i.op_str, 16) for i in self.md.disasm(self.body(wrapper, 96), wrapper)
                     if i.mnemonic == 'call' and i.op_str.startswith('0x')]
            self.assertIn(target, calls)

    def test_both_methods_are_thiscall_one_stack_arg_ret4(self):
        self.assertEqual(self.body(0x100D790, 248)[-3:], bytes([0xC2, 0x04, 0x00]))
        self.assertEqual(self.body(0x100AEC0, 194)[-3:], bytes([0xC2, 0x04, 0x00]))

    def test_start_returns_the_early_out_flag_and_skips_body_when_nonzero(self):
        ins = list(self.md.disasm(self.body(0x100D790, 248), 0x100D790))
        txt = [f'{i.mnemonic} {i.op_str}' for i in ins]
        # ebx = al of vtable[+0x11C](tmd,0); nonzero -> jump to epilogue which returns bl
        self.assertIn('call edx', txt[:16]); self.assertIn('movzx ebx, al', txt)
        self.assertTrue(any(t.startswith('jne 0x100d87f') for t in txt))
        self.assertIn('mov al, bl', txt)
        # The activation body (state call + FX/owner hooks) lies strictly between them.
        idx = [i.address for i in ins]
        self.assertTrue(0x100D7B3 in idx and 0x100D87F in idx)

if __name__ == '__main__':
    unittest.main()
