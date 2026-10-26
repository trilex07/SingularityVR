import unittest
from plan_campaign_audio import plan


class CampaignPlanTests(unittest.TestCase):
    def wav(self, pack='story', filename='hash.wav'):
        return dict(pack=pack,file=filename,sha256='a'*64,frames=24000,sample_rate=24000)

    def native(self, package='SP_A_LOC_INT', asset='A', filename='hash.wav'):
        return dict(package=package,asset=asset,file=filename,source=package+'/SoundNodeWave/'+asset+'.ogg')

    def run_plan(self, files, rows, skipped=None, vignette=None):
        return plan(dict(dubbing=dict(status='TECHNICAL_PASS_APPROVAL_PENDING',files=files)),
                    rows, skipped or {}, vignette or {})

    def test_aliases_are_choices_never_implicit_overlays(self):
        row=self.native()
        result=self.run_plan([self.wav(),self.wav('skipped','A.pl.wav')],[row],
                            {'A':dict(package=row['package'],asset='A',source=row['source'])})
        self.assertEqual(result['counts']['mapped_takes'],2)
        self.assertEqual(result['counts']['unique_native_waves'],1)
        self.assertEqual(result['counts']['choice_groups'],1)
        self.assertFalse(result['approval_granted'])

    def test_named_asset_requires_exact_curated_package_and_source(self):
        row=self.native()
        result=self.run_plan([self.wav('skipped','A.pl.wav')],[row],
                            {'A':dict(package='SP_OTHER_LOC_INT',asset='A',source=row['source'])})
        self.assertEqual(result['counts']['mapped_takes'],0)
        self.assertEqual(result['unmatched'][0]['reason'],'NATIVE_IDENTITY_MISSING')

    def test_duplicate_lookup_name_is_ambiguous_not_first_match(self):
        result=self.run_plan([self.wav()],[self.native(),self.native('SP_B_LOC_INT')])
        self.assertEqual(result['unmatched'][0]['reason'],'NATIVE_IDENTITY_AMBIGUOUS')

    def test_tmp_artifact_refused_even_if_present_in_lookup(self):
        result=self.run_plan([self.wav(filename='hash.wav.tmp.wav')],
                             [self.native(filename='hash.wav.tmp.wav')])
        self.assertEqual(result['unmatched'][0]['reason'],'TEMPORARY_ARTIFACT_NOT_A_NATIVE_TAKE')

    def test_noncampaign_packs_excluded_and_no_bank_pins_invented(self):
        result=self.run_plan([self.wav(),self.wav('mp'),self.wav('intro')],[self.native()])
        self.assertEqual(result['excluded_packs'],{'mp':1,'intro':1})
        self.assertIsNone(result['bank_inputs'][0]['accepted_physical_sha256'])

    def test_story_folder_does_not_prove_campaign_native_ownership(self):
        result=self.run_plan([self.wav()],[self.native('MP_CvS_Village_p_LOC_INT')])
        self.assertEqual(result['counts']['mapped_takes'],0)
        self.assertEqual(result['counts']['excluded_native_noncampaign'],1)
        self.assertEqual(result['excluded_native_noncampaign'][0]['package'],'MP_CvS_Village_p_LOC_INT')

    def test_existing_native_line_with_no_current_wav_is_explicit_gap(self):
        row=self.native(filename='')
        result=self.run_plan([], [row])
        self.assertEqual(result['counts']['uncovered_lookup_rows'],1)
        self.assertEqual(result['uncovered_lookup_rows'][0]['asset'],'A')


if __name__=='__main__':
    unittest.main()
