"""TTS 409 MP (osobny kawalek PR, ale jedziemy od razu). Dawca = wlasny oryginal EN.
Temp 0.3, chunki zdaniowe, normalizacja -1dBFS PCM16. Flaga REVIEW przy tempie >2.0."""
import sys, os, json, glob
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices'
ENB = 'C:/Program Files (x86)/Steam/steamapps/common/Singularity/RvGame/CookedPC'
OUT = BASE + '/out/mp'
os.makedirs(OUT, exist_ok=True)

mp = json.load(open(BASE + '/mp-pl.json', encoding='utf-8'))
mpl = json.load(open(BASE + '/mp-speech.json', encoding='utf-8'))
pkg_of = {}
for r in mpl:
    pkg_of[(r['package'], r['asset'])] = r
print('zadan:', len(mp), flush=True)

import miniaudio, array as pyarray

OGG_CACHE = {}

def find_ogg(pkg, asset):
    key = (pkg, asset)
    if key in OGG_CACHE:
        return OGG_CACHE[key]
    # dekoduj z pakietu .xxx przez .u? NIE - uzyj story_en_reference lub umodel export.
    # MP ogg: sprobuj umodel export pojedynczego obiektu do wav przez raw API? prosciej: decode z .xxx niemozliwe bez .u.
    # Rozwiazanie: dekoduj .u on-demand i wyciagnij PCM bulks().
    OGG_CACHE[key] = None
    return None

print('TRYB: dawcy z pakietow MP wymagaja dekodowania .u per linia - sprawdzam probke', flush=True)
sys.path.insert(0, BASE + '/../../tools/audio')
os.chdir('C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices')
from tools.audio.ue3_sound_package import inspect as _insp
from tools.uedecompress import unpack_chunks as _u
print('importy narzedzi OK - pelny batch w nastepnym kroku', flush=True)
