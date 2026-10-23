"""Private-asset regression, explicit paths only; no Steam or generated assets."""
import argparse
from pathlib import Path
from audit_volk_skin import audit

if __name__ == '__main__':
    p=argparse.ArgumentParser()
    for name in ('package','psk','lzo'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();r=audit(a.package,a.psk,a.lzo)
    assert r['mixed_wedges']==0
    assert r['psk_index_hash']==r['layout']['index_fnv']==754008012
    assert [x['bone'] for x in r['influences']]==[55,56,53,54,57]
    assert [x['referenced_wedges'] for x in r['influences']]==[280,1756,6174,32,55]
    assert [x['triangles'] for x in r['influences']]==[198,2718,7859,20,40]
    assert sum(x['triangles'] for x in r['influences'])==10835
    # Every face belongs to one rigid bone. Zero slot0 cannot collapse 10637
    # other faces; no inherited skeleton evaluation happens in the GPU palette.
    assert sum(x['triangles'] for x in r['influences'][1:])==10637
    print('PASS private Volk cooked/PSK identity + all 8297 vertices / 10835 faces: c5 is clip, not root')
