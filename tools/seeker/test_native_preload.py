"""Private evidence and render-route checks; GPU fixtures test actual resources."""
import argparse
import hashlib
import json
from pathlib import Path
import re

def test(repo, private):
    receipt=json.loads((private/'native_identity/identity.json').read_text())
    registry=(repo/'spikes/texture_cache/targets_generated.h').read_text()
    pins=(repo/'spikes/texture_cache/seeker_targets_generated.h').read_text()
    for suffix in ('d','n'):
        name='ar_steerable_'+suffix;r=receipt[name]
        data=(private/'native_identity'/(name+'.mip0.bin')).read_bytes()
        fnv=0xcbf29ce484222325
        for b in data:fnv=((fnv^b)*0x100000001b3)&0xffffffffffffffff
        assert f'{fnv:016X}'==r['fnv1a64']
        assert hashlib.sha256(data).hexdigest()==r['mip0_sha256']
        line=next(l for l in registry.splitlines() if '"'+name+'"' in l)
        assert str(fnv)+'ULL' in line and r['format'][3:] in line
        dds=private/'final_handoff/native_textures'/f'AR_Steerable_{suffix}.dds'
        assert hashlib.sha256(dds.read_bytes()).hexdigest() in pins
    source=(repo/'spikes/texture_cache/texture_injector.cpp').read_text()
    route=source[source.index('if(seekerTarget>=0)'):source.index('// New blood identities')]
    header=(repo/'spikes/texture_cache/seeker_preload.h').read_text()
    ready=header[header.index('inline IDirect3DBaseTexture9* Ready'):]
    for code in (route,ready):
        assert not re.search(r'LoadDDS|CreateReplacement|fopen|new\s|\.resize\(|\.push_back\(',code)
    assert 'seekerpreload::Ready' in route and 'return forward' in route
    assert 'seekerpreload::Index(t.name)>=0&&!seekerpreload::mode' in source
    assert 'seeker_material_mode",0' in header
    print('PASS exact original DXT identities, generated targets/SHA pins, OFF gate and Seeker bind route has no DDS/file/allocation/upload calls')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--private',type=Path,required=True);p.add_argument('--repo',type=Path,default=Path('.'))
    a=p.parse_args();test(a.repo,a.private)
