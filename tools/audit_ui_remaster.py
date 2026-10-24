"""Read-only source/install UI inventory. Does not deploy or launch anything."""
from pathlib import Path
import argparse,hashlib,json,configparser
p=argparse.ArgumentParser();p.add_argument('--installed-binaries',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
r=Path(__file__).resolve().parents[1]
names=['ui_atlas_neutral.png','ui_atlas_localized_pl.png','ui_atlas_localized_en.png']
rows=[]
for name in names:
 src=r/'assets/processed/ui_remaster/atlas'/name
 dst=a.installed_binaries/'vr-assets/ui/remaster'/name if a.installed_binaries else None
 digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper() if p and p.is_file() else None
 rows.append(dict(file=name,source_sha256=digest(src),installed_sha256=digest(dst),installed_path=str(dst) if dst else None))
flags={}
if a.installed_binaries:
 ini=a.installed_binaries/'SingularityVR.ini'
 if ini.is_file():
  c=configparser.ConfigParser(strict=False);c.read(ini,encoding='utf-8-sig')
  for key in ['UiRemasterRuntime','UiRemasterSettings','UiRemasterWheel','UiRemasterMenus','VrPromptTokens','VrHideLegacyPadPrompts']:
   flags[key]=c.get('Render',key,fallback='absent')
result={'atlases':rows,'installed_flags':flags,
        'installed_dll_sha256':digest(a.installed_binaries/'d3d9.dll') if a.installed_binaries else None,
        'rendering_proven':False,'note':'File identity/config only, not a live binding or pixel receipt'}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result,indent=2))
