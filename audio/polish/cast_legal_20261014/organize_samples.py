import os, shutil, io

base = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014'
dl = 'C:/Users/Trilex/Downloads'
mus = 'C:/Users/Trilex/Music'
log = io.open(os.path.join(base, 'organize_log.txt'), 'w', encoding='utf-8')

# 1. User voice files
udir = os.path.join(base, 'user_voice')
os.makedirs(udir, exist_ok=True)
names = {}
for f in os.listdir(dl):
    if f.endswith('.m4a'):
        src = os.path.join(dl, f)
        dst = os.path.join(udir, f)
        shutil.copy2(src, dst)
        names[f] = os.path.getsize(dst)
log.write('user_voice:\n')
for k, v in names.items():
    log.write(f'  {k} ({v} B)\n')

# 2. Intro voice
idir = os.path.join(base, 'intro_voice')
os.makedirs(idir, exist_ok=True)
shutil.copy2(os.path.join(mus, 'SNG_intro_polish_voice.wav'), os.path.join(idir, 'SNG_intro_polish_voice_FULL_123s.wav'))
for f in os.listdir(os.path.join(mus, 'intro_samples')):
    shutil.copy2(os.path.join(mus, 'intro_samples', f), os.path.join(idir, f))
log.write('intro_voice:\n')
for f in sorted(os.listdir(idir)):
    log.write(f'  {f} ({os.path.getsize(os.path.join(idir, f))} B)\n')
log.write('DONE\n')
log.close()