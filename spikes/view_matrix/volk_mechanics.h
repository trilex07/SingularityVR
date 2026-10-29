#pragma once
#include "weapon_reload_runtime.h"

namespace volkmechanics {
// Source-authored clip seat terminates 0.56 UU from PSK bind. Normalize the
// COMPLETE endpoint transform, not a guessed translation. Row-vector order:
// D(f)=inverseBind*Anim(f); corrected D(f)*inverse(D(end)); then native root.
inline bool Relative(const weaponreloadanim::Data& d,unsigned slot,float frame,float endpoint,armbind::Mat& out){
    armbind::Mat pose[8],end[8],inverse;
    if(!weaponreloadanim::Sample(d,frame,pose)||!weaponreloadanim::Sample(d,endpoint,end))return false;
    for(unsigned i=0;i<d.tracks;++i)if(d.target[i]==slot){
        if(!skeletal::Inverse(inverse.m,end[i].m))return false;
        skeletal::Multiply(out.m,pose[i].m,inverse.m);return armbind::Rigid(out);
    }
    return false;
}
inline bool Palette(const weaponreloadanim::Data& d,weaponreload::Phase phase,
        uint64_t since,uint64_t inserted,uint64_t now,const float* parent,unsigned regs,float* out){
    using namespace weaponreload;
    if(d.kind!=Kind::Volk||!parent||!out||regs!=15||now<since||(inserted&&now<inserted))return false;
    armbind::Mat root;if(!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(parent+2*12),root))return false;
    float staged[60];memcpy(staged,parent,sizeof(staged));
    const bool absent=Absent(phase);
    const float elapsed=float(now-since)*.001f*36.f;
    const float after=inserted?float(now-inserted)*.001f*36.f:0;
    if(absent){for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)staged[r*4+c]=0;}
    else if(inserted&&after<6.f){
        armbind::Mat delta,skin;
        if(!Relative(d,0,68.f+std::min(after,6.f),74.f,delta))return false;
        skeletal::Multiply(skin.m,delta.m,root.m);armbind::ToGpu(skin,reinterpret_cast<float(*)[4]>(staged));
    }
    // Clip removal/insertion references are deliberately NOT sampled. Their
    // motion belongs to the tracked hand and existing CarryPalette replay.
    if(absent||(inserted&&after<6.f)){
        const float frame=inserted?74.f+std::min(after,2.f):19.f+std::min(elapsed,5.f);
        armbind::Mat delta,skin;
        if(!Relative(d,1,frame,76.f,delta))return false;
        skeletal::Multiply(skin.m,delta.m,root.m);armbind::ToGpu(skin,reinterpret_cast<float(*)[4]>(staged+12));
    }
    // release is a static marker, Trigger untouched; native root byte exact.
    memcpy(out,staged,sizeof(staged));return true;
}
}
