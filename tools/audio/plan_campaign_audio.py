"""Exact current WAV -> existing native campaign identity plan. No approval/deploy.

Hashed story basenames join the existing lookup. Named skipped/vignette takes
require their explicit curated native identities, never speaker/filename guesses.
Competing takes remain choices; a temporary file cannot become a native wave.
"""
import argparse
from collections import Counter, defaultdict
import csv
import json
from pathlib import Path

from audio_release_gate import load, require, sha


def plan(audit, lookup, skipped, vignette):
    require(audit['dubbing']['status'] == 'TECHNICAL_PASS_APPROVAL_PENDING', 'inventory failed')
    by_file, by_identity = defaultdict(list), defaultdict(list)
    for row in lookup:
        by_file[row['file'].casefold()].append(row)
        by_identity[(row['package'].casefold(), row['asset'])].append(row)
    mapped, unmatched, noncampaign = [], [], []
    excluded = Counter()
    for wav in audit['dubbing']['files']:
        pack, filename = wav['pack'], wav['file']
        if pack not in ('story', 'skipped', 'vignette'):
            excluded[pack] += 1
            continue
        reason, reference = '', None
        if '.tmp.' in filename.casefold():
            hits, reason = [], 'TEMPORARY_ARTIFACT_NOT_A_NATIVE_TAKE'
        elif pack == 'story':
            hits = by_file[filename.casefold()]
            reference = 'existing lookup exact runtime filename'
        elif pack == 'skipped':
            asset = filename.removesuffix('.pl.wav')
            row = skipped.get(asset)
            hits = [] if row is None else [x for x in by_identity[(row['package'].casefold(), asset)]
                if x.get('source', '').casefold() == row['source'].casefold() and row['asset'] == asset]
            reference = 'curated skipped package/asset/source + existing lookup'
        else:
            asset = filename.removesuffix('.pl.wav')
            hits = by_identity[('sp_rt3_vignette_loc_int', asset)] if asset in vignette else []
            reference = 'curated vignette asset + exact known bank + existing lookup'
        if len(hits) != 1:
            unmatched.append(dict(id=pack+'/'+filename, sha256=wav['sha256'],
                reason=reason or ('NATIVE_IDENTITY_MISSING' if not hits else 'NATIVE_IDENTITY_AMBIGUOUS'),
                candidates=[dict(package=x['package'], asset=x['asset']) for x in hits]))
            continue
        row = hits[0]
        if not (row['package'].upper().startswith('SP_') and row['package'].upper().endswith('_LOC_INT')):
            noncampaign.append(dict(id=pack+'/'+filename, sha256=wav['sha256'],
                package=row['package'], asset=row['asset'], reason='NATIVE_IDENTITY_OUTSIDE_CAMPAIGN'))
            continue
        mapped.append(dict(id=pack+'/'+filename, sha256=wav['sha256'],
            package=row['package'], asset=row['asset'], runtime_file=row['file'],
            source=row.get('source'), identity_evidence=reference,
            frames=wav['frames'], sample_rate=wav['sample_rate'],
            approval='PENDING_NOT_GRANTED_BY_MAPPING'))
    identities = defaultdict(list)
    for row in mapped:
        identities[(row['package'].casefold(), row['asset'])].append(row)
    choices = []
    for key, rows in sorted(identities.items()):
        if len(rows) > 1:
            choices.append(dict(package=rows[0]['package'], asset=key[1],
                candidates=[dict(id=r['id'], sha256=r['sha256']) for r in rows],
                disposition='SELECT_EXACTLY_ONE_APPROVED_TAKE_NO_AUTOMATIC_OVERLAY'))
    package_groups = defaultdict(list)
    for rows in identities.values():
        package_groups[rows[0]['package']].append(rows[0]['asset'])
    banks = [dict(package=p, unique_native_waves=len(assets), assets=sorted(assets),
                  accepted_physical_sha256=None, accepted_logical_sha256=None,
                  bank_build='BLOCKED_PENDING_EXACT_ACCEPTED_BANK_INPUTS_AND_APPROVAL')
             for p, assets in sorted(package_groups.items(), key=lambda x:x[0].casefold())]
    uncovered = [dict(package=r['package'],asset=r['asset'],runtime_file=r['file'],
                      source=r.get('source'),reason='NO_CURRENT_WAV_FOR_EXISTING_NATIVE_IDENTITY')
                 for r in lookup if r['package'].upper().startswith('SP_')
                 and (r['package'].casefold(),r['asset']) not in identities]
    return dict(schema='singularityvr-campaign-audio-plan-v1',
        status='IDENTITY_PLAN_ONLY_NO_NATIVE_REPLACEMENTS',
        counts=dict(mapped_takes=len(mapped), unique_native_waves=len(identities),
                    required_banks=len(banks), choice_groups=len(choices), unmatched=len(unmatched),
                    excluded_native_noncampaign=len(noncampaign), uncovered_lookup_rows=len(uncovered)),
        excluded_packs=dict(excluded), excluded_native_noncampaign=noncampaign,
        mapped=mapped, choices=choices, unmatched=unmatched, uncovered_lookup_rows=uncovered,
        bank_inputs=banks, approval_granted=False, steam_accessed=False,
        physical_acceptance='NOT_RUN')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for key in ('audit', 'lookup', 'skipped', 'vignette', 'out'):
        ap.add_argument('--'+key, type=Path, required=True)
    args = ap.parse_args()
    require(not args.out.exists(), 'never overwrite identity plan')
    csv_path = args.out.with_suffix('.mapping.csv')
    require(not csv_path.exists(), 'never overwrite mapping CSV')
    lookup = [json.loads(s) for s in args.lookup.read_text(encoding='utf-8-sig').splitlines() if s.strip()]
    result = plan(load(args.audit), lookup, load(args.skipped), load(args.vignette))
    result['input_sha256'] = {k:sha(getattr(args,k)) for k in ('audit','lookup','skipped','vignette')}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding='utf-8')
    with csv_path.open('w',encoding='utf-8',newline='') as stream:
        fields=('id','sha256','package','asset','runtime_file','source','frames','sample_rate','identity_evidence','approval')
        writer=csv.DictWriter(stream,fieldnames=fields)
        writer.writeheader()
        writer.writerows(result['mapped'])
    print(json.dumps(result['counts']), 'plan_sha256='+sha(args.out))


if __name__ == '__main__':
    main()
