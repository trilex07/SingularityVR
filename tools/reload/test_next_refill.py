import copy
from pathlib import Path
import unittest
from inspect_next_refill import CLASSES, shared_bindings
from inspect_refill_only import fingerprint

ROOT = Path(__file__).resolve().parents[2]


class Tests(unittest.TestCase):
    def rows(self):
        methods = [dict(name=name, method=i+100) for i, name in enumerate(
            ('GetAmmoInClip', 'GetAmmoInBin', 'GetClipSize', 'RefillClip'))]
        rows = [dict(class_name=c, methods={m['name']: m['method'] for m in methods}) for c in CLASSES]
        return rows, methods

    def test_all_twelve_bindings(self):
        rows, methods = self.rows()
        self.assertTrue(shared_bindings(rows, methods))
        for cls in range(3):
            for name in rows[cls]['methods']:
                bad = copy.deepcopy(rows);bad[cls]['methods'][name] += 1
                with self.assertRaisesRegex(ValueError, 'override'): shared_bindings(bad, methods)

    def test_unknown_missing_or_duplicate_class(self):
        rows, methods = self.rows()
        for bad in (rows[:2], rows+[rows[0]], [rows[0], rows[1], rows[1]]):
            with self.assertRaises(ValueError): shared_bindings(bad, methods)
        rows[0]['class_name'] = 'Unknown'
        with self.assertRaises(ValueError): shared_bindings(rows, methods)

    def test_missing_getter(self):
        rows, methods = self.rows();del rows[1]['methods']['GetAmmoInBin']
        with self.assertRaises(ValueError): shared_bindings(rows, methods)
        with self.assertRaises(ValueError): shared_bindings(self.rows()[0], methods[:3])

    def test_only_cast_rel32_is_normalized(self):
        code = bytearray(53);code[13] = 0xe8
        f = fingerprint(code, 14)
        for at in range(53):
            bad = bytearray(code);bad[at] ^= 1
            if at == 13:
                with self.assertRaises(ValueError): fingerprint(bad, 14)
            else:
                self.assertEqual(fingerprint(bad, 14) == f, 14 <= at < 18)

    def test_runtime_no_original_in_new_branch(self):
        src = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        branch = src.split('if(g_nextRefillOnly&&nextOwned&&decision==', 1)[1].split('// This is forwarding', 1)[0]
        self.assertIn('nativerefill::Commit', branch)
        self.assertNotIn('g_origCenturionAttemptReload(', branch)
        self.assertNotIn('ProcessEvent(', branch)
        self.assertIn('"ReloadRefillOnly",0,path', src)
        self.assertIn('g_nextNativeGate.current.canPermit=false', src)

    def test_engine_binding_audit_retained(self):
        src = (ROOT/'spikes/view_matrix/native_refill_engine.inl').read_text(encoding='utf-8')
        self.assertIn('nativerefill::Audited', src)
        self.assertIn('method+spec.rel+4+rel!=inventoryCast', src)
        self.assertIn('strcmp(owner,"RvWeaponShared")', src)

    def test_holster_keeps_block_only_snapshot(self):
        src = (ROOT/'spikes/view_matrix/weapon_reload_dispatch.inl').read_text(encoding='utf-8')
        self.assertIn('valid||storedOwned', src)
        self.assertIn('!g_nextRefillOnly||!Rd(g_weaponHolstered)', src)
        self.assertIn('next.insertedMs=now', src)
        self.assertIn('!g_nextRefillOnly&&next.rootValid&&observedPermitMs', src)

    def test_discovery_completes_before_next_field_and_keeps_all_audits(self):
        src = (ROOT/'spikes/view_matrix/native_refill_engine.inl').read_text(encoding='utf-8')
        scan = src.split('for(unsigned n=0;', 1)[1].split('if(found!=4)', 1)[0]
        self.assertLess(scan.index('++found;report.matched|=1u<<j;'), scan.index('if(found==4)break;'))
        self.assertLess(scan.index('if(found==4)break;'), scan.index('TryLoadAlignedI32(field+g_upNext'))
        self.assertIn('if(pending.method[j]||', scan)
        self.assertIn('if(field&&found!=4)return reject("field-budget")', scan)
        resolver = src.split('static bool ResolveNativeRefill(', 1)[1].split('static void LogNativeRefillAudit', 1)[0]
        for stage in ('missing-bindings','wrapper-abi','method-range/slot','method-fingerprint','inventory-cast-target'):
            self.assertLess(resolver.index('reject("'+stage+'")'), resolver.index('result=cached;return true;'))
        self.assertEqual(resolver.count('result=cached'), 1)

    def test_each_profile_preflight_is_bounded_and_read_only(self):
        src = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        probe = src.split('if(g_nextRefillOnly&&nextOwned&&object==weapon', 1)[1].split('if(note)Log(', 1)[0]
        for guard in ('index<3','probes[index]<3','now>=nextProbe[index]','nextProbe[index]=now+1000',
                      'complete[index]=ready','ResolveNativeRefill(self,unused,&audit)'):
            self.assertIn(guard, probe)
        for forbidden in ('.Arm(', '.Refill(', 'nativerefill::Commit(', 'g_origCenturionAttemptReload('):
            self.assertNotIn(forbidden, probe)

    def test_body_scope_and_magazine_share_refill_only_release_policy(self):
        src = (ROOT/'spikes/view_matrix/weapon_reload_dispatch.inl').read_text(encoding='utf-8')
        prepare = src.split('static int PrepareNextReload(', 1)[1].split('static void DrawNextReload(', 1)[0]
        for call in ('Animation::HoldRoot(g_nextRefillOnly!=0', 'Animation::AnimateMagazine(g_nextRefillOnly!=0',
                     'if(!rootHeld||!armbind::Finite(visual.root))return 0;', 'rootHeld?&visual.root:nullptr',
                     'if(!animateMagazine&&!rootHeld&&!drop)return 0;', 'weaponreload::SeatingPalette('):
            self.assertIn(call, prepare)
        self.assertIn('if(g_nextRefillOnly&&visual.insertedMs)', prepare)


if __name__ == '__main__': unittest.main()
