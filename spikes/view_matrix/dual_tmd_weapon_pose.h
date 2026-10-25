#pragma once
#include "arm_bind.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

// The weapon body is submitted before the material-0 glove in each UE3 frame.
// Keep the last completed, stereo-confirmed wrist and advance it by the RAW
// controller delta. HANDS_TMD may animate the gun root, while BuildGunC applies
// a separate controller ride: comparing the un-ridden roots was the 59-UU error.
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
inline Mat Product(const Mat& a,const Mat& b) {
    Mat result;skeletal::Multiply(result.m,a.m,b.m);return result;
}
inline bool Affine(const Mat& a) {
    return armbind::Finite(a)&&
        std::fabs(a.m[0][3])+std::fabs(a.m[1][3])+std::fabs(a.m[2][3])+
        std::fabs(a.m[3][3]-1.f)<1e-5f;
}
inline bool Uniform(const Mat& a) {
    if(!Affine(a))return false;
    float length=0;
    for(unsigned i=0;i<3;++i)length+=a.m[0][i]*a.m[0][i];
    length=std::sqrt(length);
    if(length<.1f||length>4.f)return false;
    Mat unit=a;
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)unit.m[r][c]/=length;
    return armbind::Rigid(unit);
}
struct Reference {
    Key key{};
    Mat gunFromWrist{},lastWrist{},lastRawController{};
    Mat pendingGun{};
    std::uint64_t tick=0,wristTick=0;
    unsigned pendingFrame=0,wristFrame=0;
    bool valid=false,wristValid=false,pending=false;
    void Clear(){valid=wristValid=pending=false;}
    bool ObserveGun(Key k,const float rootGpu[3][4],const Mat& local,
                    const Mat& ride,unsigned frame,std::uint64_t now) {
        Mat root;
        if(!k.pawn||!k.weapon||k.rootSlot>=kMaxBones||!frame||!now||
           !armbind::FromGpu(rootGpu,root)||!armbind::Rigid(root)||
           !Affine(local)||!Uniform(ride))return false;
        pendingGun=Product(Product(root,local),ride);
        if(!Uniform(pendingGun))return false;
        key=k;pendingFrame=frame;pending=true;return true;
    }
    bool ObserveWrist(Key k,const Mat& wrist,const Mat& rawController,
                      unsigned frame,std::uint64_t now,bool normalHands) {
        if(!k.pawn||!k.weapon||!frame||!now||!Uniform(wrist)||
           !Uniform(rawController))return false;
        if(normalHands&&pending&&key==k&&pendingFrame==frame) {
            Mat inverse;
            if(!skeletal::Inverse(inverse.m,wrist.m))return false;
            const Mat relative=Product(inverse,pendingGun);
            if(!Uniform(relative))return false;
            gunFromWrist=relative;tick=now;valid=true;
        }
        if(valid&&!(key==k)){Clear();return false;}
        lastWrist=wrist;lastRawController=rawController;
        wristFrame=frame;wristTick=now;wristValid=true;
        pending=false;return true;
    }
    bool Correct(Key k,const float original[kMaxBones][3][4],unsigned bones,
                 const Mat& local,const Mat& ride,const Mat& rawController,
                 unsigned frame,std::uint64_t now,
                 float result[kMaxBones][3][4],Mat* finalGun=nullptr) const {
        if(!valid||!wristValid||!(key==k)||!frame||frame<=wristFrame||
           !bones||bones>kMaxBones||k.rootSlot>=bones||
           now<tick||now-tick>5000||now<wristTick||now-wristTick>50||
           !Affine(local)||!Uniform(ride)||!Uniform(rawController))return false;
        Mat inverseController,inverseTail,inverseRoot,root;
        const Mat tail=Product(local,ride);
        if(!skeletal::Inverse(inverseController.m,lastRawController.m)||
           !skeletal::Inverse(inverseTail.m,tail.m)||
           !armbind::FromGpu(original[k.rootSlot],root)||!armbind::Rigid(root)||
           !skeletal::Inverse(inverseRoot.m,root.m))return false;
        const Mat predictedWrist=Product(Product(lastWrist,inverseController),rawController);
        const Mat desiredFinal=Product(predictedWrist,gunFromWrist);
        const Mat targetSkin=Product(desiredFinal,inverseTail);
        const Mat delta=Product(inverseRoot,targetSkin);
        if(!Uniform(predictedWrist)||!Uniform(desiredFinal)||
           !armbind::Rigid(targetSkin)||!armbind::Rigid(delta))return false;
        const auto p=armbind::Position(delta);
        if(p.x*p.x+p.y*p.y+p.z*p.z>256.f*256.f)return false;
        float staged[kMaxBones][3][4]{};
        for(unsigned i=0;i<bones;++i){
            Mat check;
            if(!armbind::FromGpu(original[i],check))return false;
            bool empty=true;
            for(const auto& row:original[i])for(float value:row)if(value!=0)empty=false;
            if(empty)std::memcpy(staged[i],original[i],sizeof(staged[i]));
            else skeletal::TransformBone(staged[i],original[i],delta.m);
            if(!armbind::FromGpu(staged[i],check))return false;
        }
        Mat mapped;
        if(!armbind::FromGpu(staged[k.rootSlot],mapped)||
           LargestDifference(Product(Product(mapped,local),ride),desiredFinal)>.03f)return false;
        std::memcpy(result,staged,bones*sizeof(staged[0]));
        if(finalGun)*finalGun=desiredFinal;
        return true;
    }
};
}
