#include "../centurion_cylinder_animation.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace centurionanim;
static armbind::Mat Identity(){armbind::Mat m;for(int i=0;i<4;++i)m.m[i][i]=1;return m;}
int main(int argc,char** argv){
    for(unsigned bits=0;bits<1024;++bits){
        assert(ReadableUsage(false,40602,true,bits,8)==bits);
        assert(ReadableUsage(true,40600,true,bits,8)==bits);
        assert(ReadableUsage(true,40602,false,bits,8)==bits);
        assert(ReadableUsage(true,40602,true,bits,8)==(bits&~8u));
    }
    Data d;d.ready=true;d.parent=d.inverse[0]=d.inverse[1]=Identity();
    for(auto& frame:d.keys)for(auto& k:frame)k[6]=1;
    float native[15][4]{};for(unsigned slot=0;slot<5;++slot)
        armbind::ToGpu(Identity(),reinterpret_cast<float (*)[4]>(&native[slot*3]));
    native[3][3]=6; // parent translation must follow the native gun, not camera
    for(unsigned f=0;f<53;++f){d.keys[f][0][0]=float(f)*.1f;d.keys[f][1][1]=float(f)*.01f;}
    for(unsigned step=0;step<=520;++step){
        const float frame=float(step)*.1f;float changed[15][4];
        assert(Palette(d,{true,false,frame},native,changed));
        assert(std::fabs(changed[0][3]-(6+frame*.1f))<1e-4f);
        assert(std::memcmp(&native[3],&changed[3],9*4*sizeof(float))==0); // parent/hammer/trigger untouched
        assert(Palette(d,{true,true,frame},native,changed));
        for(int c=0;c<3;++c)for(int r=0;r<3;++r)assert(changed[c][r]==0);
    }
    float changed[15][4]{};assert(!Palette(d,{},native,changed));
    armbind::Mat sample;assert(!Sample(d,2,0,sample)&&!Sample(d,0,NAN,sample));
    using P=centurionreload::Phase;
    for(auto phase:{P::Idle,P::Cooldown,P::Removing,P::Discard})assert(!Time(phase,0,100).active);
    for(auto phase:{P::Unlocked,P::Open,P::NeedAmmo,P::Carrying,P::Loaded,P::WaitingNative}){
        assert(Time(phase,100,100).active&&!Time(phase,100,99).active);
        for(unsigned ms=0;ms<2000;++ms){const auto t=Time(phase,100,100+ms);assert(t.frame>=0&&t.frame<=52);}
    }
    assert(!Time(P::NeedAmmo,0,299).hideClip&&Time(P::NeedAmmo,0,300).hideClip);
    if(argc==2){
        FILE* f=nullptr;assert(!fopen_s(&f,argv[1],"rb")&&f);
        std::vector<unsigned char> cache(3208);assert(fread(cache.data(),1,cache.size(),f)==cache.size());fclose(f);
        Data actual;assert(Decode(cache.data(),cache.size(),actual));
        assert(actual.indexHash==147026316u);
        for(unsigned b=0;b<2;++b)for(unsigned frame=0;frame<=520;++frame)
            assert(Sample(actual,b,float(frame)*.1f,sample)&&armbind::Rigid(sample));
        for(unsigned b=0;b<2;++b){
            assert(Sample(actual,b,0,sample));
            const auto id=Identity();
            for(int r=0;r<4;++r)for(int c=0;c<4;++c)assert(std::fabs(sample.m[r][c]-id.m[r][c])<2e-4f);
        }
        for(unsigned at:{0u,4u,8u,12u,16u,24u,28u,32u,36u,40u,44u}){
            auto bad=cache;bad[at]^=0xff;Data reject;assert(!Decode(bad.data(),bad.size(),reject));
        }
        assert(!Decode(cache.data(),cache.size()-1,actual));
        puts("PASS private original cache: cooked palette witness + 1042 rigid interpolated poses + idle bind identity");
    }
    puts("PASS animation CPU: original slot scope, parent following, 521 poses, phase timing/hide/cancel; not Steam/PSVR2");
}
