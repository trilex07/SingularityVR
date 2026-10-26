"""Prepare exact-original Centurion 2K and Kasimov diffuse 4K private candidates.

Kasimov nominal mip0 is UNUSED in both SP and MP packages. Its first present
cooked diffuse is 1024x512 (mip1), so 4K here means 4x interpolation of THAT
source, not recovered missing detail. All original alpha and DXT formats stay.
Existing Valkyrie 4K / Kasimov normal / Volk+gloves are reused without changes.
"""
import argparse
import json
from pathlib import Path
from audit_cooked_texture import sha
from dds_io import load_dds, save_dds, save_normal_dds, validate_dds
from prepare_hires_first_person import safe_private, upscale_diffuse, upscale_normal, preview

MAPS = {
 'handgun_d': dict(source_name='AR_Pistol_d', package='wp_SP_ARPistol_SF', role='diffuse',
    source_sha='0c958ed11c3311ddbb517a8893ec7e372d0811a58582220f04a6efd30a0d9a39',
    source_layout=[1024,1024], fmt='DXT5', factor=2, cooked_mip=0),
 'handgun_n': dict(source_name='AR_Pistol_n', package='wp_SP_ARPistol_SF', role='normal',
    source_sha='ae206388bb839175c3d3d0db453a7b7a0f0505b6db76fc3dc46d950ec23ae2d8',
    source_layout=[1024,1024], fmt='DXT1', factor=2, cooked_mip=0),
 'ar_sniper_rifle_d': dict(source_name='ar_sniper_rifle_d', package='RvGameMPLoader_SF', role='diffuse',
    source_sha='ac7d2276fff2042fac8ea210519115cf712719f83162e914557ec7b83b7e936b',
    source_layout=[1024,512], fmt='DXT5', factor=4, cooked_mip=1),
}

def build(local: Path, output: Path):
    safe_private(output)
    if output.exists():raise FileExistsError(output)
    recovery=local/'material-remaster-hires/recovery-20261026'
    inputs={}
    # Verify every source against its exact cooked cache receipt before output.
    for name,s in MAPS.items():
        source=(recovery/'exports'/s['package']/'Texture2D'/(s['source_name']+'.dds')) if name.startswith('handgun') else (
            local/'textures/original'/s['package']/'Texture2D'/(s['source_name']+'.dds'))
        receipt=json.loads((recovery/(s['source_name']+'.audit.json')).read_text())
        if (sha(source)!=s['source_sha'] or receipt['source_sha256']!=s['source_sha'] or
            receipt['texture']!=s['source_name'] or not receipt['exact_bulk_dds_match'] or
            receipt['first_present_mip']!=s['cooked_mip'] or receipt['source_layout']!=s['source_layout']):
            raise ValueError('Source or exact cooked-mip evidence changed: '+name)
        image,header=load_dds(source)
        if (header['width'],header['height'],header['fourcc']) != (*s['source_layout'],s['fmt']):
            raise ValueError('Wrong source dimensions/format: '+name)
        inputs[name]=(source,receipt,image)
    output.mkdir(parents=True)
    report={'status':'OFFLINE_ONLY','physical_acceptance':'PENDING','maps':{}}
    for name,s in MAPS.items():
        source,receipt,image=inputs[name]
        enhanced=upscale_normal(image) if s['role']=='normal' else upscale_diffuse(image,s['factor'])
        dest=output/(name+'.dds')
        if s['role']=='normal':save_normal_dds(dest,enhanced)
        else:save_dds(dest,enhanced,s['fmt'])
        layout=validate_dds(dest)
        if layout['pixel_data_bytes']!=layout['declared_mip_bytes'] or layout['mip_count']!=max(enhanced.size).bit_length():
            raise ValueError('Incomplete replacement mip chain: '+name)
        preview(image,enhanced,output/'previews'/(name+'.png'),name,factor=s['factor'],
            source_label='cooked mip'+str(s['cooked_mip']),candidate_label=str(s['factor'])+'x original interpolation')
        report['maps'][name]={**s,'source_fnv64':receipt['source_fnv64'],'source_sha256':sha(source),
            'sha256':sha(dest),'dds':layout,'method':str(s['factor'])+'x original interpolation, mild RGB-only unsharp; no invented detail',
            'audit':receipt}
    (output/'manifest.private.json').write_text(json.dumps(report,indent=2))
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--assets-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path)
    a=p.parse_args();r=build(a.assets_root,a.output)
    print(json.dumps({k:{'sha256':v['sha256'],'source_fnv64':v['source_fnv64'],'dds':v['dds']} for k,v in r['maps'].items()},indent=2))
