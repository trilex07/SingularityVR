"""Join explicit human/agent semantic verdicts to exact native property evidence.

This is not an automatic translation judge. Private source texts/voice packs are
never fixtures or emitted into tracked documentation by this tool.
"""
import argparse
import json
from pathlib import Path
from prepare_timed_voice_manifest import inventory, digest

ap = argparse.ArgumentParser()
ap.add_argument('--bank-report',type=Path,required=True)
ap.add_argument('--logical-directory',type=Path,required=True)
ap.add_argument('--old-manifest',type=Path,required=True)
ap.add_argument('--verdicts',type=Path,required=True)
ap.add_argument('--out',type=Path,required=True)
args = ap.parse_args()
if args.out.exists():
    raise ValueError('refuse existing output')
old = {r['line_id']:r for r in map(json.loads,args.old_manifest.read_text(encoding='utf-8').splitlines())}
verdicts = json.loads(args.verdicts.read_text(encoding='utf-8'))
rows = []
for bank in json.loads(args.bank_report.read_text(encoding='utf-8')):
    natives = {r['asset']:r for r in inventory(args.logical_directory/(bank['package']+'.u'))}
    for line in bank['lines']:
        row = dict(natives[line['asset']])
        previous = old[row['line_id']]
        verdict = verdicts[row['asset']]
        if verdict['status'] not in ('FAITHFUL_TEXT_REVIEW','NEEDS_CORRECTION','REVIEW_CONTEXT') or not verdict['note']:
            raise ValueError('invalid explicit semantic verdict')
        row.update(verdict,old_source_english=previous['source_english'],
                   old_polish=previous['corrected_polish'],candidate_wav_sha256=digest(Path(previous['file'])),
                   review_level='SIDE_semantic_text_review_not_auditory')
        rows.append(row)
args.out.parent.mkdir(parents=True,exist_ok=True)
args.out.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
from collections import Counter
print(json.dumps(dict(count=len(rows),statuses=dict(Counter(r['status'] for r in rows)),
    source_manifest_sha256=digest(args.old_manifest),verdicts_sha256=digest(args.verdicts))))
