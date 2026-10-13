#include "shadow_receiver_test_device.h"
#include "../shadow_first_person_receiver_fix.h"
static void Point(const float* p,const float* m,float* out){for(int c=0;c<4;++c){out[c]=0;for(int r=0;r<4;++r)out[c]+=p[r]*m[r*4+c];}}
int main(){
    using namespace fpshadowfix;IDirect3DDevice9 d;float original[32];memcpy(original,d.ps,128);
    {Scope s(&d,true,8,12,0);}assert(!d.reads&&!d.resourceReads&&!d.writes);
    enabled=true;worldshadowfix::vsMatch=worldshadowfix::psMatch=true;
    {Scope s(&d,true,8,12,0);}assert(!d.reads&&!d.writes); // parent OFF
    worldshadowfix::enabled=true;{Scope s(&d,true,8,12,0);}assert(!d.reads&&!d.writes); // ClearGate OFF
    shadowcleargate::enabled=1;
    {Scope s(&d,true,8,12,1);}{Scope s(&d,false,8,12,0);}assert(!d.reads&&!d.writes);
    {Scope s(&d,true,8,12,0);}assert(depthSkipped==1&&!d.resourceReads&&!d.writes); // world near10 excluded
    {worldshadowfix::Scope w(&d,true,8,12,0);Scope f(&d,true,8,12,0);assert(w.changed&&!f.changed);}
    assert(memcmp(d.ps,original,128)==0); // nested draw scopes leave world path intact
    const float near1[4]={.999000013f,.000999987125f,1.001001f,.00100098806f};
    assert(FirstDepth(near1));float invalid[4];memcpy(invalid,near1,16);invalid[2]=0;assert(!FirstDepth(invalid));
    invalid[2]=near1[2];invalid[1]=1;assert(!FirstDepth(invalid));invalid[1]=NAN;assert(!FirstDepth(invalid));
    memcpy(d.depth,near1,16);
    const float nativeWorld[16]={.735813141f,-3.07219958f,-.00343087013f,-3.79502723e-8f,
        -.516575873f,-.125421539f,1.52050555f,4.03221634e-8f,
        -.918093979f,-.219521374f,-.330020458f,0,.018847147f,-.0356542356f,.000875811267f,10};
    memcpy(original,nativeWorld,64);memcpy(d.ps,original,128);
    const float eyeVP[2][16]={
        {.563439131f,-.216869056f,-.917176068f,-.918094158f,-.256444126f,-.151874781f,-.219301865f,-.219521388f,
         .118189149f,.704338074f,-.329690456f,-.330020487f,-2.17234683f,1.31887627f,3.23732328f,4.24056435f},
        {-.354655057f,-.216869056f,-.917176068f,-.918094158f,-.475965530f,-.151874781f,-.219301865f,-.219521388f,
         -.211831331f,.704338074f,-.329690456f,-.330020487f,.488608479f,1.31693530f,3.25047112f,4.25372553f}};
    const float p[4]={4.78710938f,1.15625f,-148.502563f,1};float back[2][4]{},light[2][4]{};
    for(int eye=0;eye<2;++eye){
        memcpy(d.vp,eyeVP[eye],64);d.sc={eye?100L:0L,0,eye?200L:100L,100};
        float clip[4],input[4];Point(p,d.vp,clip);input[0]=clip[0];input[1]=clip[1];input[2]=clip[3];input[3]=1;
        {worldshadowfix::Scope w(&d,true,8,12,0);Scope s(&d,true,8,12,0);assert(!w.changed&&s.changed);Point(input,d.ps,back[eye]);Point(input,d.ps+16,light[eye]);
            for(int c=0;c<4;++c)assert(std::fabs(back[eye][c]-p[c])<5e-5f);}
        assert(memcmp(d.ps,original,128)==0&&d.color.refs==1&&d.map.refs==1&&d.rt.refs==1);
    }
    assert(appliedLeft==1&&appliedRight==1);
    for(int c=0;c<4;++c)assert(std::fabs(back[0][c]-back[1][c])<5e-5f);
    for(int c=0;c<4;++c)assert(std::fabs(light[0][c]-light[1][c])<5e-4f);
    d.writes=0;d.failTexture=true;{Scope s(&d,true,8,12,0);}assert(!d.writes&&d.color.refs==1&&d.map.refs==1&&d.rt.refs==1);d.failTexture=false;
    d.writes=0;d.failFirst=true;{Scope s(&d,true,8,12,0);assert(!s.changed);}assert(d.writes==2&&memcmp(d.ps,original,128)==0);
    d.writes=0;d.failFirst=false;d.failRestore=true;{Scope s(&d,true,8,12,0);}assert(fault&&restoreFailures==1);
    unsigned writes=d.writes;{Scope s(&d,true,8,12,0);}assert(writes==d.writes);
    puts("PASS: near1 only; required world/ClearGate; captured eye round trips; no mesh/world edits; balanced refs and restored constants/fault latch");
}
