import os, shutil

base = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014'
IC = os.path.join(base, 'internet_candidates')
out = os.path.join(base, 'casting_final')

# Patrol (internet): MC Speech samples
pat = os.path.join(out, 'role_patrol_internet')
os.makedirs(pat, exist_ok=True)
for f in ['mc_speech_sample1.wav', 'mc_speech_sample2.wav']:
    shutil.copy2(os.path.join(IC, f), os.path.join(pat, 'PATROL_' + f))

# Renko (main character): Nater ref file lives in internet_candidates (big mp3, reference only)
# Demichev primary already in pocket_demos; copy it into role_demichev as PRIMARY
PD = os.path.join(base, 'pocket_demos')
shutil.copy2(os.path.join(PD, 'demichev_darkman_ref.wav'),
             os.path.join(out, 'role_demichev', 'PRIMARY_pocket_demichev_darkman_ref.wav'))

# Barisov primary is Adrian mp3 in internet_candidates (reference only, 5.8 MB) - nothing to copy
# Renko ALT: piper mc_speech
REF = os.path.join(base, 'references')
os.makedirs(os.path.join(out, 'role_renko'), exist_ok=True)
src = os.path.join(REF, 'piper_mc_speech_reference.wav')
if os.path.exists(src):
    shutil.copy2(src, os.path.join(out, 'role_renko', 'ALT_piper_mc_speech_reference.wav'))

# Barisov ALT: pocket lektor_natan + rhvoice natan
os.makedirs(os.path.join(out, 'role_barisov'), exist_ok=True)
src = os.path.join(PD, 'lektor_natan_ref.wav')
if os.path.exists(src):
    shutil.copy2(src, os.path.join(out, 'role_barisov', 'ALT_pocket_lektor_natan_ref.wav'))
src = os.path.join(REF, 'rhvoice_natan_reference.wav')
if os.path.exists(src):
    shutil.copy2(src, os.path.join(out, 'role_barisov', 'ALT_rhvoice_natan_reference.wav'))

print('ok')
