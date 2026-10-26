"""Offline, approval-bound audio release inventory and native-bank input staging.

Never reads or writes Steam. Existing native_story_package owns container edits;
this tool produces hash-pinned inputs, not a second audio playback mechanism.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import wave
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PRIVATE = ROOT / 'build/audio-release-private'
PACKS = ('story', 'skipped', 'vignette', 'intro', 'barks', 'mp')
REMASTER_IDS = (
    '01_weapon_centurion_revolver', '02_weapon_kasimov_sniper',
    '03_e99_tmd_impulse', '04_weapon_walkiria_ar9',
    '05_weapon_spikeshot_shotgun', '06_music_explore_drone',
    '07_music_combat_boss_grom', '08_ambience_ocean_storm',
)
VARIANTS = ('original.wav', 'restored_conservative.wav', 'remaster_enhanced.wav')


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def safe_leaf(name):
    return bool(re.fullmatch(r'[A-Za-z0-9_.-]+', name)) and name not in ('.', '..')


def wav_info(path):
    """Read all PCM frames: a plausible RIFF header is not format validation."""
    with wave.open(str(path), 'rb') as wav:
        require(wav.getcomptype() == 'NONE', 'compressed WAV refused')
        frames, channels, width, rate = (wav.getnframes(), wav.getnchannels(),
                                         wav.getsampwidth(), wav.getframerate())
        count, nonzero = 0, False
        while chunk := wav.readframes(65536):
            count += len(chunk)
            nonzero |= any(chunk)
    require(frames > 0 and channels > 0 and rate > 0, 'empty WAV')
    require(count == frames * channels * width, 'truncated PCM')
    require(nonzero, 'silent PCM')
    return dict(frames=frames, channels=channels, sample_width=width, sample_rate=rate)


def inventory_dubbing(root, manifest):
    records = [json.loads(s) for s in Path(manifest).read_text(encoding='utf-8-sig').splitlines() if s.strip()]
    seen, result, errors = set(), [], []
    for row in records:
        pack, name = row.get('pack'), row.get('file', '')
        require(pack in PACKS and safe_leaf(name) and name.lower().endswith('.wav'), 'unsafe dubbing identity')
        key = (pack, name.casefold())
        require(key not in seen, 'duplicate manifest identity')
        seen.add(key)
        source = Path(root) / pack / name
        entry = dict(pack=pack, file=name)
        try:
            require(not source.is_symlink(), 'symlink input refused')
            entry.update(wav_info(source), sha256=sha(source))
            require(entry['sha256'] == row.get('sha256', '').lower(), 'manifest SHA mismatch')
            require(source.stat().st_size == row['size_bytes'], 'manifest size mismatch')
            require((entry['channels'], entry['sample_width'], entry['sample_rate']) == (1, 2, 24000), 'not mono PCM16 24kHz')
            entry['status'] = 'TECHNICAL_PASS_APPROVAL_PENDING'
        except (ValueError, OSError, EOFError, wave.Error) as exc:
            entry['status'], entry['error'] = 'REFUSED', str(exc)
            errors.append(dict(pack=pack, file=name, error=str(exc)))
        result.append(entry)
    actual = {(p, f.name.casefold()): f for p in PACKS for f in (Path(root) / p).glob('*.wav')}
    extras = []
    for pack, name in sorted(actual.keys() - seen):
        source = actual[(pack, name)]
        entry = dict(pack=pack, file=source.name, sha256=sha(source))
        try:
            require(not source.is_symlink(), 'symlink input refused')
            entry.update(wav_info(source))
        except (ValueError, OSError, EOFError, wave.Error) as exc:
            entry['error'] = str(exc)
        extras.append(entry)
    return dict(manifest_sha256=sha(manifest), counts=dict(Counter(r['pack'] for r in records)),
                files=result, errors=errors, unmanifested_files=extras,
                status='REFUSED' if errors or extras else 'TECHNICAL_PASS_APPROVAL_PENDING')


def inventory_remaster(root):
    result = []
    for target in REMASTER_IDS:
        folder = Path(root) / target
        meta_file = folder / 'metadata.json'
        meta = load(meta_file)
        require(meta['id'] == target and meta['dir_name'] == target, 'remaster metadata identity mismatch')
        entry = {k: meta[k] for k in ('id', 'package', 'asset', 'cue', 'confidence')}
        entry['metadata_sha256'], entry['variants'] = sha(meta_file), []
        original = wav_info(folder / VARIANTS[0])
        for variant in VARIANTS:
            wav = folder / variant
            require(not wav.is_symlink(), 'symlink WAV refused')
            info = wav_info(wav)
            require(info == original, 'remaster changed native PCM sample/channel budget')
            entry['variants'].append(dict(file=variant, sha256=sha(wav), **info))
        entry['status'] = ('TECHNICAL_PASS_APPROVAL_PENDING' if meta['confidence'] == 'VERIFIED_REFERENCE'
                           else 'BLOCKED_NATIVE_GRAPH_UNPROVEN')
        result.append(entry)
    return result


def approved_selection(audit, approval):
    """No broad 'pilot approved' prose can approve changed audio bytes."""
    require(approval.get('schema') == 'singularityvr-audio-approval-v1', 'approval schema')
    require(approval.get('audit_sha256') == sha(audit), 'approval refers to different inventory')
    require(approval.get('listening_approved') is True, 'human listening approval missing')
    require(approval.get('rights_approved') is True, 'rights clearance missing')
    for key in ('approved_by', 'listening_evidence', 'rights_evidence'):
        require(isinstance(approval.get(key), str) and approval[key].strip(), key + ' missing')
    selected = approval.get('selected', [])
    require(selected, 'no audio selection')
    seen = set()
    for row in selected:
        key = row.get('id', '')
        require(key and key not in seen, 'duplicate/empty selection')
        require(re.fullmatch('[a-fA-F0-9]{64}', row.get('sha256', '')), 'selection SHA missing')
        seen.add(key)
    return selected


def stage_dubbing(audit_path, approval_path, source_root, lookup_path, profile_path, out):
    """Generate current-version candidate inputs for prepare_xtts_native_banks.py.

    Deliberately no hardcoded 1667/1084 inventory counts. Every approved take is
    joined to exact existing package/asset identity, never guessed by speaker.
    Barks/MP/intro cannot enter a campaign-story package through this route.
    """
    audit = load(audit_path)
    require(audit['dubbing']['status'] == 'TECHNICAL_PASS_APPROVAL_PENDING', 'dubbing inventory failed')
    selection = approved_selection(audit_path, load(approval_path))
    profile = load(profile_path)
    require(profile.get('approved') is True, 'baseline profile unapproved')
    pins = [r for r in profile['files'] if r['path'].replace('\\', '/').casefold() ==
            'binaries/vr-assets/localization/pl/polish_story_voice.jsonl']
    require(len(pins) == 1, 'missing/ambiguous baseline lookup pin')
    lookup_sha = pins[0]['sha256'].lower()
    require(sha(lookup_path) == lookup_sha, 'baseline lookup changed')
    lookup = [json.loads(s) for s in Path(lookup_path).read_text(encoding='utf-8-sig').splitlines() if s.strip()]
    files = {r['pack'] + '/' + r['file']: r for r in audit['dubbing']['files']}
    plans, identities = [], set()
    for selected in selection:
        key = selected['id']
        require(key in files, 'selection not in audited dubbing scope')
        row = files[key]
        require(row['pack'] in ('story', 'skipped', 'vignette'), 'not campaign-story scope')
        source = Path(source_root) / row['pack'] / row['file']
        require(not source.is_symlink(), 'symlink input refused')
        require(sha(source) == row['sha256'] == selected['sha256'].lower(), 'selected WAV drift')
        require(wav_info(source) == {k: row[k] for k in ('frames', 'channels', 'sample_width', 'sample_rate')}, 'PCM drift')
        package, asset = selected.get('package', ''), selected.get('asset', '')
        require(re.fullmatch(r'SP_[A-Za-z0-9_]+_LOC_INT', package, re.I), 'not a native campaign bank')
        require(safe_leaf(asset), 'unsafe native asset')
        matches = [r for r in lookup if r.get('package', '').casefold() == package.casefold() and r.get('asset') == asset]
        require(len(matches) == 1, 'native lookup identity is missing/ambiguous')
        require((package.casefold(), asset) not in identities, 'duplicate native replacement')
        identities.add((package.casefold(), asset))
        # Preserve the established runtime filename even when SIDE renamed its WAV.
        target_name = matches[0]['file']
        require(safe_leaf(target_name) and target_name.lower().endswith('.wav'), 'unsafe lookup WAV')
        require(target_name.casefold() not in {r[1].casefold() for r in plans}, 'runtime filename collision')
        plans.append((source, target_name, row, package, asset))
    out = Path(out).resolve()
    require(out.is_relative_to(PRIVATE.resolve()) and out != PRIVATE.resolve(), 'private output required')
    require(not out.exists(), 'never overwrite candidate')
    payload = out / 'payload/Binaries/vr-assets/audio/polish/story'
    payload.mkdir(parents=True)
    staged = []
    for source, target_name, row, package, asset in plans:
        destination = payload / target_name
        shutil.copyfile(source, destination)
        require(sha(destination) == row['sha256'], 'copy mismatch')
        staged.append(dict(name=target_name, candidateSha256=row['sha256'], package=package, asset=asset))
    target_lookup = out / 'payload/Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'
    target_lookup.parent.mkdir(parents=True)
    shutil.copyfile(lookup_path, target_lookup)
    result = dict(schema='singularityvr-xtts-audio-candidate-v1',
                  status='APPROVED_INPUTS_STAGED_NATIVE_BANK_BUILD_PENDING',
                  files=staged, lookupAfterSha256=lookup_sha.lower(),
                  acceptedProfileSha256=sha(profile_path),
                  nativeScope=[dict(package=p, asset=a) for _, _, _, p, a in plans],
                  approvalSha256=sha(approval_path), auditSha256=sha(audit_path),
                  steam_written=False, physical_acceptance='PENDING')
    (out / 'audio-payload-manifest.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    return result


def stage_remaster(audit_path, approval_path, source_root, out):
    """Stage a selected take and its measured native graph, never a mixer overlay.

    Output is NOT a deployable bank. MAIN must hash-pin the accepted native
    package, encode its exact PC Vorbis payload, and validate the opaque bank
    roundtrip before any deployment. No guessed graph (including storm) enters.
    """
    audit = load(audit_path)
    selection = approved_selection(audit_path, load(approval_path))
    targets = {r['id']: r for r in audit['remaster']}
    plans, seen = [], set()
    for selected in selection:
        pieces = selected['id'].split('/')
        require(len(pieces) == 2, 'remaster identity must specify target/variant.wav')
        target, variant = pieces
        require(target in targets and variant in VARIANTS[1:], 'unknown remaster take')
        require(target not in seen, 'multiple variants for one native wave')
        seen.add(target)
        row = targets[target]
        require(row['status'] == 'TECHNICAL_PASS_APPROVAL_PENDING', 'native graph unproven')
        take = next(r for r in row['variants'] if r['file'] == variant)
        folder = Path(source_root) / target
        source, metadata = folder / variant, folder / 'metadata.json'
        require(not source.is_symlink() and not metadata.is_symlink(), 'symlink input refused')
        require(sha(source) == take['sha256'] == selected['sha256'].lower(), 'selected WAV drift')
        require(sha(metadata) == row['metadata_sha256'], 'native graph metadata drift')
        info = wav_info(source)
        require(info == {k: take[k] for k in ('frames', 'channels', 'sample_width', 'sample_rate')}, 'PCM drift')
        plans.append((target, source, metadata, row, take))
    out = Path(out).resolve()
    require(out.is_relative_to(PRIVATE.resolve()) and out != PRIVATE.resolve(), 'private output required')
    require(not out.exists(), 'never overwrite candidate')
    out.mkdir(parents=True)
    staged = []
    for target, source, metadata, row, take in plans:
        folder = out / target
        folder.mkdir()
        shutil.copyfile(source, folder / source.name)
        shutil.copyfile(metadata, folder / 'metadata.json')
        require(sha(folder / source.name) == take['sha256'], 'copy mismatch')
        staged.append(dict(id=target, package=row['package'], asset=row['asset'], cue=row['cue'],
                           selected_file=source.name, sha256=take['sha256'],
                           metadata_sha256=row['metadata_sha256']))
    result = dict(schema='singularityvr-native-remaster-inputs-v1', files=staged,
                  status='APPROVED_INPUTS_STAGED_NATIVE_BANK_BUILD_PENDING',
                  approval_sha256=sha(approval_path), audit_sha256=sha(audit_path),
                  steam_written=False, physical_acceptance='PENDING')
    (out / 'native-remaster-inputs.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest='action', required=True)
    audit = sub.add_parser('audit')
    for name in ('dubbing-root', 'dubbing-manifest', 'audition-root', 'out'):
        audit.add_argument('--' + name, type=Path, required=True)
    stage = sub.add_parser('stage-dubbing')
    for name in ('audit', 'approval', 'source-root', 'lookup', 'profile', 'out'):
        stage.add_argument('--' + name, type=Path, required=True)
    remaster = sub.add_parser('stage-remaster')
    for name in ('audit', 'approval', 'source-root', 'out'):
        remaster.add_argument('--' + name, type=Path, required=True)
    args = ap.parse_args()
    if args.action == 'audit':
        require(not args.out.exists(), 'never overwrite audit')
        result = dict(schema='singularityvr-audio-release-audit-v1',
                      dubbing=inventory_dubbing(args.dubbing_root, args.dubbing_manifest),
                      remaster=inventory_remaster(args.audition_root),
                      steam_accessed=False, physical_acceptance='PENDING')
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(result, indent=2), encoding='utf-8')
        print('Audit:', args.out, sha(args.out), result['dubbing']['counts'])
    elif args.action == 'stage-dubbing':
        print(json.dumps(stage_dubbing(args.audit, args.approval, args.source_root,
                                      args.lookup, args.profile, args.out), indent=2))
    else:
        print(json.dumps(stage_remaster(args.audit, args.approval, args.source_root, args.out), indent=2))


if __name__ == '__main__':
    main()
