// Offline synthetic cameras; not a replay of game matrices or PSVR2 proof.
#include "../diegetic_hud_draw.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace diegetichud;

Pose FixturePose(const WeaponMount& mo,float roll=0.f) {
    Pose p;const float a=mo.pitchDeg*3.14159265f/180.f;
    // Place the measured mount 30 UU in front of a camera facing its screen.
    p.chain.m[0][0]=1;p.chain.m[1][1]=-sinf(a);p.chain.m[2][1]=cosf(a);
    p.chain.m[1][2]=cosf(a);p.chain.m[2][2]=sinf(a);p.chain.m[3][3]=1;
    p.chain.m[3][0]=-mo.cxUU;
    p.chain.m[3][1]=-mo.cyUU*sinf(a)-mo.czUU*cosf(a);
    p.chain.m[3][2]=30+mo.cyUU*cosf(a)-mo.czUU*sinf(a);
    for(int r=0;r<4;++r){const float x=p.chain.m[r][0],y=p.chain.m[r][1];
        p.chain.m[r][0]=x*cosf(roll)-y*sinf(roll);p.chain.m[r][1]=x*sinf(roll)+y*cosf(roll);}
    for(int eye=0;eye<2;++eye){auto& m=p.eyeClip[eye].m;
        m[0][0]=.5f;m[1][1]=1;m[2][2]=100.f/99.f;m[2][3]=1;
        m[3][2]=-100.f/99.f;m[2][0]=eye?.5f:-.5f;
        m[3][0]=eye?-.75f:.75f; // 3 UU separation; half-frame clip remap
    }return p;
}

#ifndef DIEGETIC_FIXTURE_INCLUDE_ONLY
int main(){
    const unsigned verts[]={5799,11387,8297};
    const unsigned tris[]={6767,12834,10835};
    unsigned projections=0;
    for(int w=0;w<3;++w){const auto* mo=MountForBodyDraw(0,0,verts[w],0,tris[w]);assert(mo);
        assert(!MountForBodyDraw(1,0,verts[w],0,tris[w]));
        assert(!MountForBodyDraw(0,1,verts[w],0,tris[w]));
        assert(!MountForBodyDraw(0,0,verts[w],3,tris[w]));
        assert(!MountForBodyDraw(0,0,verts[w],0,tris[w]-1));
        Quad3D q[16];const int count=BuildFixtureQuads3D(*mo,6,24,q,16);
        const V3 centre=ToNativeVertexSpace({mo->cxUU,mo->cyUU,mo->czUU});
        for(int f=0;f<5;++f){const auto a=q[f].p[1]-q[f].p[0],b=q[f].p[3]-q[f].p[0];
            const V3 normal{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
            V3 mid{};for(const auto& v:q[f].p)mid=mid+v*.25f;
            const auto outward=mid-centre;
            const float dot=normal.x*outward.x+normal.y*outward.y+normal.z*outward.z;
            if(dot>=0){printf("winding w=%d face=%d dot=%g\n",w,f,dot);fflush(stdout);}
            assert(dot<0); // reflection reverses the original source winding
        }
        for(float roll:{0.f,.785398f,1.570796f,3.141593f})for(unsigned width:{4128u,5264u}){
            auto pose=FixturePose(*mo,roll);
            const auto centreWorld=Point(centre,pose.chain);
            assert(fabsf(centreWorld.x)<.0001f && fabsf(centreWorld.y)<.0001f && fabsf(centreWorld.z-30)<.0001f);
            const auto wrong=Point({centre.x,-centre.y,centre.z},pose.chain);
            assert(fabsf(wrong.z-centreWorld.z)>200.f); // disproves the handoff identity conversion
            const auto plan=Project(q,count,pose,width,width==4128?2208:2684,100);
            assert(plan.ready && plan.eye[0].front[5] && plan.eye[1].front[5]);
            for(int eye=0;eye<2;++eye)for(int k=0;k<4;++k){
                const auto v=plan.eye[eye].quad[4][k];
                assert(v.x>=eye*width/2 && v.x< (eye+1)*width/2);assert(v.y>0 && v.y<plan.height);}
            ++projections;
            auto bad=pose;bad.eyeClip[1].m[3][3]=-100;
            assert(!Project(q,count,bad,width,2684,100).ready); // one bad eye rejects whole pair
            bad=pose;bad.chain.m[1][1]=std::numeric_limits<float>::quiet_NaN();
            assert(!Project(q,count,bad,width,2684,100).ready);
        }
    }
    assert(MountForBodyDraw(0,0,11378,0,12834)->verts==11387);
    auto body=FixturePose(*MountForVerts(11387));RecordLiveTransform(11387,body,1000,10);
    // Actual register conversion consumed by the production capture. ROW and
    // COL layouts must produce identical per-eye vertices, without a second C.
    Pose columns=body;
    for(int eye=0;eye<2;++eye){float regs[16];
        for(int r=0;r<4;++r)for(int c=0;c<4;++c)regs[c*4+r]=body.eyeClip[eye].m[r][c];
        columns.eyeClip[eye]=RowProjection(regs,true);
        assert(memcmp(&columns.eyeClip[eye],&body.eyeClip[eye],sizeof(Mat4))==0);
        const auto rows=RowProjection(&body.eyeClip[eye].m[0][0],false);
        assert(memcmp(&rows,&body.eyeClip[eye],sizeof(Mat4))==0);
    }
    // Late attachment must not overwrite the body's pose/root camera.
    assert(!MountForBodyDraw(0,0,11387,38502,8));
    assert(!MountForBodyDraw(0,11378,9,38502,8));
    auto unrelated=body;unrelated.eyeClip[0].m[3][0]=1000;
    if(const auto* m=MountForBodyDraw(0,0,11387,38502,8))RecordLiveTransform(m->verts,unrelated,1000,10);
    Pose saved;assert(GetLiveTransform(11387,1001,10,saved));
    assert(saved.eyeClip[0].m[3][0]==body.eyeClip[0].m[3][0]);
    assert(!GetLiveTransform(5799,1001,10,saved));assert(!GetLiveTransform(11387,1101,10,saved));
    printf("PASS: %u stereo projections, reflected source winding, exact body/alias selection, attachment rejection, coherent pose, near-plane/NaN failure\n",projections);
}
#endif
