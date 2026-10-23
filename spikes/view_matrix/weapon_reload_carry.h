#pragma once
#include "weapon_reload_runtime.h"

namespace weaponcarry {
using Mat=armbind::Mat;
using Kind=weaponreload::Kind;
enum class WristSource { None, NoTmd, Tmd };
// Only Volk's original TMD glove is a second source of the carried-clip wrist.
// Other weapons retain their accepted noTMD producer; neither route invents a pose.
inline WristSource Source(Kind kind,bool carrying,bool noTmdGlove,bool tmdGlove,
                          bool noTmdPosed,bool leftTracked,unsigned wristBone){
    if(!carrying)return WristSource::None;
    if(noTmdGlove&&noTmdPosed)return WristSource::NoTmd;
    if(kind==Kind::Volk&&tmdGlove&&leftTracked&&wristBone==9u)return WristSource::Tmd;
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
    bool Current(Kind k,std::uint32_t p,std::uint32_t w,std::uint64_t id,std::uint64_t now)const{
        return valid&&k==kind&&p&&w&&id&&p==pawn&&w==weapon&&id==transaction&&
            now>=tick&&now-tick<=150&&armbind::Finite(tracking);
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
