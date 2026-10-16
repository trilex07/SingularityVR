import hashlib
from pathlib import Path
import tempfile
import unittest

import analyze_reload_log as analyzer


HEADER = '=== view_matrix attached 2026-10-16 12:00:00 - run starts here ==='


def ammo(t, clip, reserve, weapon='00000011', hud='00000022', klass='00000033', size=6):
    return (f'centurion reload ammo: weapon={weapon} hud={hud} class={klass} '
            f'clip={clip} reserve={reserve} size={size} t={t} (observer only)')


def probe(t, name='AnimNotify_RefillClip', weapon='00000011', pawn='00000044', self_id=None):
    if self_id is None:
        self_id = pawn if name == 'AnimNotify_RefillClip' else weapon
    return (f'centurion reload probe: {name} self={self_id} Pawn/RvSharedPawn '
            f'weapon={weapon} pawn={pawn} t={t} (entry only, no ABI call)')


def report(*rows, window_ms=5000):
    return analyzer.analyze([HEADER, *rows], window_ms=window_ms)


class Tests(unittest.TestCase):
    def test_conserved_transfer_and_callback_are_only_correlation(self):
        result = report(ammo(100, 2, 12), probe(1500), ammo(1600, 6, 8))
        self.assertFalse(result['native_reload_proven'])
        item = result['sessions'][0]['transfers'][0]
        self.assertEqual(item['classification'], 'CORRELATION_ONLY')
        self.assertEqual(item['amount'], 4)
        self.assertEqual(item['callback_pawns'], ['00000044'])

    def test_structural_binding_or_callback_alone_is_not_transfer(self):
        result = report('centurion reload binding: matches=4 fieldsVisited=200 '
                        '(structural identity only; not reload execution proof)', probe(1500))
        self.assertEqual(result['sessions'][0]['binding_rows'], 1)
        self.assertEqual(result['sessions'][0]['transfers'], [])

    def test_unmatched_conservation_or_polling_is_not_reload_proof(self):
        for middle in ([], [probe(200, 'IsReloading')], [probe(200, weapon='00000099')],
                       [probe(200, self_id='00000099')], [probe(200, pawn='00000000')],
                       [probe(200), probe(201, pawn='00000055')]):
            item = report(ammo(100, 2, 12), *middle, ammo(300, 6, 8))['sessions'][0]['transfers'][0]
            self.assertEqual(item['classification'], 'CONSERVED_DELTA_ONLY')

    def test_pickup_firing_and_unequal_reserve_never_credit_transfer(self):
        for clip, reserve in ((2, 18), (1, 12), (6, 9), (6, 12), (2, 8)):
            result = report(ammo(100, 2, 12), probe(200), ammo(300, clip, reserve))
            self.assertEqual(result['sessions'][0]['transfers'], [])

    def test_hud_weapon_class_capacity_changes_break_chain(self):
        for changed in (dict(weapon='00000099'), dict(hud='00000099'),
                        dict(klass='00000099'), dict(size=7)):
            result = report(ammo(100, 2, 12), probe(200), ammo(300, 6, 8, **changed))
            self.assertEqual(result['sessions'][0]['transfers'], [])

    def test_invalid_counts_and_zero_identity_break_chain(self):
        invalid = [ammo(200, -1, 12), ammo(200, 7, 12), ammo(200, 2, -1),
                   ammo(200, 2, 1000001), ammo(200, 2, 12, size=1001),
                   ammo(200, 0, 12, size=0), ammo(200, 2, 12, weapon='00000000'),
                   ammo(200, 2, 12, hud='00000000'), ammo(200, 2, 12, klass='00000000')]
        for middle in invalid:
            result = report(ammo(100, 2, 12), middle, probe(250), ammo(300, 6, 8))
            self.assertEqual(result['sessions'][0]['invalid_ammo_rows'], 1)
            self.assertEqual(result['sessions'][0]['transfers'], [])

    def test_session_boundary_cannot_join_even_reused_addresses(self):
        result = report(ammo(100, 2, 12), HEADER, probe(200), ammo(300, 6, 8))
        self.assertEqual(len(result['sessions']), 2)
        self.assertTrue(all(not s['transfers'] for s in result['sessions']))

    def test_timing_gap_same_tick_and_outside_callback(self):
        for t in (100, 5101):
            self.assertEqual(report(ammo(100, 2, 12), ammo(t, 6, 8))['sessions'][0]['transfers'], [])
        result = report(ammo(100, 2, 12), probe(50), probe(301), ammo(300, 6, 8))
        self.assertEqual(result['sessions'][0]['transfers'][0]['callbacks'], [])
        for window in (0, -1, 60001):
            with self.assertRaises(ValueError):
                report(window_ms=window)

    def test_async_file_order_uses_ticks_and_reports_reordering(self):
        result = report(ammo(300, 6, 8), probe(200), ammo(100, 2, 12))['sessions'][0]
        self.assertEqual(result['transfers'][0]['classification'], 'CORRELATION_ONLY')
        self.assertIn('ammo rows reordered by tick; async file order differed', result['warnings'])

    def test_malformed_relevant_rows_disable_inference_not_crash(self):
        for broken in (ammo(150, 3, 12)[:-5], probe(200).replace('t=200', 't=nope')):
            result = report(ammo(100, 2, 12), broken, probe(200), ammo(300, 6, 8))['sessions'][0]
            self.assertEqual(result['malformed_lines'], [3])
            self.assertEqual(result['transfers'], [])

    def test_empty_partial_and_caps_are_explicit(self):
        self.assertEqual(analyzer.analyze([])['sessions'], [])
        partial = analyzer.analyze([ammo(100, 2, 12)])['sessions'][0]
        self.assertTrue(partial['warnings'])
        capped = report(*[probe(100 + i) for i in range(16)])['sessions'][0]
        self.assertTrue(any('cap' in w for w in capped['warnings']))

    def test_unrelated_logs_ignored_and_input_bounds(self):
        result = report('centurion reload: HUD observers offsets=123/456/789 (read-only)',
                        'centurion reload probe: name attempt=1 existing script hook=1234 '
                        '(zero events is NOT proof of no reload)', 'xinput: pad attached')
        self.assertEqual(result['sessions'][0]['malformed_lines'], [])
        self.assertEqual(result['sessions'][0]['valid_probe_rows'], 0)
        with self.assertRaises(ValueError):
            analyzer.analyze(['x' * (analyzer.MAX_LINE + 1)])
        with self.assertRaises(ValueError):
            report(*[ammo(i, 2, 12) for i in range(analyzer.MAX_RECORDS + 1)])

    def test_readonly_file_sha_and_line_numbers(self):
        raw = ('unrelated\n' + HEADER + '\n' + ammo(100, 2, 12) + '\n' +
               probe(200) + '\n' + ammo(300, 6, 8) + '\n').encode('utf-8')
        test_root = Path(__file__).resolve().parents[2] / 'build/centurion-reload/log-tests'
        test_root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=test_root) as directory:
            path = Path(directory) / 'synthetic.log'
            path.write_bytes(raw)
            lines, sha = analyzer.read_log(path)
            self.assertEqual(sha, hashlib.sha256(raw).hexdigest())
            self.assertEqual(path.read_bytes(), raw)
            self.assertEqual(analyzer.analyze(lines)['sessions'][0]['transfers'][0]['after_line'], 5)
            path.write_bytes(b'x' * (analyzer.MAX_LINE + 1))
            with self.assertRaises(ValueError):
                analyzer.read_log(path)


if __name__ == '__main__':
    unittest.main()
