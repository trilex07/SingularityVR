#pragma once
#include "arm_bind.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

// The weapon body is submitted before the material-0 glove in each UE3 frame.
// Keep the last paired HANDS_WEAPON gun/wrist pose as the anchor and advance it
// by the RAW controller delta. The HANDS_TMD wrist is a different native pose:
// it confirms that stereo tracking is still live but must not replace the gun
// anchor. Otherwise successive color submissions in one frame move the gun to
// different positions (or stop retargeting after the 50 ms wrist timeout).
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
enum class CorrectGate : unsigned char {
    Pass, Reference, Frame, Matrix, Inverse, Root, Predicted, Target,
    Delta, Displacement, Bone, FinalWitness
};
inline const char* GateName(CorrectGate gate) {
    switch(gate) {
    case CorrectGate::Pass: return "pass";
    case CorrectGate::Reference: return "reference";
    case CorrectGate::Frame: return "frame";
    case CorrectGate::Matrix: return "matrix";
    case CorrectGate::Inverse: return "inverse";
    case CorrectGate::Root: return "root";
    case CorrectGate::Predicted: return "predicted";
    case CorrectGate::Target: return "target";
    case CorrectGate::Delta: return "delta";
    case CorrectGate::Displacement: return "displacement";
    case CorrectGate::Bone: return "bone";
    case CorrectGate::FinalWitness: return "final-witness";
    }
    return "unknown";
}
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
    Mat gunFromWrist{},normalWrist{},normalRawController{};
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
        if(valid&&!(key==k))Clear();
        pendingGun=Product(Product(root,local),ride);
        if(!Uniform(pendingGun))return false;
        key=k;pendingFrame=frame;pending=true;return true;
    }
    bool ObserveWrist(Key k,const Mat& wrist,const Mat& rawController,
                      unsigned frame,std::uint64_t now,bool normalHands) {
        if(!k.pawn||!k.weapon||!frame||!now||!Uniform(wrist)||
           !Uniform(rawController))return false;
        if(valid&&!(key==k)){Clear();return false;}
        if(normalHands) {
            if(!pending||!(key==k)||pendingFrame!=frame){pending=false;return false;}
            Mat inverse;
            if(!skeletal::Inverse(inverse.m,wrist.m))return false;
            const Mat relative=Product(inverse,pendingGun);
            if(!Uniform(relative))return false;
            gunFromWrist=relative;tick=now;valid=true;
            normalWrist=wrist;normalRawController=rawController;
        } else if(!valid||!wristValid||frame<wristFrame||
                  frame-wristFrame>2||now<wristTick||now-wristTick>500) {
            pending=false;return false;
        }
        // In TMD mode the rendered wrist can use another animation and root.
        // Its successful stereo draw is a freshness witness only. Retaining the
        // normal wrist makes every gun pass in this frame use one rigid pose.
        wristFrame=frame;wristTick=now;wristValid=true;
        pending=false;return true;
    }
    bool Correct(Key k,const float original[kMaxBones][3][4],unsigned bones,
                 const Mat& local,const Mat& ride,const Mat& rawController,
                 unsigned frame,std::uint64_t now,
                 float result[kMaxBones][3][4],Mat* finalGun=nullptr,
                 CorrectGate* rejected=nullptr) const {
        if(rejected)*rejected=CorrectGate::Pass;
        const auto fail=[&](CorrectGate gate) {if(rejected)*rejected=gate;return false;};
        if(!valid||!wristValid||!(key==k))return fail(CorrectGate::Reference);
        if(!frame||frame<wristFrame||frame-wristFrame>2||
           now<tick||now<wristTick||now-wristTick>500)return fail(CorrectGate::Frame);
        if(!bones||bones>kMaxBones||k.rootSlot>=bones||
           !Affine(local)||!Uniform(ride)||!Uniform(rawController))return fail(CorrectGate::Matrix);
        Mat inverseController,inverseTail,inverseRoot,root;
        const Mat tail=Product(local,ride);
        if(!skeletal::Inverse(inverseController.m,normalRawController.m)||
           !skeletal::Inverse(inverseTail.m,tail.m))return fail(CorrectGate::Inverse);
        if(!armbind::FromGpu(original[k.rootSlot],root)||!armbind::Rigid(root)||
           !skeletal::Inverse(inverseRoot.m,root.m))return fail(CorrectGate::Root);
        const Mat predictedWrist=Product(Product(normalWrist,inverseController),rawController);
        const Mat desiredFinal=Product(predictedWrist,gunFromWrist);
        const Mat targetSkin=Product(desiredFinal,inverseTail);
        const Mat delta=Product(inverseRoot,targetSkin);
        if(!Uniform(predictedWrist)||!Uniform(desiredFinal))return fail(CorrectGate::Predicted);
        if(!armbind::Rigid(targetSkin))return fail(CorrectGate::Target);
        if(!armbind::Rigid(delta))return fail(CorrectGate::Delta);
        const auto p=armbind::Position(delta);
        if(p.x*p.x+p.y*p.y+p.z*p.z>256.f*256.f)return fail(CorrectGate::Displacement);
        float staged[kMaxBones][3][4]{};
        for(unsigned i=0;i<bones;++i){
            Mat check;
            if(!armbind::FromGpu(original[i],check))return fail(CorrectGate::Bone);
            bool empty=true;
            for(const auto& row:original[i])for(float value:row)if(value!=0)empty=false;
            if(empty)std::memcpy(staged[i],original[i],sizeof(staged[i]));
            else skeletal::TransformBone(staged[i],original[i],delta.m);
            if(!armbind::FromGpu(staged[i],check))return fail(CorrectGate::Bone);
        }
        Mat mapped;
        if(!armbind::FromGpu(staged[k.rootSlot],mapped)||
           LargestDifference(Product(Product(mapped,local),ride),desiredFinal)>.03f)
            return fail(CorrectGate::FinalWitness);
        std::memcpy(result,staged,bones*sizeof(staged[0]));
        if(finalGun)*finalGun=desiredFinal;
        return true;
    }
};
}
