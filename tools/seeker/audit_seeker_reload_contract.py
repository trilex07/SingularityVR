"""Read-only script inheritance and source motion audit for Seeker reload."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'title'))
from ue3_layout import inspect
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'reload'))
from inspect_centurion_animation import read_psa


def one(rows, label):
    if len(rows) != 1: raise ValueError(f'{label}: expected one export, got {len(rows)}')
    return rows[0]


def audit(game, seeker, psa):
    core = inspect(game, include_all=True)['exports']
    source = inspect(seeker, include_all=True)['exports']
    shared = one([e for e in core if e['name'] == 'RvWeaponShared' and e['cls'] == 0], 'shared class')
    rocket = one([e for e in core if e['name'] == 'RvWeaponShared_RocketLauncher' and e['cls'] == 0], 'rocket class')
    middle = one([e for e in core if e['index'] == rocket['super_index']], 'rocket parent')
    if middle['name'] != 'RvWeaponShared_RememberProjectile' or middle['super_index'] != shared['index']:
        raise ValueError('rocket inheritance drift')
    children = [e['name'] for e in core if e['outer'] in (rocket['index'], middle['index'])]
    overrides = sorted(set(children) & {'AttemptReload','ServerStartReload','RefillClip','ReloadFinished'})
    archetype = one([e for e in source if e['name'] == 'Shared_ARSteerable'], 'Seeker archetype')
    if archetype['class_name'] != 'RvWeaponShared_RocketLauncher':
        raise ValueError('Seeker archetype class drift')
    prop = {p['name']: p.get('value') for p in archetype['properties']}
    if prop.get('mUsesAmmoType') != 'ERAT_ARSteerable' or prop.get('mbIsHeavyWeapon') is not True:
        raise ValueError('Seeker ammo/heavy archetype drift')
    names, sequences, _ = read_psa(psa.read_bytes())
    reload = one([s for s in sequences if s['name'] == 'Reload'], 'Reload sequence')
    if reload['frames'] != 54 or reload['rate'] != 24 or 'clip' not in names:
        raise ValueError('source reload drift')
    return {'game_sha256':hashlib.sha256(game.read_bytes()).hexdigest(),
            'seeker_package_sha256':hashlib.sha256(seeker.read_bytes()).hexdigest(),
            'psa_sha256':hashlib.sha256(psa.read_bytes()).hexdigest(),
            'script':{'rocket_parent':'RvWeaponShared_RememberProjectile -> RvWeaponShared','relevant_overrides':overrides,
                      'seeker_class':archetype['class_name'],
                      'ammo_type':prop['mUsesAmmoType'],'heavy_weapon':True,
                      'reload_frames':54,'reload_fps':24},
            'native':{'method_vtable':'NOT VERIFIED',
                      'Seeker_clip_reserve_commit':'NOT VERIFIED',
                      'reason':'No archived decompressed PE text or live authoritative Seeker reload transaction in SIDE inputs; script inheritance is not native execution proof'},
            'gate':'Do not activate physical reload or inject ammunition until native method, local owner, clip/reserve before-after and one acknowledgment are observed'}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for key in ('game','seeker','psa','output'):
        p.add_argument('--'+key,type=Path,required=True)
    a = p.parse_args()
    report = audit(a.game,a.seeker,a.psa)
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
