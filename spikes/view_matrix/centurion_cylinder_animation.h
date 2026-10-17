#pragma once
#include "centurion_reload.h"
#include "arm_bind.h"
#include <algorithm>

// Render-only original clip/clip_release tracks. Never invokes UE3 animation
// notifies, writes inventory, or replaces the gun's native material/shaders.
namespace centurionanim {
// Capability request only, not resource identity. Keep the cooked-size IB
// readable for its later content witness; every other flag/pool is preserved.
inline unsigned ReadableUsage(bool enabled,unsigned bytes,bool index16,unsigned usage,unsigned writeOnly){
    return enabled&&bytes==6767*3*2&&index16 ? usage&~writeOnly : usage;
}
struct Data {
    bool ready=false;
    bool nativeSpace=false; // runtime only; SVCA stays ActorX/export-space v1
    std::uint32_t indexHash=0;
    armbind::Mat parent{},inverse[2]{};
    float keys[53][2][7]{}; // original PSA position xyz/quaternion xyzw
};
inline std::uint32_t Hash(const unsigned char* p,unsigned n){
    std::uint32_t h=2166136261u;for(unsigned i=0;i<n;++i)h=(h^p[i])*16777619u;return h;
}
inline bool Decode(const unsigned char* p,std::size_t size,Data& out){
    if(!p||size!=3208||std::memcmp(p,"SVCA",4))return false;
    std::uint32_t header[10];float fps;
    std::memcpy(header,p+4,40);std::memcpy(&fps,p+44,4);
    if(header[0]!=1||header[1]!=5799||header[2]!=0||header[3]!=6767||
       header[5]!=5||header[6]!=1||header[7]!=0||header[8]!=4||header[9]!=53||fps!=24.f)return false;
    Data d;d.indexHash=header[4];
    std::memcpy(d.parent.m,p+48,64);std::memcpy(d.inverse,p+112,128);
    std::memcpy(d.keys,p+240,sizeof(d.keys));
    if(!armbind::Rigid(d.parent)||!armbind::Rigid(d.inverse[0])||!armbind::Rigid(d.inverse[1]))return false;
    for(const auto& f:d.keys)for(const auto& k:f){
        double norm=0;
        for(int i=0;i<7;++i)if(!std::isfinite(k[i]))return false;
        for(int i=3;i<7;++i)norm+=double(k[i])*k[i];
        if(std::fabs(norm-1.)>.01||std::fabs(k[0])+std::fabs(k[1])+std::fabs(k[2])>500.f)return false;
    }
    d.ready=true;out=d;return true;
}
// Actual cooked 5799-vertex stream is UE3-space, unlike the ActorX cache.
// Convert bind transforms ONCE, not native GPU vertices or live bone constants.
inline bool ToNative(const Data& source,Data& out){
    if(!source.ready)return false;
    Data staged=source;
    if(!staged.nativeSpace){
        auto reflect=[](armbind::Mat& m){
            for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
                m.m[r][c]*=(r==1?-1.f:1.f)*(c==1?-1.f:1.f);
        };
        reflect(staged.parent);reflect(staged.inverse[0]);reflect(staged.inverse[1]);
        staged.nativeSpace=true;
    }
    if(!armbind::Rigid(staged.parent)||!armbind::Rigid(staged.inverse[0])||!armbind::Rigid(staged.inverse[1]))return false;
    out=staged;return true;
}
struct PoseTime {bool active=false,hideClip=false;float frame=0;};
inline PoseTime Time(centurionreload::Phase phase,std::uint64_t since,std::uint64_t now){
    if(now<since)return {};
    const float age=float(std::min<std::uint64_t>(now-since,1000));
    using P=centurionreload::Phase;
    switch(phase){
    case P::Unlocked:return {true,false,10.f*std::min(age/120.f,1.f)};
    case P::Open:return {true,false,10.f+2.f*std::min(age/180.f,1.f)};
    case P::NeedAmmo:return {true,age>=300.f,12.f+7.f*std::min(age/300.f,1.f)};
    case P::Carrying:return {true,true,19.f};
    case P::Loaded:return {true,false,26.f+7.f*std::min(age/280.f,1.f)};
    case P::WaitingNative:return {true,false,33.f+15.f*std::min(age/625.f,1.f)};
    default:return {};
    }
}
inline bool Sample(const Data& d,unsigned bone,float frame,armbind::Mat& out){
    if(!d.ready||bone>1||!std::isfinite(frame)||frame<0||frame>52)return false;
    const unsigned a=(unsigned)frame,b=std::min(a+1,52u);const float t=frame-float(a);
    const auto& ka=d.keys[a][bone];const auto& kb=d.keys[b][bone];
    float dot=0;for(int i=3;i<7;++i)dot+=ka[i]*kb[i];
    armbind::ReferenceBone key;
    key.pskPosition={ka[0]+(kb[0]-ka[0])*t,ka[1]+(kb[1]-ka[1])*t,ka[2]+(kb[2]-ka[2])*t};
    for(int i=0;i<4;++i)key.pskQuaternion[i]=ka[i+3]*(1-t)+kb[i+3]*t*(dot<0?-1.f:1.f);
    armbind::Mat local;if(!armbind::PskLocal(key,false,local))return false;
    // PskLocal already reverses ActorX reflection into UE3 native space.
    // Only the legacy cache-space route reflects back into exported geometry.
    if(!d.nativeSpace)for(int r=0;r<4;++r)for(int c=0;c<4;++c)
        local.m[r][c]*=(r==1?-1.f:1.f)*(c==1?-1.f:1.f);
    armbind::Mat absolute;skeletal::Multiply(absolute.m,local.m,d.parent.m);
    skeletal::Multiply(out.m,d.inverse[bone].m,absolute.m);
    return armbind::Rigid(out);
}
inline bool Palette(const Data& d,PoseTime time,const float (&native)[15][4],float (&out)[15][4]){
    if(!time.active)return false;
    armbind::Mat parent;if(!armbind::FromGpu(reinterpret_cast<const float (*)[4]>(&native[3]),parent))return false;
    float staged[15][4];std::memcpy(staged,native,sizeof(staged));
    for(unsigned bone=0;bone<2;++bone){
        armbind::Mat delta,target;
        if(!Sample(d,bone,time.frame,delta))return false;
        skeletal::Multiply(target.m,delta.m,parent.m);
        if(!armbind::Finite(target))return false;
        if(bone==0&&time.hideClip)for(int r=0;r<3;++r)for(int c=0;c<3;++c)target.m[r][c]=0;
        const unsigned slot=bone?4:0;
        armbind::ToGpu(target,reinterpret_cast<float (*)[4]>(&staged[slot*3]));
    }
    std::memcpy(out,staged,sizeof(staged));return true;
}
} // namespace centurionanim
