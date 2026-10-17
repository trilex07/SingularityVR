#pragma once
#include "centurion_cylinder_animation.h"
#include "pose_conversion.h"

// Original native material/geometry replay. No D3D state ownership or UE3 calls.
namespace centurionvisual {
using armbind::Mat;using hipknife::V;using hipknife::Pose;
inline Mat Identity(){Mat m;for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
inline Mat Product(const Mat& a,const Mat& b){Mat out;skeletal::Multiply(out.m,a.m,b.m);return out;}
inline V Point(V v,const Mat& m){return {
    v.x*m.m[0][0]+v.y*m.m[1][0]+v.z*m.m[2][0]+m.m[3][0],
    v.x*m.m[0][1]+v.y*m.m[1][1]+v.z*m.m[2][1]+m.m[3][1],
    v.x*m.m[0][2]+v.y*m.m[1][2]+v.z*m.m[2][2]+m.m[3][2]};}
inline bool Tail(std::uint64_t closed,std::uint64_t now){return closed&&now>=closed&&now-closed<2600;}
inline centurionanim::PoseTime Time(centurionreload::Phase phase,std::uint64_t since,
                                    std::uint64_t closed,std::uint64_t now){
    if(now<since)return {};
    const float age=float(std::min<std::uint64_t>(now-since,1000));
    using P=centurionreload::Phase;
    if(Tail(closed,now))return {true,false,26.f+6.f*std::min(float(now-closed)/280.f,1.f)};
    switch(phase){
    case P::Unlocked:return {true,false,10.f*std::min(age/120.f,1.f)};
    case P::Open:return {true,false,11.f+std::min(age/180.f,1.f)};
    case P::NeedAmmo:return {true,age>=100,12.f};
    case P::Carrying:return {true,true,12.f};
    case P::Loaded:return {true,false,26.f}; // stays visibly unseated until flick
    case P::WaitingNative:return {true,false,32.f};
    default:return {};
    }
}
struct RootHold {
    std::uint32_t pawn=0,weapon=0;std::uint64_t transaction=0;Mat root{};bool valid=false;
    bool Get(std::uint32_t p,std::uint32_t w,std::uint64_t id,const float (&native)[15][4],Mat& out){
        if(!p||!w||!id)return false;
        if(!valid||p!=pawn||w!=weapon||id!=transaction){
            Mat next;if(!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(native+3),next)||
                !armbind::Rigid(next))return false;
            pawn=p;weapon=w;transaction=id;root=next;valid=true;
        }
        out=root;return true;
    }
};
inline bool Attached(const centurionanim::Data& d,centurionanim::PoseTime time,
                     const float (&native)[15][4],const Mat& root,float (&out)[15][4]){
    if(!armbind::Rigid(root))return false;
    float staged[15][4];std::memcpy(staged,native,sizeof(staged));
    armbind::ToGpu(root,reinterpret_cast<float(*)[4]>(staged+3));
    return centurionanim::Palette(d,time,staged,out);
}
inline bool TrackingMap(V centre,float yaw,float base,float units,Mat& out){
    if(!hipknife::Finite(centre)||!std::isfinite(yaw)||!std::isfinite(base)||
       !std::isfinite(units)||units<10||units>1000)return false;
    Mat m=Identity();
    for(unsigned r=0;r<4;++r){
        vrpose::V3 v{};if(r<3){if(r==0)v.x=1;if(r==1)v.y=1;if(r==2)v.z=1;}
        else v={-centre.x,-centre.y,-centre.z};
        const auto p=vrpose::TrackingDeltaToWorldUU(v,yaw,base,units);
        m.m[r][0]=p.x;m.m[r][1]=p.y;m.m[r][2]=p.z;
    }
    if(!armbind::Finite(m))return false;out=m;return true;
}
inline bool SpareDesired(const centurionanim::Data& d,const Mat& local,const Mat& gunC,
                         const Mat& trackingMap,const Mat& desired,float (&out)[15][4]){
    if(!d.ready||!armbind::Finite(desired))return false;
    Mat undoGun;const auto gun=Product(local,gunC);
    if(!skeletal::Inverse(undoGun.m,gun.m))return false;
    const auto clip=Product(Product(Product(d.inverse[0],desired),trackingMap),undoGun);
    if(!armbind::Finite(clip))return false;
    float staged[15][4]{};
    for(unsigned slot=0;slot<5;++slot)for(unsigned row=0;row<3;++row)staged[slot*3+row][3]=clip.m[3][row];
    armbind::ToGpu(clip,reinterpret_cast<float(*)[4]>(staged));
    std::memcpy(out,staged,sizeof(staged));return true;
}
inline bool Spare(const centurionanim::Data& d,const Mat& root,const Mat& local,const Mat& gunC,
                  const Mat& trackingMap,const Pose& aim,const Pose& target,float (&out)[15][4]){
    if(!d.ready||!armbind::Rigid(root)||!armbind::Finite(local)||!armbind::Finite(gunC)||
       !armbind::Finite(trackingMap)||!hipknife::Valid(aim.q)||!hipknife::Valid(target.q)||
       !hipknife::Finite(target.p))return false;
    Mat bind,toTracking;
    const auto gun=Product(local,gunC);
    if(!skeletal::Inverse(bind.m,d.inverse[0].m)||!skeletal::Inverse(toTracking.m,trackingMap.m))return false;
    const auto observed=Product(Product(Product(bind,root),gun),toTracking);
    Mat desired=Identity();const hipknife::Q inv{-aim.q.x,-aim.q.y,-aim.q.z,aim.q.w};
    for(unsigned r=0;r<3;++r){
        const auto v=hipknife::Rotate(target.q,hipknife::Rotate(inv,
            {observed.m[r][0],observed.m[r][1],observed.m[r][2]}));
        if(!hipknife::Finite(v)||hipknife::Length(v)<.0001f||hipknife::Length(v)>2)return false;
        desired.m[r][0]=v.x;desired.m[r][1]=v.y;desired.m[r][2]=v.z;
    }
    desired.m[3][0]=target.p.x;desired.m[3][1]=target.p.y;desired.m[3][2]=target.p.z;
    return SpareDesired(d,local,gunC,trackingMap,desired,out);
}
// In production upload/draw are the original setters/DIP inside EACH eye.
// Restore even if upload or draw fails (including drivers with partial writes).
template<class Upload,class Draw>bool Replay(Upload upload,Draw draw,const float (&spare)[15][4],
    const float (&attached)[15][4],bool& restored){
    const bool changed=upload(spare);const bool drawn=changed&&draw();
    restored=upload(attached);return drawn&&restored;
}
} // namespace centurionvisual
