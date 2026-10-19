"""Link verified runtime, campaign assets and NPC package into ONE test plan.

No deployment, launch, timer or Steam discovery. Paths are explicit and local;
game-owned assets must not be published with this source repository.
"""
import argparse
import configparser
import json
from pathlib import Path
import subprocess
import sys

from prepare_session_assets import ROOT, require, sha
sys.path.insert(0, str(ROOT/'tools/reload'))
from prepare_runtime_candidate import build_receipt


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def seal(a):
    runtime, assets, npc = [p.resolve() for p in (a.runtime, a.assets, a.npc)]
    allowed = (ROOT/'build/next-weapon-reload').resolve()
    for folder in (runtime, assets, npc):
        require(folder != allowed and folder.is_relative_to(allowed), 'private candidate root required')
    target = runtime/'session-plan.json'
    require(not target.exists(), 'never overwrite a sealed plan')
    git = lambda *args: subprocess.check_output(['git', '-C', str(ROOT), *args], text=True).strip()
    head = git('rev-parse', 'HEAD')
    binary = (runtime/'d3d9.dll').read_bytes()
    build_receipt(read(runtime/'d3d9.build.json'), binary, head, git('status', '--porcelain'))
    runtime_manifest = read(runtime/'manifest.json')
    for row in runtime_manifest['files']:
        path = runtime/row['path']
        require(path.resolve().is_relative_to(runtime) and sha(path.read_bytes()) == row['sha256'], 'runtime payload drift')
    ini = configparser.ConfigParser(interpolation=None, strict=True)
    ini.read(runtime/'SingularityVR.ini', encoding='utf-8-sig')
    expected = {
        'Input': {'ReloadControlLayout':'1', 'ValkyrieManualReload':'1', 'VolkManualReload':'1',
                  'KasimovManualReload':'1', 'ReloadPalmGrip':'1', 'ReloadRefillOnly':'1',
                  'ReloadNativeMagazine':'1', 'WeaponTraceRigidOrigin':'1', 'ReloadHints':'0',
                  'ValkyrieAuthoredGrip':'1', 'ValkyrieSingleGrip':'1', 'VolkAuthoredGrip':'1',
                  'WeaponWheelObservedSelection':'1'},
        'Render': {'VrSettingsMinimal':'1', 'NativeMuzzleParticleStereo':'1', 'XrCaptureOpaqueAlpha':'1',
                   'DesktopMirrorWindow':'1', 'VrPromptTokens':'1'},
        'Diagnostic': {'VrSettingsSessionOnly':'1', 'NativeMuzzleProbe':'1'},
        'Localization': {'PolishLanguage':'1', 'NativePolishSpeech':'1'},
    }
    for section, values in expected.items():
        for key, value in values.items():
            require(ini.get(section, key, fallback=None) == value, 'missing combined-session opt-in: '+key)
    asset_manifest = assets/'asset-manifest.json'
    am = read(asset_manifest)
    require(am['schema'] == 'singularityvr-session-assets-delta-v1' and
            am['active_session_timeout_seconds'] is None and am['launch_window_seconds'] == 7200,
            'wrong asset scope/timer policy')
    for row in am['files']:
        path = assets/'payload'/row['path']
        require(path.resolve().is_relative_to(assets/'payload') and sha(path.read_bytes()) == row['after'], 'asset payload drift')
    nr = read(npc/'report.json')
    require(nr['native_decoder_roundtrip'] and nr['old_twips'] == 340 and nr['new_twips'] == 204
            and sha((npc/'RvGame.xxx').read_bytes()) == nr['candidate_sha256'].lower(), 'unverified NPC pair')
    profile = read(a.profile)
    require(profile['approved'], 'unapproved baseline profile')
    plan = dict(schema='singularityvr-combined-session-plan-v1', status='OFFLINE_READY_L3_L4_PENDING',
        branch=git('branch', '--show-current'), commit=head,
        runtime=str(runtime), runtime_manifest_sha256=sha((runtime/'manifest.json').read_bytes()),
        dll_sha256=sha(binary), ini_sha256=sha((runtime/'SingularityVR.ini').read_bytes()),
        change_plan_sha256=sha((runtime/'change-plan.json').read_bytes()),
        session_assets_manifest=str(asset_manifest), session_assets_manifest_sha256=sha(asset_manifest.read_bytes()),
        npc_candidate=str(npc), npc_before_sha256=nr['input_sha256'], npc_after_sha256=nr['candidate_sha256'],
        npc_report_sha256=sha((npc/'report.json').read_bytes()),
        accepted_profile=str(a.profile.resolve()), accepted_profile_sha256=sha(a.profile.read_bytes()),
        before_launch_seconds=7200, active_session_timeout_seconds=None, post_exit_quiet_seconds=30,
        requires_new_user_start=True, auto_launch=False, steam_written=False,
        unresolved=['Rifle VFX contributing draw: bounded idle/three-shot probe, not a claimed fix',
                    '380 native takes retained to preserve speech and sample/cue metadata',
                    'Real 8K upload/appearance/frame-time and combined physical regression pending'])
    target.write_text(json.dumps(plan, indent=2, ensure_ascii=False), encoding='utf-8')
    print('ONE combined test plan sealed:', target)
    print('DLL', plan['dll_sha256'], '; assets', len(am['files']), '; NO Steam writes/launch')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ('runtime', 'assets', 'npc', 'profile'):
        ap.add_argument('--'+name, type=Path, required=True)
    seal(ap.parse_args())
