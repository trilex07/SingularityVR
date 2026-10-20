"""Offline wiring and original serialized operand audit; never opens a process."""
from pathlib import Path
import re
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/title'))
from ue3_layout import inspect


class CombinedFollowupTests(unittest.TestCase):
    def test_private_original_script_operands(self):
        path = ROOT / 'build/next-weapon-reload/npc-name-60pct-oneblock-20261019/control.u'
        self.assertTrue(path.is_file(), 'private original fixture required')
        data = path.read_bytes()
        package = inspect('memory', data, include_all=True)
        function = package['exports'][10581]
        self.assertEqual((function['name'], function['size']), ('SwitchWeaponByArchetype', 343))
        self.assertEqual(package['records'][function['outer'] - 1]['name'], 'RvInventoryManager')
        raw = data[function['offset']:function['offset'] + function['size']]
        self.assertEqual(struct.unpack_from('<I', raw, 40)[0], 284)
        code = raw[44:328]
        policy = (ROOT / 'spikes/view_matrix/inventory_retention_policy.h').read_text()
        objects, names = policy.split('inline constexpr Ref Names[]=', 1)
        for offset, expected in re.findall(r'\{(\d+),"(\w+)"\}', objects):
            ref = struct.unpack_from('<i', code, int(offset))[0]
            actual = (package['records'][ref - 1]['name'] if ref > 0 else
                      package['names'][package['imports'][-ref - 1][5]])
            self.assertEqual(actual, expected)
        for offset, expected in re.findall(r'\{(\d+),"(\w+)"\}', names.split(';', 1)[0]):
            index, number = struct.unpack_from('<II', code, int(offset))
            self.assertEqual((package['names'][index], number), (expected, 0))
        self.assertEqual(code[200], 0x1b)
        self.assertEqual(code[209:211], b'\x4a\x16')

    def test_retention_is_scoped_and_default_off(self):
        native = (ROOT / 'spikes/view_matrix/native_owned_inventory.h').read_text()
        self.assertIn('(uintptr_t)self!=manager', native)
        self.assertIn('thread!=GetCurrentThreadId()', native)
        # Shared script is immutable. The exact opcode adapter consumes the
        # call including EmptyParmValue/EndFunctionParms on the caller frame.
        self.assertNotIn('memcpy((void*)(script+inventoryretention::DropAt),saved', native)
        self.assertNotIn('memset((void*)(script+inventoryretention::DropAt),0x0b', native)
        self.assertIn('no bytecode write', native)
        source = (ROOT / 'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        self.assertIn('"KeepAcquiredWeapons",0,path', source)
        self.assertIn('"WeaponWheelOwnedInventory",0,path', source)
        self.assertIn('NativeOwnedInventory::Queue((int)tgtV)', source)
        self.assertIn('ownedMask & inventoryretention::WeaponBit', source)
        hook = (ROOT / 'spikes/view_matrix/inventory_vm_hook.h').read_text()
        self.assertIn('NativeOwnedInventory::SkipPickupThrow(self,frame)', hook)
        self.assertIn('original(self,edx,frame,result)', hook)
        self.assertIn('InventoryVmHook::Install();', source)

    def test_wheel_never_empty_and_ownership_uses_audited_layout(self):
        native = (ROOT / 'spikes/view_matrix/native_owned_inventory.h').read_text()
        source = (ROOT / 'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        # The wheel falls back to the accepted mesh-observed set whenever the
        # possession snapshot is not ready: an empty wheel is a regression.
        self.assertIn('NativeOwnedInventory::Ready()', source)
        self.assertIn('const bool ownedReady = Rd(g_ownedWeaponWheel) && ownedMask!=0;', source)
        self.assertIn('NativeOwnedInventory::VertsFromMask(ownedMask,ownedSeen,8)', source)
        self.assertIn('labelSeen = ownedReady ? ownedSeen : seenCopy', source)
        self.assertIn('if (Rd(g_ownedWeaponWheel) && NativeOwnedInventory::Ready())', source)
        self.assertIn('const uintptr_t vt=P(pawn);', native)
        self.assertNotIn('const uintptr_t vt=P(P(pawn));', native)
        # The possession chain is read through the audited native layout and
        # every object is validated by its class name.
        self.assertIn('kPawnInvManager=0x3d0', native)
        self.assertIn('kManagerChain=0x1e0', native)
        self.assertIn('kItemNext=0x1e0', native)
        self.assertIn('kItemWeaponName=0x2e8', native)
        self.assertIn('ClassHas(pc,"PlayerPawn")', native)
        self.assertIn('ClassHas(mc,"InventoryManager")', native)
        self.assertIn('ClassHas(ic,"RvWeaponShared")', native)
        # The selector is identified by the audited vtable neighbour, not by a
        # body hash that cannot be recomputed from the packed image.
        self.assertIn('inventoryretention::HasWeaponImpl', native)
        self.assertIn('HasWeaponImpl=0x0102F700ull',
                      (ROOT / 'spikes/view_matrix/inventory_retention_policy.h').read_text())

    def test_npc_and_icon_gates(self):
        source = (ROOT / 'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        self.assertNotIn('i < 8 && slotIcons[i] && slotNamed[i]', source)
        native = (ROOT / 'spikes/view_matrix/native_npc_name.h').read_text()
        self.assertIn('_root.txtMsgContainer.npcTextField.nameTxt._xscale', native)
        self.assertIn('_root.txtMsgContainer.npcTextField.nameTxt._yscale', native)
        self.assertIn('0x21ACFC3AD63D0F40ull', native)


if __name__ == '__main__':
    unittest.main()
