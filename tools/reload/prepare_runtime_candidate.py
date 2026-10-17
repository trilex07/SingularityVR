"""Build an OFFLINE handoff folder, never deploy or change an installed INI.

Requires a clean /O2 x86 PE32 build of THIS checkout and the measured private
original caches. Output is restricted to this checkout's ignored build tree.
MAIN must integrate source with its current accepted build, not blindly replace
that build with a SIDE DLL. The settings file is a proposal, NOT an installed INI.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
NAMES = ('valkyrie', 'volk', 'kasimov')
# Same complete-cache witnesses as weapon_reload_runtime.h, SHA-256 is recorded
# in the output manifest; originals are private, never committed to this repo.
EXPECTED = {
    'valkyrie': (1866531932, 1387338662, 2571601141),
    'volk': (2782548863, 789788386, 2003765126),
    'kasimov': (2750451985, 3422279735, 543706490),
}


def fnv(data):
    value = 2166136261
    for byte in data: value = ((value ^ byte)*16777619) & 0xffffffff
    return value


def checked_output(path):
    path = path.resolve()
    allowed = (ROOT/'build/next-weapon-reload').resolve()
    if path == allowed or not path.is_relative_to(allowed):
        raise ValueError('output must be a NEW child of this checkout/build/next-weapon-reload')
    if path.exists(): raise ValueError('output already exists; no overwrite')
    return path


def pe32(data):
    if len(data) < 256 or data[:2] != b'MZ': raise ValueError('not PE')
    off = struct.unpack_from('<I', data, 60)[0]
    if off+26 > len(data) or data[off:off+4] != b'PE\0\0': raise ValueError('invalid PE offset')
    if struct.unpack_from('<H', data, off+4)[0] != 0x14c or struct.unpack_from('<H', data, off+24)[0] != 0x10b:
        raise ValueError('not x86 PE32')


def build_receipt(receipt, data, head, status):
    pe32(data)
    if status or receipt['workingTreeStatus']:
        raise ValueError('clean source and clean build manifest required')
    if receipt['commit'] != head or receipt['optimization'] != '/O2' or receipt['architecture'] != 'x86 PE32':
        raise ValueError('build provenance differs from HEAD/required optimized architecture')
    if receipt['performanceInstrumentation']:
        raise ValueError('diagnostic performance build is not this candidate')
    if hashlib.sha256(data).hexdigest().upper() != receipt['sha256']:
        raise ValueError('DLL no longer matches build manifest')


def prepare(prepared, output):
    output = checked_output(output)
    prepared = prepared.resolve()
    if not prepared.is_relative_to((ROOT/'build/next-weapon-reload').resolve()):
        raise ValueError('prepared assets must be inside this SIDE checkout build tree')
    git = lambda *args: subprocess.check_output(['git', '-C', str(ROOT), *args], text=True).strip()
    head = git('rev-parse', 'HEAD'); status = git('status', '--porcelain')
    dll = ROOT/'spikes/view_matrix/d3d9.dll'
    receipt = json.loads(dll.with_suffix('.build.json').read_text(encoding='utf-8-sig'))
    binary = dll.read_bytes();build_receipt(receipt, binary, head, status)
    staged = [(Path('d3d9.dll'), dll), (Path('d3d9.build.json'), dll.with_suffix('.build.json'))]
    for name in NAMES:
        for suffix, expected in zip(('reload.svwa', 'clip.svrk', 'clip.svrt'), EXPECTED[name]):
            source = prepared/name/(name+'_'+suffix)
            if fnv(source.read_bytes()) != expected: raise ValueError('wrong original cache: '+str(source))
            staged.append((Path('vr-assets/reload')/name/source.name, source))
    # Verify everything before the first output write. No existing folder/file
    # is replaced and the installed game is not discovered or accessed.
    output.mkdir(parents=True)
    files = []
    for relative, source in staged:
        destination = output/relative;destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)
        digest = hashlib.sha256(destination.read_bytes()).hexdigest()
        if digest != hashlib.sha256(source.read_bytes()).hexdigest(): raise ValueError('copy mismatch')
        files.append(dict(path=relative.as_posix(), sha256=digest, bytes=destination.stat().st_size))
    settings = dict(section='Input', preserve_other_keys=True, defaults_all_off=True,
                    test_one_weapon_per_session=True, required={'ReloadControlLayout': 1},
                    opt_in={'ValkyrieManualReload': 0, 'VolkManualReload': 0, 'KasimovManualReload': 0,
                            'ReloadNativeMagazine': 0})
    (output/'settings-proposal.json').write_text(json.dumps(settings, indent=2)+'\n', encoding='utf-8')
    manifest = dict(scope='OFFLINE SIDE CANDIDATE; NOT installed/accepted or L3/L4', source_commit=head,
                    source_branch=git('branch', '--show-current'), base_main=git('rev-parse', 'origin/main'),
                    warning='MAIN integrates source into current accepted dependency profile; no blind SIDE DLL deployment',
                    files=files, settings='settings-proposal.json', live_tests='UNKNOWN', proprietary_assets='local only; do not publish')
    (output/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    return manifest


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--prepared-root', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args();print(json.dumps(prepare(args.prepared_root, args.output), indent=2))
