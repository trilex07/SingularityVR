"""Prepare private next-weapon clip prototypes from explicit original PSK/PSA.

Fixed ignored output below this checkout only. No Steam, native calls, asset
redistribution, texture guess or claim that bone-local axes are controller axes.
"""
import argparse
import hashlib
import json
from pathlib import Path

from prepare_centurion import extract, preview, read_psk
from inspect_centurion_animation import describe


SPECS = {
    'valkyrie': dict(label='AR9 Valkyrie', mesh='ch_inview_machinegun_mesh',
                     wedges=11387, parent='cog_machinegun', animation='MachineGun'),
    'volk': dict(label='Volk S4', mesh='ch_inview_shotgun_mesh',
                 wedges=8297, parent='cog_shotgun', animation='Shotgun'),
    'kasimov': dict(label='Kasimov SNV-E99', mesh='ch_inview_sniper_mesh',
                    wedges=11389, parent='cog_sniper', animation='sniper'),
}


def prepare(kind, psk, psa_data, units=52.5):
    if kind not in SPECS:
        raise ValueError('unsupported profile; Centurion stays separate')
    spec = SPECS[kind]
    if len(psk[6]) != spec['wedges'] or psk[0].count('clip') != 1:
        raise ValueError('profile/mesh wedge or clip identity mismatch')
    clip = psk[0].index('clip')
    if not 0 <= psk[1][clip] < clip or psk[0][psk[1][clip]] != spec['parent']:
        raise ValueError('profile/clip parent mismatch')
    cache, part = extract(psk, units)
    animation = describe(psa_data, psk[0], psk[1])
    reloads = [s for s in animation['sequences'] if s['name'] == 'Reload']
    if len(reloads) != 1 or reloads[0]['max_clip_local_travel_uu'] <= .001:
        raise ValueError('missing or immobile original reload clip track')
    return cache, dict(profile=kind, label=spec['label'], part=part,
        animation=animation, runtime='NOT WIRED; native material/socket/ammo ABI UNKNOWN',
        physical_scale='authoring units only, not controller calibration',
        texture='NOT assigned; resolve native material before presentation')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--weapon', choices=SPECS, required=True)
    parser.add_argument('--psk', type=Path, required=True)
    parser.add_argument('--psa', type=Path, required=True)
    parser.add_argument('--units-per-metre', type=float, default=52.5)
    parser.add_argument('--preview', action='store_true')
    args = parser.parse_args()
    spec = SPECS[args.weapon]
    if args.psk.stem != spec['mesh'] or args.psa.stem != spec['animation']:
        parser.error('input basenames do not match selected original profile')
    cache, report = prepare(args.weapon, read_psk(args.psk), args.psa.read_bytes(), args.units_per_metre)
    report['psk_sha256'] = hashlib.sha256(args.psk.read_bytes()).hexdigest()
    destination = Path(__file__).resolve().parents[2] / 'build/centurion-reload/next-weapons/prepared' / args.weapon
    destination.mkdir(parents=True, exist_ok=True)
    (destination / f'{args.weapon}_clip.svrk').write_bytes(cache)
    (destination / 'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    if args.preview:
        preview(cache, destination / 'clip-geometry.png', f"Original {spec['label']} clip")
    print(json.dumps(dict(profile=args.weapon, output=str(destination),
        mesh_sha256=report['psk_sha256'], cache_sha256=report['part']['output_sha256'],
        vertices=report['part']['vertices'], triangles=report['part']['triangles'],
        reload_seconds=next(s['duration_seconds'] for s in report['animation']['sequences']
                            if s['name']=='Reload')), indent=2))
