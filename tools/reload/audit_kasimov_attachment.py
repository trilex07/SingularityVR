"""Read-only original Kasimov attachment measurements. No source assets in Git."""
import argparse,hashlib,json,sys
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parent))
from prepare_weapon_animation import read_psk,read_psa,key_local

def audit(psk,psa):
    n,parents,_,bind,points,weights,wedges,faces=read_psk(psk)
    tracks,seq,keys=read_psa(psa.read_bytes());reflect=np.diag([1.,-1.,1.,1.])
    def pose(name):
        q=next(s for s in seq if s['name'].lower()==name);absolute=[]
        for i,bone in enumerate(n):
            local=reflect@key_local(keys[q['first'],tracks.index(bone)])@reflect if bone in tracks else bind[i] if i==0 else bind[i]@np.linalg.inv(bind[parents[i]])
            absolute.append(local if i==0 else local@absolute[parents[i]])
        return absolute[9]@np.linalg.inv(absolute[53])
    idle,fire=pose('idle'),pose('fire');error=float(np.max(np.abs(idle-fire)))
    assert error<.01 and n[53]=='cog_sniper' and n[9]=='l_arm_wrist'
    assert np.allclose(idle[:3,:3]@idle[:3,:3].T,np.eye(3),atol=1e-5)
    native=points*[1,-1,1];root=bind[53][3,:3]
    section=native[(abs(native[:,0])<1.4)&(abs(native[:,2]-root[2])<1.5)]
    # Place the existing housing clear of the original receiver section, at its
    # measured rear root station. This is a new UI design, not an aim adjustment.
    mount=[0.,float(-section[:,1].min()+.225),float(root[2])]
    return dict(psk_sha256=hashlib.sha256(psk.read_bytes()).hexdigest(),psa_sha256=hashlib.sha256(psa.read_bytes()).hexdigest(),
        root_bind=bind[53].tolist(),left_idle_relative=idle.tolist(),idle_fire_max_difference_uu=error,
        housing_center_actorx=mount,receiver_section_native_bounds=[section.min(0).tolist(),section.max(0).tolist()])
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--psk',type=Path,required=True);p.add_argument('--psa',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--header',type=Path)
    a=p.parse_args();r=audit(a.psk,a.psa);a.report.write_text(json.dumps(r,indent=2)+'\n')
    if a.header:
        def mat(name,data):
            return 'inline constexpr armbind::Mat '+name+'{{\n'+',\n'.join('    {'+','.join(f'{v:.10e}f' for v in row)+'}' for row in data)+'}};\n'
        a.header.write_text('#pragma once\n// Original PSK + idle PSA; reproduce with audit_kasimov_attachment.py.\n#include "seeker_geometry.h"\nnamespace kasimovsupport {\n'+mat('RootBind',r['root_bind'])+mat('LeftGrip',r['left_idle_relative'])+'}\n')
    print('PASS original Kasimov idle/fire attachment: max difference',r['idle_fire_max_difference_uu'],'UU; HUD centre',r['housing_center_actorx'])
