"""Approval, provenance and native-identity safety (no game installation)."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import wave

import audio_release_gate as gate
from prepare_xtts_native_banks import restrict_native_scope, select_rows


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value), encoding='utf-8')


def write_wav(path, rate=24000, channels=1, frames=240):
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), 'wb') as out:
        out.setparams((channels, 2, rate, 0, 'NONE', 'not compressed'))
        out.writeframes(b'\x01\x00' * frames * channels)


class ReleaseGateTests(unittest.TestCase):
    def setUp(self):
        # Keep fixtures inside the workspace on restricted Windows runners.
        fixture_root = gate.ROOT / 'build/audio-release-private/tests'
        fixture_root.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=fixture_root)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.private = self.root / 'private'
        self.patcher = patch.object(gate, 'PRIVATE', self.private)
        self.patcher.start()
        self.addCleanup(self.patcher.stop)
        self.source = self.root / 'source'
        self.wav = self.source / 'story/new-take.wav'
        write_wav(self.wav)
        self.manifest = self.root / 'manifest.jsonl'
        write_json(self.manifest, dict(pack='story', file=self.wav.name,
                                      sha256=gate.sha(self.wav), size_bytes=self.wav.stat().st_size))
        self.audit = self.root / 'audit.json'
        write_json(self.audit, dict(dubbing=gate.inventory_dubbing(self.source, self.manifest)))
        self.lookup = self.root / 'lookup.jsonl'
        write_json(self.lookup, dict(package='SP_AREA_LOC_INT', asset='KAT_01',
                                    file='existing-runtime-name.wav', text_en='Original'))
        self.profile = self.root / 'profile.json'
        write_json(self.profile, dict(approved=True, files=[dict(
            path='Binaries/vr-assets/localization/pl/polish_story_voice.jsonl', sha256=gate.sha(self.lookup))]))
        self.approval = self.root / 'approval.json'
        write_json(self.approval, self.make_approval([dict(id='story/new-take.wav',
            sha256=gate.sha(self.wav), package='SP_AREA_LOC_INT', asset='KAT_01')]))

    def make_approval(self, selected):
        # Test-only approval: fixture bytes and evidence, never real production assets.
        return dict(schema='singularityvr-audio-approval-v1', audit_sha256=gate.sha(self.audit),
                    approved_by='fixture', listening_approved=True, rights_approved=True,
                    listening_evidence='fixture receipt', rights_evidence='synthetic PCM fixture', selected=selected)

    def stage(self):
        return gate.stage_dubbing(self.audit, self.approval, self.source, self.lookup,
                                  self.profile, self.private / 'candidate')

    def test_changed_bytes_refuse_manifest_even_when_count_is_correct(self):
        write_wav(self.wav, frames=241)
        result = gate.inventory_dubbing(self.source, self.manifest)
        self.assertEqual(result['status'], 'REFUSED')
        self.assertEqual(result['errors'][0]['error'], 'manifest SHA mismatch')
        self.assertEqual(result['files'][0]['sha256'], gate.sha(self.wav))

    def test_unmanifested_audio_is_hashed_and_never_silently_admitted(self):
        write_wav(self.source / 'story/extra.wav')
        result = gate.inventory_dubbing(self.source, self.manifest)
        self.assertEqual(result['status'], 'REFUSED')
        self.assertEqual(result['unmanifested_files'][0]['sha256'], gate.sha(self.source / 'story/extra.wav'))

    def test_truncated_pcm_is_not_validated_from_header_only(self):
        self.wav.write_bytes(self.wav.read_bytes()[:-4])
        with self.assertRaisesRegex(ValueError, 'truncated'):
            gate.wav_info(self.wav)

    def test_approval_required_before_any_payload_write(self):
        approved = gate.load(self.approval)
        for missing in ('listening_approved', 'rights_approved', 'rights_evidence'):
            changed = dict(approved)
            changed.pop(missing)
            write_json(self.approval, changed)
            with self.assertRaises(ValueError):
                self.stage()
            self.assertFalse(self.private.exists())

    def test_approval_cannot_follow_an_updated_inventory_implicitly(self):
        data = gate.load(self.audit)
        data['extra'] = 'new inventory'
        write_json(self.audit, data)
        with self.assertRaisesRegex(ValueError, 'different inventory'):
            self.stage()

    def test_stage_preserves_native_lookup_and_runtime_filename(self):
        before = self.lookup.read_bytes()
        staged = self.stage()
        self.assertEqual(staged['files'][0]['name'], 'existing-runtime-name.wav')
        candidate = self.private / 'candidate/payload/Binaries/vr-assets'
        self.assertEqual((candidate / 'localization/pl/polish_story_voice.jsonl').read_bytes(), before)
        self.assertEqual((candidate / 'audio/polish/story/existing-runtime-name.wav').read_bytes(), self.wav.read_bytes())
        self.assertEqual(staged['nativeScope'], [dict(package='SP_AREA_LOC_INT', asset='KAT_01')])
        self.assertFalse(staged['steam_written'])

    def test_unknown_native_asset_fails_without_mkdir(self):
        approved = gate.load(self.approval)
        approved['selected'][0]['asset'] = 'KAT_OTHER'
        write_json(self.approval, approved)
        with self.assertRaisesRegex(ValueError, 'missing/ambiguous'):
            self.stage()
        self.assertFalse(self.private.exists())

    def test_staged_source_drift_and_lookup_drift_refused(self):
        write_wav(self.wav, frames=241)
        with self.assertRaisesRegex(ValueError, 'WAV drift'):
            self.stage()
        write_wav(self.wav)
        self.lookup.write_text('{}', encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'lookup changed'):
            self.stage()

    def test_stage_refuses_external_output_and_existing_candidate(self):
        with self.assertRaisesRegex(ValueError, 'private output required'):
            gate.stage_dubbing(self.audit, self.approval, self.source, self.lookup,
                              self.profile, self.root / 'elsewhere')
        self.stage()
        with self.assertRaisesRegex(ValueError, 'never overwrite'):
            self.stage()

    def test_selected_subset_replaces_only_exact_asset_within_native_bank(self):
        rows = [dict(package='SP_A', asset='one', file='one.wav'),
                dict(package='SP_A', asset='two', file='two.wav')]
        scoped = restrict_native_scope(rows, dict(nativeScope=[dict(package='sp_a', asset='two')]))
        self.assertEqual(list(select_rows(scoped, 'SP_A')), ['two'])
        self.assertIs(restrict_native_scope(rows, {}), rows)

    def test_duplicate_identity_cannot_mask_missing_approved_identity(self):
        one = dict(package='SP_A', asset='one', file='one.wav')
        with self.assertRaisesRegex(ValueError, 'absent/ambiguous'):
            restrict_native_scope([one, one], dict(nativeScope=[
                dict(package='SP_A', asset='one'), dict(package='SP_A', asset='missing')]))
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            restrict_native_scope([one], dict(nativeScope=[
                dict(package='SP_A', asset='one'), dict(package='sp_a', asset='one')]))

    def test_remaster_stages_one_variant_only_and_refuses_unproven_graph(self):
        target = gate.REMASTER_IDS[0]
        folder = self.source / target
        for variant in gate.VARIANTS:
            write_wav(folder / variant, rate=48000, channels=2)
        meta = dict(id=target, dir_name=target, package='wp_SP_ARPistol_SF.xxx',
                    asset='arPistolFire03', cue='arPistolFireNEW', confidence='VERIFIED_REFERENCE')
        write_json(folder / 'metadata.json', meta)
        with patch.object(gate, 'REMASTER_IDS', (target,)):
            remaster = gate.inventory_remaster(self.source)
        write_json(self.audit, dict(remaster=remaster))
        selected = dict(id=target + '/restored_conservative.wav',
                        sha256=gate.sha(folder / 'restored_conservative.wav'))
        write_json(self.approval, self.make_approval([selected]))
        result = gate.stage_remaster(self.audit, self.approval, self.source, self.private / 'remaster')
        self.assertEqual(result['files'][0]['asset'], 'arPistolFire03')
        self.assertFalse((self.private / 'remaster' / target / 'original.wav').exists())
        remaster[0]['status'] = 'BLOCKED_NATIVE_GRAPH_UNPROVEN'
        write_json(self.audit, dict(remaster=remaster))
        write_json(self.approval, self.make_approval([selected]))
        with self.assertRaisesRegex(ValueError, 'graph unproven'):
            gate.stage_remaster(self.audit, self.approval, self.source, self.private / 'refused')
        self.assertFalse((self.private / 'refused').exists())


if __name__ == '__main__':
    unittest.main()
