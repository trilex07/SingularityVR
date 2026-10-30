#pragma once
#include "weapon_reload_runtime.h"

namespace weaponcarry {
using Mat=armbind::Mat;
using Kind=weaponreload::Kind;
enum class WristSource { None, NoTmd, Tmd };
inline bool CarrySectionEligible(Kind kind,bool noTmdGlove){
    return noTmdGlove||weaponreload::Index(kind)<3;
}
// All three existing magazine mounts are wrist-local. The original TMD
// material-0 publishes that same physical left wrist after stereo submission.
inline WristSource Source(Kind kind,bool carrying,bool noTmdGlove,bool tmdGlove,
                          bool noTmdPosed,bool leftTracked,unsigned wristBone){
    if(!carrying)return WristSource::None;
    if(noTmdGlove&&noTmdPosed)return WristSource::NoTmd;
    if(weaponreload::Index(kind)<3&&tmdGlove&&leftTracked&&wristBone==9u)
        return WristSource::Tmd;
    return WristSource::None;
}
inline float Curl(Kind kind){return kind==Kind::Kasimov?.35f:.25f;}
// Original glove + original clip, fit_magazine_palm.py: zero intersecting
// triangles and zero sampled containment. Units are NATIVE wrist UU, not XR.
inline bool Mount(Kind kind,Mat& out){
    if(weaponreload::Index(kind)>=3)return false;
    Mat m{};m.m[0][0]=-1;m.m[3][3]=1;
    if(kind==Kind::Kasimov){m.m[1][1]=-1;m.m[2][2]=1;
        m.m[3][0]=5.02334944f;m.m[3][1]=-.50722513f;m.m[3][2]=3.90860947f;
    }else{m.m[1][2]=-1;m.m[2][1]=-1;
        if(kind==Kind::Valkyrie){m.m[3][0]=5.05332961f;m.m[3][1]=-3.56168574f;m.m[3][2]=.62759385f;}
        else{m.m[3][0]=5.99999568f;m.m[3][1]=-8.88449250f;m.m[3][2]=3.80813382f;}
    }
    out=m;return true;
}
struct Wrist {
    bool valid=false;Kind kind=Kind::None;std::uint32_t pawn=0,weapon=0;
    std::uint64_t transaction=0,tick=0;Mat tracking{};
    WristSource source=WristSource::None;
    // Pose from the same tracked-controller sample as the final bound wrist.
    // Used only if that glove draw temporarily stops publishing while carried.
    hipknife::Pose sensor{};std::uint64_t sensorSampleMs=0;
    bool Current(Kind k,std::uint32_t p,std::uint32_t w,std::uint64_t id,std::uint64_t now)const{
        return valid&&k==kind&&p&&w&&id&&p==pawn&&w==weapon&&id==transaction&&
            now>=tick&&now-tick<=150&&armbind::Finite(tracking);
    }
    bool Projected(Kind k,std::uint32_t p,std::uint32_t w,std::uint64_t id,
                   std::uint64_t now,const hipknife::Pose& live,
                   std::uint64_t liveSampleMs,bool tracked,Mat& out)const{
        if(!tracked||!valid||k!=kind||!p||!w||!id||p!=pawn||w!=weapon||id!=transaction||
           now<tick||now-tick<=150||now-tick>350||!sensorSampleMs||
           tick<sensorSampleMs||tick-sensorSampleMs>40||
           now<liveSampleMs||now-liveSampleMs>40||
           !hipknife::Finite(sensor.p)||!hipknife::Valid(sensor.q)||
           !hipknife::Finite(live.p)||!hipknife::Valid(live.q)||
           hipknife::Length(live.p-sensor.p)>.75f||!armbind::Finite(tracking))return false;
        const auto poseMatrix=[](const hipknife::Pose& pose){
            Mat m{};m.m[3][3]=1.f;
            for(unsigned row=0;row<3;++row){
                hipknife::V v{};if(row==0)v.x=1.f;else if(row==1)v.y=1.f;else v.z=1.f;
                v=hipknife::Rotate(pose.q,v);
                m.m[row][0]=v.x;m.m[row][1]=v.y;m.m[row][2]=v.z;
            }
            m.m[3][0]=pose.p.x;m.m[3][1]=pose.p.y;m.m[3][2]=pose.p.z;
            return m;
        };
        const Mat initial=poseMatrix(sensor),current=poseMatrix(live);
        Mat inverse,relative,result;
        if(!skeletal::Inverse(inverse.m,initial.m))return false;
        skeletal::Multiply(relative.m,tracking.m,inverse.m);
        skeletal::Multiply(result.m,relative.m,current.m);
        if(!armbind::Finite(result))return false;
        out=result;return true;
    }
    bool Resolve(Kind k,std::uint32_t p,std::uint32_t w,std::uint64_t id,
                 std::uint64_t now,const hipknife::Pose& live,
                 std::uint64_t liveSampleMs,bool tracked,bool continuity,
                 Mat& out,bool* continued=nullptr)const{
        if(continued)*continued=false;
        if(Current(k,p,w,id,now)){out=tracking;return true;}
        if(!continuity||!Projected(k,p,w,id,now,live,liveSampleMs,tracked,out))return false;
        if(continued)*continued=true;
        return true;
    }
};
// The engine submits the same foreground draw to a depth prime and a colour
// target in one XR frame. A new glove witness may arrive between those draws.
// Keep the first validated physical wrist for the whole frame so the spare
// magazine has one world pose in both passes (and both eyes).
struct FrameWrist {
    std::uint32_t frame=0,pawn=0,weapon=0;
    std::uint64_t transaction=0;
    Kind kind=Kind::None;
    bool sampled=false,valid=false;
    Mat tracking{};
    bool Select(std::uint32_t nextFrame,Kind nextKind,std::uint32_t nextPawn,
                std::uint32_t nextWeapon,std::uint64_t nextTransaction,
                bool currentValid,const Mat& current,Mat& out,bool* reused=nullptr){
        const bool same=sampled&&frame==nextFrame&&kind==nextKind&&pawn==nextPawn&&
            weapon==nextWeapon&&transaction==nextTransaction;
        if(reused)*reused=same;
        if(!same){
            frame=nextFrame;kind=nextKind;pawn=nextPawn;weapon=nextWeapon;
            transaction=nextTransaction;sampled=true;
            valid=currentValid&&armbind::Finite(current);
            if(valid)tracking=current;
        }
        if(valid)out=tracking;
        return valid;
    }
};
inline bool WristTracking(const Mat& bind,const float skin[3][4],const Mat& chain,
                          const Mat& world,Mat& out){
    Mat joint,inv;
    if(!armbind::CurrentJoint(bind,skin,joint)||!skeletal::Inverse(inv.m,world.m))return false;
    Mat a,result;skeletal::Multiply(a.m,joint.m,chain.m);skeletal::Multiply(result.m,a.m,inv.m);
    if(!armbind::Finite(result))return false;out=result;return true;
}
inline bool Matrix(Kind kind,const Mat& wrist,Mat& out){
    Mat mount,metres{},a,result;if(!Mount(kind,mount)||!armbind::Finite(wrist))return false;
    // SVRK = reflected clip-local UU / 52.5. Undo it exactly once before mount.
    metres.m[0][0]=52.5f;metres.m[1][1]=-52.5f;metres.m[2][2]=52.5f;metres.m[3][3]=1;
    skeletal::Multiply(a.m,metres.m,mount.m);skeletal::Multiply(result.m,a.m,wrist.m);
    if(!weaponreload::PhysicalMatrix(result))return false;out=result;return true;
}
} // namespace weaponcarry
