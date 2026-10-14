"""Generate the v6 install manifest for the previous Polish intro bank."""
import json
import hashlib
import os

CORE = r'C:\Users\Trilex\vr-mods\SingularityVR-recovery\deployment\hands-dubbing\recovery-ready\core'
GAME = r'C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\CookedPC\SP_VI_intro_Kismet_LOC_INT.xxx'

SEP = chr(92)
cs = hashlib.sha256(open(os.path.join(CORE, 'SP_VI_intro_Kismet_LOC_INT.xxx'), 'rb').read()).hexdigest().upper()
bs = hashlib.sha256(open(GAME, 'rb').read()).hexdigest().upper()
assert cs == '4904F2818B9CA9505AE636CB33ADE88C2B2E173AD1D2DB2A85BC43EDD49E1290', 'staged changed'
assert bs == '890C61720ABC0BB5AFAD86F16433A811A6F21BA6FC6284563BC5875E52642E85', 'installed not stock'
m = {'Kind': 'SingularityVR intro bank delta v6', 'Mode': 'MAIN-only indistinguishable-lease',
     'Files': [{'RelativePath': SEP.join(['RvGame', 'CookedPC', 'SP_VI_intro_Kismet_LOC_INT.xxx']),
                'Source': 'SP_VI_intro_Kismet_LOC_INT.xxx',
                'BaselineSha256': bs, 'CandidateSha256': cs}],
     'StartRequired': False, 'SteamWritePerformed': False, 'PhysicalAcceptance': 'PENDING',
     'FontEnabled': False,
     'Notes': 'previous Polish helicopter bank; known quiet; user-ordered interim until new intro voices exist'}
open(os.path.join(CORE, 'bank-manifest-intro.json'), 'w').write(json.dumps(m, indent=1))
print('V6 MANIFEST WRITTEN')
