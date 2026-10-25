#pragma once
#include "arm_bind.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

// The native HANDS_TMD animation moves the weapon's entire root away from the
// right hand. The game still supplies valid per-bone fire/reload animation.
// Retarget that animation with the rigid difference between the current root
// and the last successfully drawn HANDS_WEAPON root of this exact weapon.
// Both roots are original GPU skin matrices, before local and controller ride.
namespace dualtmdgunpose {
using armbind::Mat;
constexpr unsigned kMaxBones=75;
struct Key {
    std::uintptr_t pawn=0,weapon=0;
    unsigned verts=0,rootSlot=0;
    bool operator==(const Key& b)const {
        return pawn==b.pawn&&weapon==b.weapon&&verts==b.verts&&rootSlot==b.rootSlot;
    }
};
inline float LargestDifference(const Mat& a,const Mat& b) {
    float d=0;
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
        d=std::max(d,std::fabs(a.m[r][c]-b.m[r][c]));
    return d;
}
struct Reference {
    Key key{};
    Mat root{};
    std::uint64_t tick=0;
    bool valid=false,held=false;
    void Clear(){valid=held=false;}
    bool Observe(Key k,const float rootGpu[3][4],std::uint64_t now) {
        Mat measured;
        if(!k.pawn||!k.weapon||k.rootSlot>=kMaxBones||
           !armbind::FromGpu(rootGpu,measured)||!armbind::Rigid(measured))return false;
        key=k;root=measured;tick=now;valid=true;held=false;return true;
    }
    bool Correct(Key k,const float original[kMaxBones][3][4],unsigned bones,
                 std::uint64_t now,float result[kMaxBones][3][4],Mat* difference=nullptr) {
        if(!valid||!(key==k)||!bones||bones>kMaxBones||k.rootSlot>=bones||
           (!held&&(now<tick||now-tick>5000)))return false;
        Mat current,inverse,delta;
        if(!armbind::FromGpu(original[k.rootSlot],current)||!armbind::Rigid(current)||
           !skeletal::Inverse(inverse.m,current.m))return false;
        skeletal::Multiply(delta.m,inverse.m,root.m);
        if(!armbind::Rigid(delta))return false;
        const auto p=armbind::Position(delta);
        if(p.x*p.x+p.y*p.y+p.z*p.z>256.f*256.f)return false;
        float staged[kMaxBones][3][4]{};
        for(unsigned i=0;i<bones;++i){
            Mat check;
            if(!armbind::FromGpu(original[i],check))return false;
            bool empty=true;
            for(const auto& row:original[i])for(const float value:row)if(value!=0)empty=false;
            if(empty)std::memcpy(staged[i],original[i],sizeof(staged[i]));
            else skeletal::TransformBone(staged[i],original[i],delta.m);
            if(!armbind::FromGpu(staged[i],check))return false;
        }
        Mat mapped;
        if(!armbind::FromGpu(staged[k.rootSlot],mapped)||LargestDifference(mapped,root)>.015f)return false;
        std::memcpy(result,staged,bones*sizeof(staged[0]));
        if(difference)*difference=delta;
        held=true;
        return true;
    }
};
}
