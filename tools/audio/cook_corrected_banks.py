"""Cook corrected xlingual banks for Singularity VR (MAIN).

Rebuilds sound packages that contain newly resynthesized audio from Side.
Verifies container roundtrip integrity for each candidate.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

# Add tools/audio to sys.path
SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.append(str(SCRIPT_DIR))

from prepare_xlingual_banks import build_one, sha, unpack_physical
from patch_opening_dialogue import require

TARGET_PACKAGES = [
    'SP_VI5_kismet03_LOC_INT',
    'sp_freight_script_train_intro_LOC_INT',
    'SP_RT1_Intro_LOC_INT',
    'SP_RT1_kismet0_LOC_INT',
    'SP_RT1_kismet_LOC_INT',
    'SP_RT1_zone0_BT_LOC_INT',
    'SP_RT1_fx01_LOC_INT',
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cooked', type=Path,
                    default=Path(r'C:\Users\Trilex\vr-mods\SingularityVR-recovery\deployment\hands-dubbing\xlingual-banks\full\cooked'))
    ap.add_argument('--logical', type=Path,
                    default=Path(r'C:\Users\Trilex\vr-mods\SingularityVR-recovery\deployment\hands-dubbing\xlingual-banks\full\logicals'))
    ap.add_argument('--xlingual', type=Path,
                    default=Path(r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual'))
    ap.add_argument('--pins', type=Path,
                    default=SCRIPT_DIR / 'xlingual_bank_pins.json')
    ap.add_argument('--out', type=Path,
                    default=Path(r'deployment\cooked-banks-update'))
    ap.add_argument('--allow-partial', action='store_true', default=True)
    ap.add_argument('--allow-pending-qa', action='store_true', default=True)
    ap.add_argument('--ffmpeg', type=str, default=None)
    args = ap.parse_args()

    if not args.ffmpeg:
        # Auto-detect ffmpeg
        where_ffmpeg = shutil_which('ffmpeg')
        if not where_ffmpeg:
            raise RuntimeError("ffmpeg not found in PATH")
        args.ffmpeg = where_ffmpeg

    ffmpeg = str(args.ffmpeg)

    def run(argv):
        return subprocess.run([ffmpeg, '-hide_banner', '-nostats', *argv],
                              capture_output=True, text=True, check=True).stderr

    xl = {}
    for line in (args.xlingual / 'manifest_xlingual.jsonl').read_text(encoding='utf-8-sig').splitlines():
        if line.strip():
            r = json.loads(line)
            xl[(r.get('package'), r.get('asset'))] = r

    recall = {}
    for qf in ['ASR_QA.json', 'ASR_QA_runda1.json']:
        p = args.xlingual / qf
        if p.exists():
            for e in json.loads(p.read_text(encoding='utf-8-sig')):
                recall[e['file']] = e.get('recall')

    qaflags = {}
    p = args.xlingual / 'QA_FLAGS.json'
    if p.exists():
        qaflags = json.loads(p.read_text(encoding='utf-8-sig'))
    flagged_files = {f for cat in qaflags.values() if isinstance(cat, list)
                     for row in cat for f in [row[0]] if isinstance(row, list) and row}

    pins = json.loads(args.pins.read_text(encoding='utf-8-sig'))
    stock_map = {pkg: v['stock'] for pkg, v in pins.items()}
    logical_map = {pkg: v['logical'] for pkg, v in pins.items()}

    args.out.mkdir(parents=True, exist_ok=True)

    report = []
    print(f"Cooking {len(TARGET_PACKAGES)} corrected audio packages...")
    for package in TARGET_PACKAGES:
        require(package in stock_map, f"Unknown package pin: {package}")
        source_sha = stock_map[package]
        logical_sha = logical_map[package]
        try:
            res = build_one(args, xl, recall, flagged_files, run,
                            package, source_sha, logical_sha)
            report.append(res)
            print(f"SUCCESS: {package} -> candidate {res['candidate_sha256'][:16]} (lines={len(res['lines'])}, retained={len(res['skipped'])})")
        except Exception as ex:
            print(f"FAILED: {package}: {ex}", flush=True)
            report.append(dict(package=package, status='FAILED: ' + str(ex)[:200],
                               lines=[], skipped=[]))

    report_path = args.out / 'bank-report.json'
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
    print(f"Wrote report to {report_path}")
    return 0


def shutil_which(cmd):
    import shutil
    return shutil.which(cmd)


if __name__ == '__main__':
    sys.exit(main())
