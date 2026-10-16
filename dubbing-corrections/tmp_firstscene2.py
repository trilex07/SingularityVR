import json, os, shutil
REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
SRC = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual'
DST = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\firstscene'
os.makedirs(DST, exist_ok=True)
rows = [json.loads(l) for l in open(REPO + r'\localization\pl\polish_story_voice.jsonl',
                                    encoding='utf-8') if l.strip()]
want_pkgs = ('sp_freight_script_train_intro_LOC_INT', 'SP_RT1_kismet0_LOC_INT',
             'SP_RT1_kismet_LOC_INT', 'SP_RT1_zone0_BT_LOC_INT',
             'SP_RT1_Intro_LOC_INT', 'SP_RT1_fx01_LOC_INT')
sel = [r for r in rows if r['package'] in want_pkgs]
print('rows: %d' % len(sel))
n = 0
for r in sel:
    s = os.path.join(SRC, r['file'])
    if os.path.exists(s):
        shutil.copyfile(s, os.path.join(DST, r['package'] + '__' + r['file']))
        n += 1
    else:
        print('MISSING WAV', r['package'], r['asset'], r['file'])
print('copied: %d' % n)
for r in sorted(sel, key=lambda x: (x['package'], x['asset'])):
    print('%s | %s | %-12s | %-22s | EN: %s' % (
        r['package'], r['asset'], r.get('translation_status'),
        r['file'], r['text_en'][:80]))
    print('   PL: %s' % r['text_pl'][:130])
