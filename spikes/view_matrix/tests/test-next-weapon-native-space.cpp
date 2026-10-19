#include "../weapon_reload_runtime.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
using namespace weaponreload;
static armbind::Mat Identity(){armbind::Mat m;for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
static std::vector<unsigned char> Read(const std::string& path){
    FILE* f=nullptr;assert(!fopen_s(&f,path.c_str(),"rb")&&f);
    assert(!fseek(f,0,SEEK_END));const long n=ftell(f);assert(n>0&&n<1024*1024);rewind(f);
    std::vector<unsigned char> b((size_t)n);assert(fread(b.data(),1,b.size(),f)==b.size());fclose(f);return b;
}
static void Close(hipknife::V a,hipknife::V b,float tolerance=.0001f){assert(hipknife::Length(a-b)<tolerance);}
static hipknife::V Position(const unsigned char* p){hipknife::V v;std::memcpy(&v,p,12);return v;}
static unsigned Short(const unsigned char* p){unsigned short i;std::memcpy(&i,p,2);return i;}
static armbind::Mat RotateZ(float angle){auto m=Identity();m.m[0][0]=m.m[1][1]=std::cos(angle);
    m.m[0][1]=std::sin(angle);m.m[1][0]=-std::sin(angle);return m;}
static void Test(const weaponreloadanim::Spec& spec,const std::string& prepared,const std::string& original){
    const std::string name=Stem(spec.kind),base=prepared+"/"+name+"/"+name;
    const auto animation=Read(base+"_reload.svwa"),cache=Read(base+"_clip.svrk");
    auto d=std::make_unique<weaponreloadanim::Data>();assert(weaponreloadanim::Decode(animation.data(),animation.size(),spec.kind,*d));
    assert(!d->nativeSpace);armbind::Mat exportBind;assert(ClipBind(*d,exportBind));
    assert(weaponreloadanim::ToNative(*d)&&d->nativeSpace);auto copy=std::make_unique<weaponreloadanim::Data>(*d);
    assert(weaponreloadanim::ToNative(*d)&&!std::memcmp(copy.get(),d.get(),sizeof(*d))); // once, not every draw
    d->inverse[d->tracks-1].m[0][0]=NAN;*copy=*d;
    assert(!weaponreloadanim::ToNative(*d)&&!std::memcmp(copy.get(),d.get(),sizeof(*d))); // reject before mutation
    assert(weaponreloadanim::Decode(animation.data(),animation.size(),spec.kind,*d)&&!d->nativeSpace);
    assert(weaponreloadanim::ToNative(*d));armbind::Mat bind;assert(ClipBind(*d,bind));
    assert(std::fabs(bind.m[3][1]+exportBind.m[3][1])<.00005f);
    assert(hipknife::Length(Transform({},bind)-Transform({},exportBind))>200.f); // old path must FAIL
    // Independent NumPy export oracle, conjugated explicitly, not through the production helper.
    const auto oracle=Read(base+"_oracle.svwo");assert(!std::memcmp(oracle.data(),"SVWO",4));
    const unsigned samples=weaponreloadanim::U32(oracle.data()+16);
    assert(samples==(d->frames-1)*4+1&&oracle.size()==20+samples*(4+d->tracks*64));
    size_t cursor=20;
    for(unsigned q=0;q<samples;++q){float frame;std::memcpy(&frame,oracle.data()+cursor,4);cursor+=4;
        armbind::Mat delta[8];assert(weaponreloadanim::Sample(*d,frame,delta));
        for(unsigned t=0;t<d->tracks;++t){armbind::Mat expected;std::memcpy(expected.m,oracle.data()+cursor,64);cursor+=64;
            for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){
                const float v=expected.m[r][c]*((r==1)!=(c==1)?-1.f:1.f);
                assert(std::fabs(delta[t].m[r][c]-v)<.0003f);
            }}
    }
    // Complete native cooked VB/IB, audited independently against PSK. No generated mesh positions.
    const auto raw=Read(original+"/"+name+"/native.svnv");
    assert(raw.size()>16&&!std::memcmp(raw.data(),"SVNV",4)&&weaponreloadanim::U32(raw.data()+4)==1);
    const unsigned nv=weaponreloadanim::U32(raw.data()+8),ni=weaponreloadanim::U32(raw.data()+12);
    assert(nv==spec.vertices&&ni==spec.indexCount&&raw.size()==16+nv*32+ni*2);
    const auto* vb=raw.data()+16;const auto* ib=vb+nv*32;
    assert(Hash(ib,ni*2)==spec.indexHash);
    const unsigned nc=weaponreloadanim::U32(cache.data()+8),ci=weaponreloadanim::U32(cache.data()+12);
    assert(!std::memcmp(cache.data(),"SVRK",4)&&weaponreloadanim::U32(cache.data()+4)==2&&cache.size()==16+nc*24+ci*2);
    const auto* cv=cache.data()+16;const auto* cb=cv+nc*24;const unsigned slot=ClipSlot(spec.kind);
    std::vector<hipknife::V> nativePoints,exportPoints;
    unsigned selected=0;double maximumOld=0;
    for(unsigned tri=0;tri<ni/3;++tri){unsigned vertex[3];bool clip=true;
        for(unsigned k=0;k<3;++k){vertex[k]=Short(ib+(tri*3+k)*2);assert(vertex[k]<nv);
            clip=clip&&vb[vertex[k]*32+20]==slot&&vb[vertex[k]*32+24]==255;}
        if(!clip)continue;
        assert(selected+3<=ci);const unsigned winding[]{1,0,2};
        for(unsigned k=0;k<3;++k){const unsigned index=Short(cb+(selected+winding[k])*2);assert(index<nc);
            const auto e=Position(cv+index*24),n=Position(vb+vertex[k]*32);
            // Export-local metres -> native-local reflection -> original native bind.
            Close(Transform({e.x*52.5f,-e.y*52.5f,e.z*52.5f},bind),n,.00008f);
            maximumOld=std::max(maximumOld,double(hipknife::Length(Transform(e*52.5f,exportBind)-n)));
            nativePoints.push_back(n);exportPoints.push_back(e);
        }selected+=3;
    }
    assert(selected==ci&&maximumOld>100); // falsifier catches the PREVIOUS wrong convention
    float palette[16*12]{};auto root=RotateZ(.41f);root.m[3][0]=5;root.m[3][1]=3;root.m[3][2]=-2;
    for(unsigned s=0;s<d->slots;++s)armbind::ToGpu(root,reinterpret_cast<float(*)[4]>(palette+s*12));
    // No inverse-bind compensation manufactured here: independently chosen engine/model poses.
    for(unsigned pose=0;pose<24;++pose){auto local=RotateZ(float(pose)*.19f),controller=RotateZ(float(pose)*-.13f);
        local.m[3][0]=11;local.m[3][1]=-8;local.m[3][2]=-15;
        controller.m[3][0]=-4;controller.m[3][1]=9;controller.m[3][2]=6;
        const hipknife::V centre{.1f,.4f,.2f};const float yaw=float(pose)*.23f,baseYaw=float(pose)*-.11f,units=45.f+pose;
        armbind::Mat map,inv,tracking;hipknife::V socket;
        assert(TrackingMap(centre,yaw,baseYaw,units,map)&&skeletal::Inverse(inv.m,map.m));
        assert(Socket(*d,palette,local,controller,centre,yaw,baseYaw,units,socket,&tracking));
        const auto nativeOrigin=Transform(Transform(Transform(Transform(Transform({},bind),root),local),controller),inv);
        Close(socket,nativeOrigin);
        for(size_t p=0;p<nativePoints.size();++p){
            const auto actual=Transform(Transform(Transform(Transform(nativePoints[p],root),local),controller),inv);
            Close(Transform(exportPoints[p],tracking),actual);
        }
        for(unsigned state=0;state<3;++state){auto desired=tracking;
            if(state){desired=RotateZ(.23f);desired.m[3][0]=-.3f;desired.m[3][1]=state==1?.8f:.1f;desired.m[3][2]=-.4f;}
            float replay[16*12];assert(SparePalette(*d,desired,local,controller,map,d->slots*3,replay));
            armbind::Mat skin;assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(replay+slot*12),skin));
            for(size_t p=0;p<nativePoints.size();++p){
                const auto actual=Transform(Transform(Transform(Transform(nativePoints[p],skin),local),controller),inv);
                Close(actual,Transform(exportPoints[p],desired));
            }
        }
    }
    Animation timing;assert(timing.Measure(*d));
    std::printf("PASS %s native: %u original clip corners, old space error %.2f UU, %u oracle poses; 24 independent poses + carry/drop replay\n",
        name.c_str(),ci,maximumOld,samples*d->tracks);
}
int main(int argc,char** argv){assert(argc==3);
    for(const auto& spec:weaponreloadanim::specs)Test(spec,argv[1],argv[2]);
    puts("L1 original cooked geometry/convention PASS; NOT live LocalToWorld/controller or headset acceptance");
}
