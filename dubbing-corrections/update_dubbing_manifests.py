"""Aktualizacja manifestow wdrozeniowych dla MAIN:
- deploy_full_manifest.jsonl: 100% zsynchronizowany z dyskiem C:\Dubbing-aktualny (1949+ plikow, sha16/sha256).
- deploy_manifest.jsonl: Zestaw delta (pliki wygenerowane / zaktualizowane z dopasowaniem emocji i glosnosci).
- Raport metryk glosnosci EN vs PL.
"""
import os, json, hashlib

DUB_ROOT = r'C:\Dubbing-aktualny'
CORR_DIR = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections'
FULL_MANIFEST_PATH = os.path.join(CORR_DIR, 'deploy_full_manifest.jsonl')
DELTA_MANIFEST_PATH = os.path.join(CORR_DIR, 'deploy_manifest.jsonl')
LOG_PATH = os.path.join(CORR_DIR, 'matched_generation_log.jsonl')

def get_hashes(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()
    s256 = hashlib.sha256(data).hexdigest()
    return len(data), s256[:16], s256

def sync_manifests():
    print('Scanning C:\\Dubbing-aktualny to synchronize deploy_full_manifest.jsonl...', flush=True)
    all_records = []
    packs = ['story', 'barks', 'mp', 'skipped', 'intro', 'vignette']
    
    for pack in packs:
        pdir = os.path.join(DUB_ROOT, pack)
        if not os.path.exists(pdir):
            continue
        files = sorted(os.listdir(pdir))
        for f in files:
            if not f.lower().endswith('.wav'):
                continue
            fp = os.path.join(pdir, f)
            sz, sha16, sha256 = get_hashes(fp)
            all_records.append({
                'pack': pack,
                'file': f,
                'path': fp,
                'size_bytes': sz,
                'sha16': sha16,
                'sha256': sha256
            })
            
    print(f'Total staged files scanned: {len(all_records)}', flush=True)
    with open(FULL_MANIFEST_PATH, 'w', encoding='utf-8') as out:
        for r in all_records:
            out.write(json.dumps(r, ensure_ascii=False) + '\n')
    print(f'Updated {FULL_MANIFEST_PATH}', flush=True)

    # Now build deploy_manifest.jsonl (Delta for MAIN)
    print('Building deploy_manifest.jsonl (Delta set)...', flush=True)
    # Generated records from log
    gen_records = {}
    if os.path.exists(LOG_PATH):
        for line in open(LOG_PATH, encoding='utf-8'):
            if line.strip():
                try:
                    d = json.loads(line)
                    if 'file' in d:
                        gen_records[(d['pack'], d['file'])] = d
                except Exception:
                    pass
                    
    # Read existing story entries from deploy_manifest if any
    story_entries = []
    if os.path.exists(DELTA_MANIFEST_PATH):
        for line in open(DELTA_MANIFEST_PATH, encoding='utf-8'):
            if line.strip():
                try:
                    d = json.loads(line)
                    if d.get('pack') == 'story':
                        story_entries.append(d)
                except Exception:
                    pass
                    
    delta_records = []
    # 1. Story entries (159 files)
    for s in story_entries:
        fp = os.path.join(DUB_ROOT, 'story', s['target'])
        if os.path.exists(fp):
            sz, sha16, sha256 = get_hashes(fp)
            s['sha16'] = sha16
            s['src'] = fp
        delta_records.append(s)
        
    # 2. Newly generated packs: intro, vignette, skipped, barks
    for pack in ['intro', 'vignette', 'skipped', 'barks']:
        pdir = os.path.join(DUB_ROOT, pack)
        if not os.path.exists(pdir):
            continue
        files = sorted(os.listdir(pdir))
        for f in files:
            if not f.lower().endswith('.wav'):
                continue
            fp = os.path.join(pdir, f)
            sz, sha16, sha256 = get_hashes(fp)
            rec = {
                'pack': pack,
                'target': f,
                'src': fp,
                'sha16': sha16
            }
            # attach loudness metadata if available
            ginfo = gen_records.get((pack, f))
            if ginfo:
                rec['rms_en_db'] = ginfo.get('rms_en_db')
                rec['rms_pl_db'] = ginfo.get('rms_pl_db')
                rec['delta_db'] = ginfo.get('delta_db')
                rec['peak_pl_db'] = ginfo.get('peak_pl_db')
            delta_records.append(rec)
            
    with open(DELTA_MANIFEST_PATH, 'w', encoding='utf-8') as out:
        for r in delta_records:
            out.write(json.dumps(r, ensure_ascii=False) + '\n')
    print(f'Updated {DELTA_MANIFEST_PATH} with {len(delta_records)} delta files!', flush=True)

if __name__ == '__main__':
    sync_manifests()
