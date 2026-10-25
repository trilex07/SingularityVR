#pragma once
#include "arm_bind.h"
#include <cstdint>

// Original ch_inview_tmd_mesh SkeletalMeshSocket TMD_Muzzle, bone l_arm_wrist.
// These are native socket properties, not ActorX points or controller offsets.
namespace tmdmuzzle {
inline bool Affine(const armbind::Mat& m) {
    if(!armbind::Finite(m)||std::fabs(m.m[0][3])+std::fabs(m.m[1][3])+
       std::fabs(m.m[2][3])+std::fabs(m.m[3][3]-1)>1e-4f)return false;
    armbind::Mat inverse;return skeletal::Inverse(inverse.m,m.m);
}
struct Witness {
    armbind::Mat wrist;
    std::uintptr_t pawn=0;
    std::uint64_t tick=0;
    std::uint32_t frame=0;
    bool valid=false;
    bool Current(std::uintptr_t owner,std::uint64_t now) const {
        return valid&&owner&&pawn==owner&&tick&&now>=tick&&now-tick<=150;
    }
};
inline armbind::Mat SocketLocal() {
    const double uu=6.2831853071795864769/65536.;
    const double p=-2495*uu,y=-3631*uu,r=1137*uu;
    const float sp=float(std::sin(p)),cp=float(std::cos(p)),sy=float(std::sin(y)),
        cy=float(std::cos(y)),sr=float(std::sin(r)),cr=float(std::cos(r));
    armbind::Mat m;
    m.m[0][0]=cp*cy;m.m[0][1]=cp*sy;m.m[0][2]=sp;
    m.m[1][0]=sr*sp*cy-cr*sy;m.m[1][1]=sr*sp*sy+cr*cy;m.m[1][2]=-sr*cp;
    m.m[2][0]=-(cr*sp*cy+sr*sy);m.m[2][1]=cy*sr-cr*sp*sy;m.m[2][2]=cr*cp;
    m.m[3][0]=5.02409f;m.m[3][1]=-7.532f;m.m[3][2]=-.710843f;m.m[3][3]=1;
    return m;
}
inline bool Capture(const armbind::Mat& bind,const float skin[3][4],
        const armbind::Mat& local,const armbind::Mat& ride,const float camera[3],
        std::uintptr_t pawn,std::uint64_t tick,std::uint32_t frame,Witness& out) {
    if(!pawn||!tick||!camera||!Affine(local)||!Affine(ride))return false;
    armbind::Mat joint,world;
    if(!armbind::CurrentJoint(bind,skin,joint))return false;
    skeletal::Multiply(world.m,joint.m,local.m);
    Witness next;skeletal::Multiply(next.wrist.m,world.m,ride.m);
    for(unsigned i=0;i<3;++i){if(!std::isfinite(camera[i]))return false;next.wrist.m[3][i]+=camera[i];}
    if(!Affine(next.wrist))return false;
    next.pawn=pawn;next.tick=tick;next.frame=frame;next.valid=true;out=next;return true;
}
inline bool SameEyes(const Witness& a,const Witness& b) {
    if(!a.valid||!b.valid||a.pawn!=b.pawn||a.frame!=b.frame)return false;
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)
        if(std::fabs(a.wrist.m[i][j]-b.wrist.m[i][j])>.002f)return false;
    return true;
}
inline bool Socket(const Witness& w,std::uintptr_t pawn,std::uint64_t now,armbind::Mat& result) {
    if(!w.Current(pawn,now))return false;
    const auto socket=SocketLocal();armbind::Mat m;
    skeletal::Multiply(m.m,socket.m,w.wrist.m);
    if(!armbind::Finite(m))return false;result=m;return true;
}
inline bool Completed(const Witness (&eyes)[2],const bool (&drawn)[2],unsigned eyeCount,
        bool drawOk,bool indexRestored,bool handRestored,bool nextRestored) {
    return eyeCount==2&&drawn[0]&&drawn[1]&&drawOk&&indexRestored&&handRestored&&
        nextRestored&&SameEyes(eyes[0],eyes[1]);
}
} // namespace tmdmuzzle
