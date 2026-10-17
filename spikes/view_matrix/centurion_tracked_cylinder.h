#pragma once
#include "centurion_visual_followup.h"

// No D3D/UE3 calls. Full native -> translated-world -> XR -> controller chain.
namespace centuriontracked {
using armbind::Mat;using hipknife::Pose;using hipknife::V;
using centurionvisual::Identity;using centurionvisual::Product;
using centurionvisual::Point;using centurionvisual::Tail;
using centurionvisual::TrackingMap;
inline bool PoseMatrix(const Pose& p,Mat& out){
    if(!hipknife::Finite(p.p)||!hipknife::Valid(p.q))return false;
    Mat m=Identity();
    for(unsigned r=0;r<3;++r){V v{};if(r==0)v.x=1;if(r==1)v.y=1;if(r==2)v.z=1;
        v=hipknife::Rotate(p.q,v);m.m[r][0]=v.x;m.m[r][1]=v.y;m.m[r][2]=v.z;}
    m.m[3][0]=p.p.x;m.m[3][1]=p.p.y;m.m[3][2]=p.p.z;out=m;return true;
}
inline bool Inverse(const Mat& m,Mat& out){return armbind::Finite(m)&&skeletal::Inverse(out.m,m.m)&&armbind::Finite(out);}
struct RootHold {
    std::uint32_t pawn=0,weapon=0;std::uint64_t transaction=0;Mat controllerLocal{};bool valid=false;
    bool Get(std::uint32_t p,std::uint32_t w,std::uint64_t id,const float (&native)[15][4],
             const Mat& local,const Mat& gun,const Mat& tracking,const Pose& aim,Mat& out){
        if(!p||!w||!id)return false;
        Mat aimMatrix,invAim,invTracking,invChain;
        const auto chain=Product(local,gun);
        if(!PoseMatrix(aim,aimMatrix)||!Inverse(aimMatrix,invAim)||!Inverse(tracking,invTracking)||!Inverse(chain,invChain))return false;
        Mat relative=controllerLocal;
        const bool changed=!valid||p!=pawn||w!=weapon||id!=transaction;
        if(changed){Mat original;
            if(!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(native+3),original)||!armbind::Rigid(original))return false;
            relative=Product(Product(Product(original,chain),invTracking),invAim);
        }
        const auto result=Product(Product(Product(relative,aimMatrix),tracking),invChain);
        if(!armbind::Rigid(result))return false;
        if(changed){pawn=p;weapon=w;transaction=id;controllerLocal=relative;valid=true;}
        out=result;return true;
    }
};
inline float OpenAmount(centurionreload::Phase phase,std::uint64_t since,std::uint64_t closed,std::uint64_t now){
    using P=centurionreload::Phase;
    if(now<since)return 0;
    if(Tail(closed,now))return 1.f-std::min(float(now-closed)/280.f,1.f);
    switch(phase){
    case P::Unlocked:case P::Open:return std::min(float(now-since)/160.f,1.f);
    case P::NeedAmmo:case P::Carrying:case P::Loaded:return 1;
    default:return 0;
    }
}
inline bool Attached(const centurionanim::Data& d,centurionanim::PoseTime time,float amount,
    const float (&native)[15][4],const Mat& root,const Mat& local,const Mat& gun,
    const Mat& tracking,const Pose& aim,float (&out)[15][4],V& socket){
    if(!d.ready||!time.active||!std::isfinite(amount)||amount<0||amount>1||!armbind::Rigid(root))return false;
    Mat original,invOriginal,invTracking,invChain,bind;
    const auto chain=Product(local,gun);
    if(!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(native+3),original)||!Inverse(original,invOriginal)||
       !Inverse(tracking,invTracking)||!Inverse(chain,invChain)||!Inverse(d.inverse[0],bind)||
       !hipknife::Valid(aim.q))return false;
    float staged[15][4];
    // Compensate parent motion for EVERY body/hammer/trigger bone. The rigid
    // clip/release tracks are overwritten below; no free-floating trigger.
    for(unsigned slot=0;slot<5;++slot){Mat bone;
        if(!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(native+slot*3),bone))return false;
        const auto held=Product(Product(bone,invOriginal),root);
        if(!armbind::Finite(held))return false;
        armbind::ToGpu(held,reinterpret_cast<float(*)[4]>(staged+slot*3));
    }
    auto clipTracking=Product(Product(Product(bind,root),chain),invTracking);
    const float scale=hipknife::Length({clipTracking.m[0][0],clipTracking.m[0][1],clipTracking.m[0][2]});
    if(!std::isfinite(scale)||scale<.0001f||scale>.2f)return false;
    // Native cylinder diameter ~5.284UU. Slide left by diameter+small clearance,
    // in the PHYSICAL controller frame, never reflected mesh/guessed PSA axis.
    const auto offset=hipknife::Rotate(aim.q,{-5.6f*scale*amount,0,0});
    clipTracking.m[3][0]+=offset.x;clipTracking.m[3][1]+=offset.y;clipTracking.m[3][2]+=offset.z;
    const V centre=Point({},clipTracking);
    const auto clipSkin=Product(Product(Product(d.inverse[0],clipTracking),tracking),invChain);
    if(!armbind::Finite(clipSkin)||!hipknife::Finite(centre))return false;
    auto shown=clipSkin;
    if(time.hideClip)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)shown.m[r][c]=0;
    armbind::ToGpu(shown,reinterpret_cast<float(*)[4]>(staged));
    Mat release;
    if(!centurionanim::Sample(d,1,10.f*amount,release))return false;
    const auto releaseSkin=Product(release,root);
    armbind::ToGpu(releaseSkin,reinterpret_cast<float(*)[4]>(staged+12));
    std::memcpy(out,staged,sizeof(staged));socket=centre;return true;
}
struct SocketWitness {
    std::uint32_t pawn=0,weapon=0;std::uint64_t transaction=0,ms=0;V point{};bool valid=false;
    bool Current(std::uint32_t p,std::uint32_t w,std::uint64_t id,std::uint64_t now)const{
        return valid&&p&&w&&id&&p==pawn&&w==weapon&&id==transaction&&now>=ms&&now-ms<=100&&hipknife::Finite(point);
    }
};
} // namespace centuriontracked
