import os, shutil, io

base = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014'
out = os.path.join(base, 'casting_final')
os.makedirs(out, exist_ok=True)

UV = os.path.join(base, 'user_voice')
IC = os.path.join(base, 'internet_candidates')
IV = os.path.join(base, 'intro_voice')
REF = os.path.join(base, 'references')
PD = os.path.join(base, 'pocket_demos')

def ls(d):
    return sorted(os.listdir(d))

uv_files = ls(UV)
log = io.open(os.path.join(out, 'build_log.txt'), 'w', encoding='utf-8')
log.write('user_voice files:\n')
for f in uv_files:
    log.write(f'  {f}\n')
log.write('internet_candidates:\n')
for f in ls(IC):
    log.write(f'  {f}\n')

# Role folders + copy small samples (<2 MB) for one-click listening
roles = {
    'role_barisov': ['mc_speech_sample1.wav', 'mc_speech_sample2.wav'],
    'role_demichev': [],
    'role_kathryn': [],
    'role_soldiers_user': [],
    'role_lektor': [],
}
import shutil as sh
for role, files in roles.items():
    os.makedirs(os.path.join(out, role), exist_ok=True)

# soldiers: copy all 5 user m4a with role names
soldier_map = {}
for f in uv_files:
    low = f.lower()
    if 'stanowczy' in low:
        soldier_map[f] = 'zolnierz_dowodca_rozkazy.m4a'
    elif 'napiety' in low or 'napi' in low:
        soldier_map[f] = 'zolnierz_walka_okrzyki.m4a'
    elif 'spok' in low:
        soldier_map[f] = 'zolnierz_patrol_meldunki.m4a'
    elif f == 'M1.m4a':
        soldier_map[f] = 'zolnierz_szeregowy_A.m4a'
    elif f == 'M2.m4a':
        soldier_map[f] = 'zolnierz_szeregowy_B.m4a'
    else:
        soldier_map[f] = f
for src, dst in soldier_map.items():
    sh.copy2(os.path.join(UV, src), os.path.join(out, 'role_soldiers_user', dst))
    log.write(f'soldier copy: {src} -> {dst}\n')

# barisov alt: copy MC samples
for f in roles['role_barisov']:
    sh.copy2(os.path.join(IC, f), os.path.join(out, 'role_barisov', 'ALT_' + f))
    log.write(f'barisov alt copy: {f}\n')

# kathryn alts: copy model refs (small)
for f in ['piper_gosia_reference.wav', 'rhvoice_magda_reference.wav', 'rhvoice_magda_demo_short.wav']:
    src = os.path.join(REF, f)
    if os.path.exists(src):
        sh.copy2(src, os.path.join(out, 'role_kathryn', 'ALT_' + f))
        log.write(f'kathryn alt copy: {f}\n')
src = os.path.join(PD, 'kathryn_magda_ref.wav')
if os.path.exists(src):
    sh.copy2(src, os.path.join(out, 'role_kathryn', 'ALT_pocket_kathryn_magda_ref.wav'))
    log.write('kathryn alt copy: pocket kathryn_magda_ref.wav\n')

# demichev alt: pocket darkman + piper darkman
for f in ['piper_darkman_reference.wav']:
    src = os.path.join(REF, f)
    if os.path.exists(src):
        sh.copy2(src, os.path.join(out, 'role_demichev', 'ALT_' + f))
        log.write(f'demichev alt copy: {f}\n')
src = os.path.join(PD, 'demichev_darkman_ref.wav')
if os.path.exists(src):
    sh.copy2(src, os.path.join(out, 'role_demichev', 'ALT_pocket_demichev_darkman_ref.wav'))
    log.write('demichev alt copy: pocket demichev_darkman_ref.wav\n')

# lektor alt: pocket lektor refs
for f in ['lektor_natan_ref.wav', 'lektor_magda_ref.wav', 'lektor_cml_ref.wav', 'lektor_mc_ref.wav']:
    src = os.path.join(PD, f)
    if os.path.exists(src):
        sh.copy2(src, os.path.join(out, 'role_lektor', 'ALT_pocket_' + f))
        log.write(f'lektor alt copy: {f}\n')

log.write('DONE\n')
log.close()
print('build ok')
