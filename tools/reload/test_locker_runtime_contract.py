import struct
import unittest
from inspect_locker_runtime_contract import (
    ADDRESSES, SIZES, HELPERS, calls, find_all, inspect,
    inspect_start, inspect_equip, inspect_popmenu, inspect_load)

BASE = 0x402000


def emit_call(at, target):
    return b'\xe8' + struct.pack('<i', target - (at + 5))


def method_region(name):
    va = ADDRESSES[name]
    size = SIZES[name]
    return va, bytearray(b'\xcc' * size)


def placed(name, buf):
    """Pad a single method body so it sits at its true capture offset."""
    va = ADDRESSES[name]
    out = bytearray(va - BASE + len(buf) + 16)
    out[va - BASE:va - BASE + len(buf)] = buf
    return bytes(out)


def fake_start():
    va, buf = method_region('Start')
    def put(off, b): buf[off:off + len(b)] = b
    put(0, b'\x55\x8b\xec')
    # getter slot 0x740 loads + push 0 + append
    put(0x30, b'\x8b\x92\x40\x07\x00\x00')          # mov edx,[edx+0x740]
    put(0x36, b'\x6a\x00')                           # push 0
    put(0x50, emit_call(va + 0x50, HELPERS['tarrayAppend']))
    # getter slot 0x740 loads + push 1 + append
    put(0x60, b'\x8b\x90\x40\x07\x00\x00')
    put(0x66, b'\x6a\x01')
    put(0x80, emit_call(va + 0x80, HELPERS['tarrayAppend']))
    return va, buf


def fake_equip():
    va, buf = method_region('equip')
    def put(off, b): buf[off:off + len(b)] = b
    put(0, b'\x55\x8b\xec')
    put(0x10, b'\x8b\x4d\x0c')                       # mov ecx,[ebp+0xc] slotNum
    put(0x20, b'\x89\x14\xc8')                       # mov [eax+ecx*8],edx
    put(0x30, b'\xc2\x0c\x00')                       # ret 0xc
    return va, buf


def fake_popmenu():
    va, buf = method_region('PopMenu')
    def put(off, b): buf[off:off + len(b)] = b
    put(0, b'\x55\x8b\xec')
    put(0x40, b'\x8b\x92\x44\x07\x00\x00')           # SetWeaponSlotName dispatch
    put(0x50, emit_call(va + 0x50, HELPERS['tarrayFind']))
    put(0x60, emit_call(va + 0x60, HELPERS['destroyItem']))
    put(0x70, emit_call(va + 0x70, HELPERS['tarrayFind']))
    put(0x80, emit_call(va + 0x80, HELPERS['spawnByName']))
    put(0x90, b'\x0f\xb6\x86\xc0\x00\x00\x00')       # movzx eax,byte [esi+0xC0]
    put(0xA0, b'\x68' + struct.pack('<I', HELPERS['scriptCallback']))
    put(0xA6, emit_call(va + 0xA6, 0x41F380))
    return va, buf


def fake_load():
    va, buf = method_region('LoadEquippedWeapons')
    def put(off, b): buf[off:off + len(b)] = b
    put(0, b'\x55\x8b\xec')
    put(0x30, b'\x8b\x87\xb4\x00\x00\x00')           # mov eax,[edi+0xb4] count
    put(0x40, b'\x8b\x81\xb0\x00\x00\x00')           # mov eax,[ecx+0xb0] data
    return va, buf


def build_text():
    # The four methods live ~0xB21000 bytes into .text; one allocated buffer
    # covers them all without materializing the real 20 MB capture.
    top = max(ADDRESSES[n] + SIZES[n] for n in ADDRESSES) - BASE
    text = bytearray(top + 0x1000)
    for maker in (fake_start, fake_equip, fake_popmenu, fake_load):
        va, buf = maker()
        assert va - BASE + len(buf) <= len(text)
        text[va - BASE:va - BASE + len(buf)] = buf
    return va_start_guard(text)


def va_start_guard(text):
    # capture must begin with a function prologue
    text[0:3] = b'\x55\x8b\xec'
    return text


class Tests(unittest.TestCase):
    def test_helpers(self):
        code = bytearray(8)
        code[0] = 0xE8
        struct.pack_into('<i', code, 1, 0x100 - 0x40205)
        self.assertEqual(calls(bytes(code), 0x40200, 0x100), [0x40200])
        self.assertEqual(find_all(bytes(code), b'\xe8'), [0])

    def test_start_contract_and_refusals(self):
        va, buf = fake_start()
        report = inspect_start(placed('Start', buf), BASE)
        self.assertEqual(report['slot_indices'], [0, 1])
        self.assertEqual(len(report['append_sites']), 2)
        bad = bytearray(buf); bad[0x31] ^= 1          # break getter slot bytes
        with self.assertRaises(ValueError): inspect_start(placed('Start', bad), BASE)
        third = bytearray(buf)
        third[0x90:0x92] = b'\x6a\x02'                # an invented slot 2
        # a third push(2) is not in (0,1) so the matcher keeps [0,1]; the count
        # of getter dispatches is the discriminator:
        third[0xA0:0xA6] = b'\x8b\x92\x40\x07\x00\x00'
        with self.assertRaises(ValueError): inspect_start(placed('Start', third), BASE)

    def test_equip_unbounded_write(self):
        va, buf = fake_equip()
        report = inspect_equip(placed('equip', buf), BASE)
        self.assertIn('no bounds check', report['conclusion'])
        alloc = bytearray(buf)
        alloc[0x40:0x45] = emit_call(va + 0x40, HELPERS['tarrayAppend'])
        with self.assertRaises(ValueError): inspect_equip(placed('equip', alloc), BASE)

    def test_popmenu_commit_contract(self):
        va, buf = fake_popmenu()
        report = inspect_popmenu(placed('PopMenu', buf), BASE)
        self.assertEqual(report['active_slot_byte'], 0xC0)
        self.assertIn('overwrites special slots', report['conclusion'])
        # calls() returns absolute VAs; destroy/find sites must not add method base twice.
        self.assertEqual(report['destroy_site'], hex(va + 0x60))
        self.assertEqual(report['find_sites'], [hex(va + 0x50), hex(va + 0x70)])
        self.assertEqual(report['callback_site'], hex(va + 0xA0))
        bad = bytearray(buf); bad[0x41] ^= 1          # setter slot bytes
        with self.assertRaises(ValueError): inspect_popmenu(placed('PopMenu', bad), BASE)
        two_setters = bytearray(buf)
        two_setters[0xC0:0xC6] = b'\x8b\x92\x44\x07\x00\x00'
        with self.assertRaises(ValueError): inspect_popmenu(placed('PopMenu', two_setters), BASE)
        no_push = bytearray(buf)
        no_push[0xA0:0xA5] = b'\x90' * 5              # remove push immediate callback
        with self.assertRaises(ValueError): inspect_popmenu(placed('PopMenu', no_push), BASE)
        no_bridge = bytearray(buf)
        no_bridge[0xA6:0xAB] = b'\x90' * 5            # remove 0x41F380 dispatch
        with self.assertRaises(ValueError): inspect_popmenu(placed('PopMenu', no_bridge), BASE)

    def test_load_enumerates_not_hardcoded(self):
        va, buf = fake_load()
        report = inspect_load(placed('LoadEquippedWeapons', buf), BASE)
        self.assertEqual(report['count_field'], '+0xB4')
        loop = bytearray(buf)
        loop[0x10:0x12] = b'\x6a\x00'
        loop[0x14:0x16] = b'\x6a\x01'
        with self.assertRaises(ValueError): inspect_load(placed('LoadEquippedWeapons', loop), BASE)

    def test_full_inspect_and_prologue_guard(self):
        text = build_text()
        report = inspect(bytes(text), BASE)
        self.assertEqual(set(report['methods']),
                         {'Start', 'equip', 'PopMenu', 'LoadEquippedWeapons'})
        nofun = bytearray(text); nofun[0] = 0x90
        with self.assertRaises(ValueError): inspect(bytes(nofun), BASE)
        out_of_range = bytearray(b'\x55\x8b\xec' + b'\x00' * 16)
        with self.assertRaises(ValueError): inspect(bytes(out_of_range), BASE)


if __name__ == '__main__':
    unittest.main()
