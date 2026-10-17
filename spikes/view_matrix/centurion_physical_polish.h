#pragma once
#include "centurion_tracked_cylinder.h"

namespace centurionphysical {
using namespace centuriontracked;
// Fit on original noTMD glove and native clip, not aim-space centimetre guesses.
// fit_cylinder_palm.py: 1482 glove/1252 clip triangles, ZERO intersections.
constexpr float Curl=.45f;
inline Mat PalmMount(){auto m=Identity();m.m[3][0]=6.f;m.m[3][1]=-7.f;return m;}
struct GripWitness {
    SocketWitness identity{};Mat desired{};
    bool Current(std::uint32_t p,std::uint32_t w,std::uint64_t id,std::uint64_t now)const{
        return identity.Current(p,w,id,now)&&armbind::Finite(desired);
    }
};
inline bool Grip(const Mat& wristBind,const float wrist[3][4],const Mat& chain,const Mat& tracking,Mat& out){
    Mat skin,invTracking;
    if(!armbind::FromGpu(wrist,skin)||!Inverse(tracking,invTracking))return false;
    const auto result=Product(Product(Product(Product(PalmMount(),wristBind),skin),chain),invTracking);
    if(!armbind::Finite(result))return false;out=result;return true;
}
inline bool MatchSize(const Mat& grip,const Mat& observed,Mat& out){
    const float native=hipknife::Length({observed.m[0][0],observed.m[0][1],observed.m[0][2]});
    const float hand=hipknife::Length({grip.m[0][0],grip.m[0][1],grip.m[0][2]});
    if(!armbind::Finite(grip)||!armbind::Finite(observed)||hand<.0001f||native<.0001f||native>.2f)return false;
    auto staged=grip;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)staged.m[r][c]*=native/hand;
    out=staged;return true;
}
// Freeze ONLY the right hand's shape relative to its tracked aim controller.
// Keep native engine reload/notifies/ammo running underneath. Changing local/
// component/view transforms cannot bring the flat-game reload sweep back.
struct RightHold {
    std::uint32_t pawn=0,weapon=0;std::uint64_t transaction=0;unsigned count=0;
    Mat relative[45]{};bool valid=false;
    bool Apply(std::uint32_t p,std::uint32_t w,std::uint64_t id,bool capture,
        float (&palette)[135][4],unsigned slots,const bool* right,
        const Mat& chain,const Mat& tracking,const Pose& aim){
        if(!p||!w||!id||!slots||slots>45||!right)return false;
        Mat pose,invPose,invTracking,invChain;
        if(!PoseMatrix(aim,pose)||!Inverse(pose,invPose)||!Inverse(tracking,invTracking)||!Inverse(chain,invChain))return false;
        const bool changed=!valid||pawn!=p||weapon!=w||transaction!=id||count!=slots;
        if(changed&&!capture)return false;
        Mat staged[45];float output[135][4];std::memcpy(output,palette,sizeof(output));
        for(unsigned i=0;i<slots;++i)if(right[i]){
            if(changed){Mat bone;if(!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(palette+i*3),bone))return false;
                staged[i]=Product(Product(Product(bone,chain),invTracking),invPose);
            }else staged[i]=relative[i];
            const auto bone=Product(Product(Product(staged[i],pose),tracking),invChain);
            if(!armbind::Finite(bone))return false;
            armbind::ToGpu(bone,reinterpret_cast<float(*)[4]>(output+i*3));
        }
        if(changed){for(unsigned i=0;i<slots;++i)if(right[i])relative[i]=staged[i];
            pawn=p;weapon=w;transaction=id;count=slots;valid=true;}
        std::memcpy(palette,output,sizeof(output));return true;
    }
};
} // namespace centurionphysical
