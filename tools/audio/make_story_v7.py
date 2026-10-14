"""Generate v7 story manifest: v4 base + native_replaced flags for new banks.

Line-level splice (no re-serialization): every other byte preserved.
Verifies each edited line parses and exposes the flag.
"""
import json
import hashlib
import os

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
V4 = os.path.join(REPO, 'deployment', 'hands-dubbing', 'recovery-ready', 'assets',
                  'payload', 'Binaries', 'vr-assets', 'localization', 'pl',
                  'polish_story_voice.jsonl')
FULLREP = os.path.join(REPO, 'deployment', 'hands-dubbing', 'xlingual-banks', 'full', 'out',
                       'xlingual-banks.json')
OUTDIR = os.path.join(REPO, 'deployment', 'hands-dubbing', 'xlingual-banks', 'full', 'out', 'story')
os.makedirs(OUTDIR, exist_ok=True)

full = json.load(open(FULLREP, encoding='utf-8-sig'))
banked = {(b['package'], l['asset']) for b in full
          if b.get('status', '').startswith('OFFLINE') for l in b['lines']}
print('banked lines:', len(banked), flush=True)

out_lines = []
flagged = 0
for raw in open(V4, encoding='utf-8-sig'):
    if not raw.strip():
        continue
    r = json.loads(raw)
    key = (r.get('package'), r.get('asset'))
    if key in banked and str(r.get('native_replaced')) != '1':
        assert '"native_replaced"' not in raw, 'unexpected existing flag'
        stripped = raw.rstrip('\r\n')
        assert stripped.endswith('}'), 'line shape'
        raw = stripped[:-1] + ',"native_replaced":"1"}\n'
        check = json.loads(raw)
        assert str(check.get('native_replaced')) == '1', 'flag missing after splice'
        flagged += 1
    out_lines.append(raw)

outp = os.path.join(OUTDIR, 'polish_story_voice.jsonl')
open(outp, 'w', encoding='utf-8', newline='').writelines(out_lines)
print('flagged new:', flagged, flush=True)
print('v7 sha:', hashlib.sha256(open(outp, 'rb').read()).hexdigest()[:16], flush=True)
