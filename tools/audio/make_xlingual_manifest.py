"""Generate the v5 install manifest for xlingual VI2 banks (deterministic)."""
import json
import hashlib
import os

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
XB = os.path.join(REPO, 'deployment', 'hands-dubbing', 'xlingual-banks', 'out')

BASELINES = {  # current Steam = accepted v4 candidates
    'SP_VI2_kismet01_LOC_INT': 'FB03741D672537CFC8117C8448361CFEABF819562B3955DADD7BBAD0945AECF8',
    'SP_VI2_kismet01_a_LOC_INT': 'D6EA4374FF67BB53BA67657A5317D61D9FB76266E185CA9AC2262EFCB2A313D3',
    'SP_VI2_kismet01_b_LOC_INT': 'A1A83F2EFF109A55BDC07A4E6FD475E102EB22380CEE8BDBB131544A447BCBCA',
    'SP_VI2_kismet02_LOC_INT': '1CAF1E3F8C6236E52609EB5AAFEA9DB7A65E14AF44996F912713E11AF519846B',
    'SP_VI2_kismet03_LOC_INT': 'A2E6B73D706C97390C7070E9E9E69DB0C4F5CB562DAF1A38A62ABDCC85BF6172',
    'SP_VI2_kismet04_LOC_INT': '56CDDB0265228D1F03857D648EE7AD25D02E48DC1351306350AD0BE1198A4623',
    'SP_VI2_zone02a_LOC_INT': '7D50BD2B225D0DD14EF00FB0A20AEF27782C7DD28C6738071D1D6301B94B2F30',
    'SP_VI2_zone02a_2010_LOC_INT': '1B2522CC7D0D1B9692A3705427438CFBE2B7AABAED666E40F2ED2F82291349ED',
}

report = json.load(open(os.path.join(XB, 'xlingual-banks.json'), encoding='utf-8-sig'))
files = []
for b in report:
    pkg = b['package']
    cand = os.path.join(XB, pkg, 'candidate.xxx')
    h = hashlib.sha256(open(cand, 'rb').read()).hexdigest().upper()
    assert h == b['candidate_sha256'].upper(), 'candidate changed: ' + pkg
    files.append({
        'RelativePath': 'RvGame\\CookedPC\\' + pkg + '.xxx',
        'Source': pkg + '\\candidate.xxx',
        'BaselineSha256': BASELINES[pkg],
        'CandidateSha256': b['candidate_sha256'].upper(),
    })
manifest = {
    'Kind': 'SingularityVR xlingual bank delta v5',
    'Mode': 'MAIN-only indistinguishable-lease',
    'Files': files,
    'StartRequired': False,
    'SteamWritePerformed': False,
    'PhysicalAcceptance': 'PENDING',
    'FontEnabled': False,
    'Notes': 'xlingual VI2 banks over accepted v4; story manifest v4 flags already cover all 78 banked lines; no overlay double-play',
}
out = os.path.join(XB, 'bank-manifest.json')
open(out, 'w', encoding='utf-8').write(json.dumps(manifest, indent=1))
print('WROTE', out, len(files), 'files')
