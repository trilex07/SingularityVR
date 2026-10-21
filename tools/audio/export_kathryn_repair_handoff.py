"""Export private SIDE repair CSVs from exact lookup/native delivery receipts.

No audio generation, installation, duration metadata changes or game access.
Private dialogue text must not be committed to Git.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path

from audit_native_voice_coverage import audit


def repair_rows(lookup, reports):
    coverage = audit(lookup, reports, 'kathryn')
    by_key = {(r['package'].casefold(), r['asset'].casefold()): r for r in lookup
              if r.get('speaker_guess', '').casefold() == 'kathryn'}
    receipts = {(b['package'].casefold(), r['asset'].casefold()): r
                for report in reports for b in report for r in b['retained']}
    result = []
    for item in coverage['lines']:
        if item['disposition'] == 'replaced':
            continue
        key = item['package'].casefold(), item['asset'].casefold()
        row, receipt = by_key[key], receipts.get(key, {})
        native = receipt.get('native_duration')
        if native is None and receipt.get('rate') and receipt.get('native_frames'):
            native = receipt['native_frames'] / receipt['rate']
        reason = item['reason'] or ('tempo exceeds integration ceiling 1.35'
                                   if item['disposition'] == 'retained' else 'native bank not covered')
        regenerate = item['disposition'] == 'retained' and 'encoded native PCM budget mismatch' not in reason
        # Same fitter contract: retain 20 ms inside the native frame window.
        # Prefer an unaccelerated performance. 1.35 is an integration ceiling,
        # not a request to generate at that speed or to trim speech.
        preferred = max(0, native - .020) if native is not None else None
        result.append(dict(
            priority='1-RT1' if item['package'].casefold().startswith('sp_rt1') else '2-other',
            action='regenerate-shorter-natural-take' if regenerate else 'MAIN-technical-review-no-regeneration-required',
            package=row['package'], asset=row['asset'], file=row['file'],
            text_en=row.get('text_en', ''), text_pl=row.get('text_pl', ''),
            native_window_s=native, preferred_guarded_take_max_s=preferred,
            integration_guarded_take_absolute_max_s=preferred * 1.35 if preferred is not None else None,
            previous_guarded_take_s=receipt.get('source_duration'),
            required_tempo=receipt.get('tempo'), reason=reason))
    return sorted(result, key=lambda r: (r['priority'], r['package'], r['asset']))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--lookup', type=Path, required=True)
    ap.add_argument('--report', type=Path, action='append', required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    root = Path(__file__).resolve().parents[2] / 'build/combined-followups'
    if not a.out.resolve().is_relative_to(root.resolve()) or a.out.exists():
        ap.error('output must be a NEW private directory under this checkout/build/combined-followups')
    rows = [json.loads(s) for s in a.lookup.read_text(encoding='utf-8-sig').splitlines() if s.strip()]
    reports = [json.loads(p.read_text(encoding='utf-8-sig')) for p in a.report]
    result = repair_rows(rows, reports)
    if not result:
        ap.error('no repair rows')
    a.out.mkdir(parents=True)
    counts = {}
    for filename, action in (('kathryn-regenerate.csv', 'regenerate-shorter-natural-take'),
                             ('main-technical-review.csv', 'MAIN-technical-review-no-regeneration-required')):
        selected = [r for r in result if r['action'] == action]
        with (a.out / filename).open('w', encoding='utf-8-sig', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=list(result[0]))
            writer.writeheader()
            writer.writerows(selected)
        counts[filename] = len(selected)
    inputs = [a.lookup, *a.report]
    manifest = dict(counts=counts, inputs=[dict(path=str(p.resolve()),
        sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in inputs],
        evidence='offline receipt audit; heard line/scene identity not proven; private text, do not commit')
    (a.out / 'receipt.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(json.dumps(counts))


if __name__ == '__main__':
    main()
