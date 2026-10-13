#include "shadow_receiver_test_device.h"
#include "../shadow_world_receiver_fix.h"
static void MapPoint(const float* point,const float* m,float* out){for(int c=0;c<4;++c){out[c]=0;for(int r=0;r<4;++r)out[c]+=point[r]*m[r*4+c];}}
int main(){
    using namespace worldshadowfix;IDirect3DDevice9 d;float original[32];memcpy(original,d.ps,128);
    {Scope s(&d,true,8,12,0);}assert(!d.reads&&!d.resourceReads&&!d.writes);
    enabled=vsMatch=psMatch=true;
    {Scope s(&d,true,8,12,1);}{Scope s(&d,false,8,12,0);}assert(!d.reads&&!d.writes);
    const float gun[4]={.999f,.001f,1.f/.999f,.001f/.999f};memcpy(d.depth,gun,16);
    {Scope s(&d,true,8,12,0);}assert(nearSkipped==1&&!d.resourceReads&&!d.writes);
    d.depth[0]=9.99f;d.depth[2]=1.f/9.99f;d.depth[3]=.001f/9.99f;
    for(int eye=0;eye<2;++eye){
        d.vp[8]=eye?.5f:-.5f;d.sc={eye?100L:0L,0,eye?200L:100L,100};
        float world[16],shadow[16];assert(worldshadowmath::Correct(d.vp,original,original+16,world,shadow));
        const float point[4]={2,1,20,1};float screen[4]={.5f*2+d.vp[8]*20,1,20,1},back[4];
        MapPoint(screen,world,back);for(int c=0;c<4;++c)assert(std::fabs(back[c]-point[c])<1e-5f);
        {Scope s(&d,true,8,12,0);assert(s.changed&&memcmp(d.ps,original,128)!=0);}
        assert(memcmp(d.ps,original,128)==0&&d.color.refs==1&&d.map.refs==1&&d.rt.refs==1);
    }
    assert(appliedLeft==1&&appliedRight==1);
    d.writes=0;d.failTexture=true;{Scope s(&d,true,8,12,0);}assert(!d.writes&&d.color.refs==1&&d.map.refs==1&&d.rt.refs==1);d.failTexture=false;
    d.writes=0;d.failFirst=true;{Scope s(&d,true,8,12,0);assert(!s.changed);}assert(d.writes==2&&memcmp(d.ps,original,128)==0);
    d.writes=0;d.failFirst=false;d.failRestore=true;{Scope s(&d,true,8,12,0);}assert(fault&&restoreFailures==1);
    unsigned writes=d.writes;{Scope s(&d,true,8,12,0);}assert(writes==d.writes);
    puts("PASS: world-only depth; per-eye round trip; unused W canonicalized; OFF/moved/RT gates; COM/constant restoration and fault latch");
}
