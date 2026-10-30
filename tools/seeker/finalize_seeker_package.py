"""Copy validated Seeker runtime caches into the private handoff and hash it."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

from validate_seeker_package import validate


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(worktree, private):
    handoff = private / 'final_handoff'
    runtime = handoff / 'runtime'
    runtime.mkdir(parents=True, exist_ok=True)
    for name in ('seeker_reload.svwa', 'seeker_clip.svrk', 'seeker_clip.svrt', 'animation-report.json'):
        shutil.copy2(private / 'prepared' / name, runtime / name)
    dll = worktree / 'spikes/view_matrix/d3d9.dll'
    if dll.is_file():
        shutil.copy2(dll, runtime / 'd3d9.offline-candidate.dll')
        shutil.copy2(worktree / 'spikes/view_matrix/d3d9.build.json', runtime / 'd3d9.build.json')
    bridge=private/'texture-build/dinput8.dll'
    if bridge.is_file():
        shutil.copy2(bridge,runtime/'dinput8.offline-candidate.dll')
        shutil.copy2(private/'texture-build/dinput8.build.json',runtime/'dinput8.build.json')
    for name in ('AR_Steerable_d.dds','AR_Steerable_n.dds'):
        shutil.copy2(handoff/'native_textures'/name,runtime/name)
    shutil.copy2(private/'native_identity/identity.json',handoff/'source/native_texture_identity.json')
    reports=handoff/'validation_reports'
    reports.mkdir(exist_ok=True)
    for name in ('texture-build.log','main-bridge-tests.log','seeker-tests.log','seeker-native-tests.log','all-reload-tests.log','build.log','blender-validation.log'):
        if (private/name).is_file():shutil.copy2(private/name,reports/name)
    shutil.copy2(worktree/'docs/handoffs/seeker-native-texture-preload.md',handoff/'MAIN_INTEGRATION.md')
    gates = validate(handoff)
    (handoff / 'validation.json').write_text(json.dumps(gates, indent=2) + '\n')
    manifest = {str(p.relative_to(handoff)).replace('\\', '/'): sha(p)
                for p in sorted(handoff.rglob('*')) if p.is_file()
                and p.name != 'sha256_manifest.json'}
    (handoff / 'sha256_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'handoff': str(handoff), 'files': len(manifest), 'validation': gates}, indent=2))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--worktree', type=Path, required=True)
    p.add_argument('--private', type=Path, required=True)
    a = p.parse_args()
    main(a.worktree.resolve(), a.private.resolve())
