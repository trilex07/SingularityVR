#pragma once
#include "arm_bind.h"
#include "hand_anchor_math.h"
#include <cstdint>

namespace seekergeometry {
using armbind::Mat;
// Original PSK/PSA/cooked measurements; reproduce with audit_seeker_geometry.py.
// Native cooked axes, NOT ActorX axes. These are attachment facts, not mesh data.
inline constexpr unsigned IndexBytes=44988*2,IndexHash=3842523959u;
inline constexpr Mat RootBind{{
    {1.f,3.6519772e-8f,-4.80625e-8f,0},
    {-3.6519773e-8f,1.f,-1.6836574e-8f,0},
    {4.80625e-8f,1.6836576e-8f,1.f,0},
    {4.187307e-6f,-109.49099916f,44.33592962f,1}}};
inline constexpr Mat LeftGrip{{
    {.046796019f,-.964132087f,-.261265098f,0},
    {.734438542f,.210489137f,-.645208766f,0},
    {.677059939f,-.161689956f,.717945818f,0},
    {5.368237084f,6.006297823f,10.267012817f,1}}};
inline constexpr armik::V Muzzle{.000006786479f,-112.88768866f,95.83399562f};
inline constexpr armik::V Forward{4.80625e-8f,1.6836576e-8f,1.f};
inline bool Vertices(unsigned n){return n==12444||n==12436;}
inline bool Body(int base,unsigned min,unsigned vertices,unsigned first,unsigned triangles){
    return base==0&&min==0&&Vertices(vertices)&&first==0&&triangles==14990;
}
inline Mat Product(const Mat& a,const Mat& b){Mat r;skeletal::Multiply(r.m,a.m,b.m);return r;}
struct Witness {
    bool valid=false;uint32_t pawn=0,weapon=0;uint64_t tick=0;
    Mat rootLocal{}; // native root-bind * source skin * source LocalToWorld; NO controller/eye
    bool Current(uint32_t p,uint32_t w,uint64_t now)const{
        return valid&&p&&w&&p==pawn&&w==weapon&&now>=tick&&now-tick<=150&&armbind::Finite(rootLocal);
    }
};
inline bool Capture(const Mat& rootBind,const Mat& skin,const Mat& local,uint32_t pawn,uint32_t weapon,uint64_t now,Witness& out){
    if(!pawn||!weapon||!handanchor::Affine(skin.m)||!handanchor::Affine(local.m))return false;
    const auto root=Product(Product(rootBind,skin),local);
    Mat inv;if(!skeletal::Inverse(inv.m,root.m)||!armbind::Finite(root))return false;
    out={true,pawn,weapon,now,root};return true;
}
inline bool Capture(const Mat& skin,const Mat& local,uint32_t pawn,uint32_t weapon,uint64_t now,Witness& out){
    return Capture(RootBind,skin,local,pawn,weapon,now,out);
}
// Solve a complete target C, not an additive guess: wrist * local * C =
// authored left-wrist-relative-to-gun * current gun-root * current BuildGunC.
inline bool SupportTarget(const Mat& leftGrip,const Witness& w,uint32_t pawn,uint32_t weapon,uint64_t now,
        const Mat& wristBind,const float skin[3][4],const Mat& local,const Mat& gun,Mat& out){
    if(!w.Current(pawn,weapon,now)||!handanchor::Affine(gun.m)||!handanchor::Affine(local.m))return false;
    Mat wrist,inverse;
    if(!armbind::CurrentJoint(wristBind,skin,wrist))return false;
    const auto source=Product(wrist,local);
    if(!skeletal::Inverse(inverse.m,source.m))return false;
    const auto desired=Product(Product(leftGrip,w.rootLocal),gun);
    const auto result=Product(inverse,desired);
    if(!handanchor::Affine(result.m))return false;
    out=result;return true;
}
inline bool SupportTarget(const Witness& w,uint32_t pawn,uint32_t weapon,uint64_t now,
        const Mat& wristBind,const float skin[3][4],const Mat& local,const Mat& gun,Mat& out){
    return SupportTarget(LeftGrip,w,pawn,weapon,now,wristBind,skin,local,gun,out);
}
// Independent Seeker-only budget: earlier ordinary/TMD draws cannot exhaust it.
struct SupportProbeBudget {
    uint32_t last[4]{}; unsigned frames[4]{};
    bool Frame(uint32_t frame,unsigned hands,bool support){
        if(!frame || hands<1 || hands>2)return false;
        const unsigned key=(hands-1)*2+(support?1:0);
        if(last[key]!=frame){last[key]=frame;++frames[key];}
        return frames[key]<=3;
    }
};
struct SupportFrame {
    bool valid=false;uint32_t pawn=0,weapon=0,frame=0;Mat local{},target{};
    bool Read(uint32_t p,uint32_t w,uint32_t f,const Mat& sourceLocal,Mat& out)const{
        if(!valid||p!=pawn||w!=weapon||f!=frame||!p||!w)return false;
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
            if(std::fabs(local.m[r][c]-sourceLocal.m[r][c])>.0001f)return false;
        out=target;return true;
    }
};
}
