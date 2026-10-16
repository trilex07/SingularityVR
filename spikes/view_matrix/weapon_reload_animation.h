#pragma once
#include "weapon_reload_profiles.h"
#include "arm_bind.h"
#include <algorithm>
#include <memory>
#include <new>

// Bounded original animation consumer used by the default-OFF runtime path.
// No UObject/inventory operation here. Private original caches stay out of Git.
namespace weaponreloadanim {
using Kind=weaponreloadproto::Kind;
struct Spec {
    Kind kind;unsigned vertices,triangles,indexCount,indexHash,slots,root,tracks,frames,fps;
    unsigned target[5];int parent[5];
};
inline constexpr Spec specs[]{
    {Kind::Valkyrie,11387,12834,38526,941526542,6,0,5,52,24,{3,1,5,4,2},{-1,-1,-1,-1,-1}},
    {Kind::Volk,8297,10835,32505,754008012,5,2,4,108,36,{3,0,1,4,0},{-1,-1,-1,-1,-1}},
    {Kind::Kasimov,11389,12942,39330,906901468,6,0,5,239,60,{4,5,2,1,3},{-1,-1,-1,2,3}}
};
inline const Spec* Find(Kind kind){for(const auto& s:specs)if(s.kind==kind)return &s;return nullptr;}
struct Data {
    bool ready=false;Kind kind=Kind::None;
    unsigned slots=0,root=0,tracks=0,frames=0;float fps=0;
    unsigned target[8]{};int parent[8]{};
    armbind::Mat rootBind{},inverse[8]{};
    float keys[256][8][7]{};
};
inline unsigned U32(const unsigned char* p){unsigned v;std::memcpy(&v,p,4);return v;}
inline bool Decode(const unsigned char* p,std::size_t size,Kind requested,Data& out){
    const auto* s=weaponreloadanim::Find(requested);
    if(!p||!s||size<112||std::memcmp(p,"SVWA",4))return false;
    unsigned h[10];float fps;std::memcpy(h,p+4,sizeof(h));std::memcpy(&fps,p+44,4);
    if(h[0]!=1||h[1]!=unsigned(requested)||h[2]!=s->vertices||h[3]||h[4]!=s->triangles||h[5]!=s->indexHash||
       h[6]!=s->slots||h[7]!=s->root||h[8]!=s->tracks||h[9]!=s->frames||fps!=float(s->fps)||
       size!=112+std::size_t(s->tracks)*72+std::size_t(s->frames)*s->tracks*28)return false;
    // Decode is a LOAD-time operation, never a draw-time allocation. Keep large
    // fixed caches off the small x86 stack and leave output unchanged on failure.
    std::unique_ptr<Data> staged;
    try{staged=std::make_unique<Data>();}catch(const std::bad_alloc&){return false;}
    auto& d=*staged;d.kind=requested;d.slots=s->slots;d.root=s->root;
    d.tracks=s->tracks;d.frames=s->frames;d.fps=fps;
    std::memcpy(d.rootBind.m,p+48,64);
    if(!armbind::Rigid(d.rootBind))return false;
    std::size_t cursor=112;
    for(unsigned i=0;i<d.tracks;++i){
        std::memcpy(&d.target[i],p+cursor,4);std::memcpy(&d.parent[i],p+cursor+4,4);
        std::memcpy(d.inverse[i].m,p+cursor+8,64);cursor+=72;
        if(d.target[i]!=s->target[i]||d.parent[i]!=s->parent[i]||!armbind::Rigid(d.inverse[i]))return false;
    }
    for(unsigned f=0;f<d.frames;++f)for(unsigned i=0;i<d.tracks;++i){
        auto& k=d.keys[f][i];std::memcpy(k,p+cursor,28);cursor+=28;
        for(float v:k)if(!std::isfinite(v))return false;
        double norm=0;for(unsigned j=3;j<7;++j)norm+=double(k[j])*k[j];
        if(std::fabs(norm-1.)>.01||std::fabs(k[0])+std::fabs(k[1])+std::fabs(k[2])>500)return false;
    }
    if(cursor!=size)return false;
    d.ready=true;out=d;return true;
}
inline bool Sample(const Data& d,float frame,armbind::Mat (&delta)[8]){
    const auto* s=weaponreloadanim::Find(d.kind);
    if(!d.ready||!s||d.tracks!=s->tracks||d.frames!=s->frames||d.slots!=s->slots||d.root!=s->root||
       !std::isfinite(frame)||frame<0||frame>float(d.frames-1))return false;
    const unsigned a=unsigned(frame),b=std::min(a+1,d.frames-1);const float t=frame-float(a);
    armbind::Mat absolute[8]{},staged[8]{};
    for(unsigned i=0;i<d.tracks;++i){
        if(d.target[i]!=s->target[i]||d.parent[i]!=s->parent[i])return false;
        const auto& ka=d.keys[a][i];const auto& kb=d.keys[b][i];
        float dot=0;for(unsigned k=3;k<7;++k)dot+=ka[k]*kb[k];
        armbind::ReferenceBone key;
        key.pskPosition={ka[0]+(kb[0]-ka[0])*t,ka[1]+(kb[1]-ka[1])*t,ka[2]+(kb[2]-ka[2])*t};
        for(unsigned k=0;k<4;++k)key.pskQuaternion[k]=ka[k+3]*(1-t)+kb[k+3]*t*(dot<0?-1.f:1.f);
        armbind::Mat local;if(!armbind::PskLocal(key,false,local))return false;
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)local.m[r][c]*=(r==1?-1.f:1.f)*(c==1?-1.f:1.f);
        const auto& parent=d.parent[i]<0?d.rootBind:absolute[d.parent[i]];
        skeletal::Multiply(absolute[i].m,local.m,parent.m);
        skeletal::Multiply(staged[i].m,d.inverse[i].m,absolute[i].m);
        if(!armbind::Rigid(staged[i]))return false;
    }
    for(unsigned i=0;i<d.tracks;++i)delta[i]=staged[i];return true;
}
// Caller MUST prove live object+IB bytes+CTAB layout/context, install for BOTH
// eyes, and restore registers. This helper alone has no permission to draw.
inline bool Palette(const Data& d,bool enabled,float frame,const float* native,unsigned regs,float* out){
    if(!enabled||!native||!out||!d.ready||d.slots>16||d.root>=d.slots||regs!=d.slots*3)return false;
    armbind::Mat parent;if(!armbind::FromGpu(reinterpret_cast<const float (*)[4]>(native+d.root*12),parent)||
       !armbind::Finite(parent))return false;
    armbind::Mat delta[8];if(!Sample(d,frame,delta))return false;
    float staged[16*12];std::memcpy(staged,native,regs*4*sizeof(float));
    for(unsigned i=0;i<d.tracks;++i){
        armbind::Mat target;skeletal::Multiply(target.m,delta[i].m,parent.m);
        if(!armbind::Finite(target))return false;
        armbind::ToGpu(target,reinterpret_cast<float (*)[4]>(staged+d.target[i]*12));
    }
    // Root/unused slots are copied byte-exact, not regenerated from quaternion.
    std::memcpy(out,staged,regs*4*sizeof(float));return true;
}
} // namespace weaponreloadanim
