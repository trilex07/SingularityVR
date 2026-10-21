"""Read-only exact package/asset coverage; staged WAVs are not native delivery.

Only explicit native-bank replacement receipts count. Does not infer which line
was heard, or certify installed hashes, voice quality or physical acceptance.
"""
import argparse
import json
from collections import Counter
from pathlib import Path


def audit(rows, reports, speaker):
    def identity(package, asset):
        if not isinstance(package, str) or not package or not isinstance(asset, str) or not asset:
            raise ValueError('missing native identity')
        return package.casefold(), asset.casefold()

    selected = {}
    for row in rows:
        if row.get('speaker_guess', '').casefold() != speaker.casefold():
            continue
        key = identity(row.get('package'), row.get('asset'))
        if key in selected:
            raise ValueError('duplicate lookup identity: ' + str(key))
        selected[key] = row
    receipts = {}
    for report in reports:
        for bank in report:
            for disposition, entries in (('replaced', bank['lines']), ('retained', bank['retained'])):
                for entry in entries:
                    key = identity(bank['package'], entry.get('asset'))
                    if key in receipts:
                        raise ValueError('overlapping native receipts: ' + str(key))
                    receipts[key] = (disposition, entry)
    result = []
    for key, row in sorted(selected.items()):
        disposition, receipt = receipts.get(key, ('not-covered', {}))
        result.append(dict(package=row['package'], asset=row['asset'], text_en=row.get('text_en'),
                           disposition=disposition, reason=receipt.get('reason'),
                           tempo=receipt.get('tempo'), source_duration=receipt.get('source_duration'),
                           native_duration=receipt.get('native_duration')))
    return dict(speaker=speaker, counts=dict(Counter(r['disposition'] for r in result)),
                lines=result, evidence='offline receipt coverage, not live playback proof')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--lookup', type=Path, required=True)
    ap.add_argument('--report', type=Path, action='append', required=True)
    ap.add_argument('--speaker', default='kathryn')
    a = ap.parse_args()
    rows = [json.loads(s) for s in a.lookup.read_text(encoding='utf-8-sig').splitlines() if s.strip()]
    reports = [json.loads(p.read_text(encoding='utf-8-sig')) for p in a.report]
    print(json.dumps(audit(rows, reports, a.speaker), ensure_ascii=True, indent=2))


if __name__ == '__main__':
    main()
