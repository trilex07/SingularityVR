"""Seal verified offline outputs into a narrow MAIN-owned session delta.

Reads the installed baseline but never writes to it. Current native banks are
the source of truth; no stock-bank reset, invented cue metadata or broad install.
The result is testable, NOT physical acceptance of audio quality or GPU assets.
"""
import argparse
import json
from pathlib import Path
import re

from prepare_session_assets import ROOT, require, sha


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def identity(row):
    return row.get('package', '').lower(), row.get('asset', '')


def merge_story(original, banks):
    """Preserve unrelated/retained lines byte-for-byte, especially UI and MP.

    Native ownership is claimed only for a roundtripped replacement. Native-only
    barks have no standalone WAV overlay: their producer is SoundNodeWave.
    """
    replacements = {}
    for bank in banks:
        for line in bank['lines']:
            key = bank['package'].lower(), line['asset']
            require(key not in replacements, 'duplicate native wave ownership')
            replacements[key] = line
    output, seen = [], set()
    for raw in original.splitlines(keepends=True):
        if not raw.strip():
            output.append(raw)
            continue
        row = json.loads(raw)
        key = identity(row)
        require(key not in seen, 'duplicate baseline story identity')
        seen.add(key)
        line = replacements.pop(key, None)
        if line is None:
            output.append(raw)
            continue
        require(key[0].startswith('sp_'), 'not a campaign wave')
        row.update(text_en=line['english'], native_replaced='1')
        if line['polish']:
            row.update(text_pl=line['polish'], tts_text=line['polish'])
        # File ownership never migrates from barks/intro into story playback.
        if line['pack'] == 'story':
            row['file'] = line['file']
        output.append(json.dumps(row, ensure_ascii=False)+'\n')
    if output and not output[-1].endswith(('\n', '\r')):
        output[-1] += '\n'
    for (package, asset), line in sorted(replacements.items()):
        require(package.startswith('sp_') and line['english'], 'missing exact native English identity')
        output.append(json.dumps(dict(package=package, asset=asset,
            id=package+'|'+asset, text_en=line['english'], text_pl=line['polish'],
            tts_text=line['polish'], file='', native_replaced='1'), ensure_ascii=False)+'\n')
    return ''.join(output)


def assemble(a):
    root = (ROOT/'build/next-weapon-reload').resolve()
    out = a.out.resolve()
    require(out.is_relative_to(root) and out != root and not out.exists(), 'fresh private output required')
    intake = read_json(a.intake/'intake.json')
    require(intake['schema'] == 'singularityvr-session-assets-intake-v1', 'unknown intake')
    # Later retry reports may replace a FAILED package only. Never silently
    # choose between competing successful candidates or duplicate identities.
    reports = {}
    for report_path in a.reports:
        for row in read_json(report_path):
            name = row['package']
            require(re.fullmatch(r'sp_[a-z0-9_]+_loc_int', name), 'invalid bank identity')
            if name in reports:
                require(reports[name][0]['status'] == 'FAILED' and row['status'] != 'FAILED', 'competing bank reports')
            reports[name] = row, report_path.parent/name
    require(reports and all(r['status'] != 'FAILED' for r, _ in reports.values()), 'unresolved native cook failure')
    out.mkdir(parents=True)
    files = []
    used = set()

    def add(relative, source, expected, before=None):
        require(relative.lower() not in used and re.fullmatch(r'[A-Za-z0-9_./-]+', relative), 'invalid/duplicate payload target')
        used.add(relative.lower())
        data = source.read_bytes()
        require(sha(data) == expected, 'prepared source drift: '+str(source))
        target = a.game/relative
        old = sha(target.read_bytes()) if target.is_file() else None
        if before is not None:
            require(old == before, 'installed baseline no longer matches prepared source: '+relative)
        if old == expected:
            return
        dest = out/'payload'/relative
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
        require(sha(dest.read_bytes()) == expected, 'payload copy drift')
        files.append(dict(path=relative, before=old, after=expected))

    text_relative = 'Binaries/vr-assets/localization/pl/polish_text.jsonl'
    add(text_relative, a.intake/'payload'/text_relative, intake['text_after_sha256'], intake['text_before_sha256'])
    for row in intake['textures']:
        relative = 'Binaries/texture-override-remaster/'+row['file']
        add(relative, a.intake/'payload'/relative, row['sha256'])
    # Drop-in story source library: no UI/MP/Bink files, no generated audio.
    for row in intake['audio']:
        if row['pack'] == 'story':
            add('Binaries/vr-assets/audio/polish/story/'+row['file'],
                a.intake/'audio-sources/story'/row['file'], row['staged_sha256'])
    passed, retained, report_rows = [], [], []
    for name, (bank, folder) in sorted(reports.items()):
        report_rows.append(bank)
        retained += [dict(package=name, **r) for r in bank['retained']]
        if bank['status'] == 'UNCHANGED_NO_ELIGIBLE_TAKES':
            continue
        require(bank['status'] == 'OFFLINE_PASS_RUNTIME_AUDITORY_PENDING' and bank['native_decoder_roundtrip'],
                'native decoder evidence absent')
        baseline = sha((folder/'baseline.xxx').read_bytes())
        require(baseline == bank['before_sha256'], 'bank snapshot drift')
        add('RvGame/CookedPC/'+name.removesuffix('_loc_int')+'_LOC_INT.xxx', folder/'candidate.xxx', bank['candidate_sha256'], baseline)
        passed.append(bank)
    # Case-insensitive Windows package target; use the declared native name,
    # not a guessed path/case from a caller-controlled manifest.
    story = a.game/'Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'
    merged = merge_story(story.read_text(encoding='utf-8-sig'), passed).encode('utf-8')
    merged_source = out/'merged-story.jsonl'
    merged_source.write_bytes(merged)
    add('Binaries/vr-assets/localization/pl/polish_story_voice.jsonl', merged_source, sha(merged))
    profile = read_json(a.profile)
    require(profile['approved'] and profile['schema'] == 'singularityvr-accepted-profile-v1', 'unapproved baseline profile')
    protected = {}
    # Renderer/INI and reload assets have a distinct guarded lease. NPC RvGame
    # has a reviewed exact-pair adapter. This transaction must not own them.
    separate = {'binaries/d3d9.dll', 'binaries/singularityvr.ini', 'rvgame/cookedpc/rvgame.xxx'}
    for r in profile['files']:
        relative = r['path']
        if relative.lower() in separate or relative.lower() in used:
            continue
        data = (a.game/relative).read_bytes()
        require(sha(data) == r['sha256'].lower(), 'accepted dependency drift: '+relative)
        protected[relative] = sha(data)
    for relative in ('Binaries/dinput8.dll', 'Binaries/texture_override.ini',
                     'RvGame/CookedPC/UI_SinglePlayer_SF.xxx', 'RvGame/CookedPC/ui_GFx_fonts.xxx',
                     'Binaries/vr-assets/localization/pl/polish_voice.jsonl'):
        require(relative.lower() not in used, 'overlapping protected dependency')
        protected[relative] = sha((a.game/relative).read_bytes())
    manifest = dict(schema='singularityvr-session-assets-delta-v1',
        status='OFFLINE_ASSET_PASS_STEAM_AND_PHYSICAL_PENDING',
        source_intake_sha256=sha((a.intake/'intake.json').read_bytes()),
        report_sha256=[sha(p.read_bytes()) for p in a.reports],
        files=sorted(files, key=lambda r:r['path']),
        protected=[dict(path=k, sha256=v) for k, v in sorted(protected.items())],
        native_replaced=sum(len(r['lines']) for r in passed), native_retained=len(retained),
        notes=104, enemy_8k='Revert diffuse+normal only; other enemies have no 8K delivery',
        launch_window_seconds=7200, active_session_timeout_seconds=None,
        restore='after normal exit +30s quiet, never while running')
    (out/'asset-manifest.json').write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding='utf-8')
    (out/'native-coverage.json').write_text(json.dumps(dict(banks=report_rows, retained=retained), indent=2, ensure_ascii=False), encoding='utf-8')
    print('OFFLINE sealed:', len(files), 'changed files;', manifest['native_replaced'], 'native takes;', len(retained), 'baseline takes retained')
    print('No Steam writes. Native quality, texture upload/appearance and frame budget require MAIN test.')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ('intake', 'game', 'profile', 'out'):
        ap.add_argument('--'+name, type=Path, required=True)
    ap.add_argument('--reports', type=Path, nargs='+', required=True)
    assemble(ap.parse_args())
