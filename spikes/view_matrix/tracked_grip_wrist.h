#pragma once
#include "arm_bind.h"
#include "dual_hand_reference.h"
#include "glove_animation.h"
#include "pose_conversion.h"

// Native idle attachment orientation, driven by the complete grip quaternion.
// Bind finger direction is not the controller mounting frame (L3 disproved it).
namespace trackedgrip {
using armbind::Mat;
using armik::V;
// Native camera rotation is read after the engine camera update in the VS hook.
// Component LocalToWorld already includes that rotation. Remove it before
// applying the absolute tracking orientation; a sampled world basis is not a mount.
inline Mat NativeView(float yaw,float pitch){
    const float cy=std::cos(yaw),sy=std::sin(yaw),cp=std::cos(pitch),sp=std::sin(pitch);
    return {{{cp*cy,cp*sy,sp,0},{-sy,cy,0,0},{-sp*cy,-sp*sy,cp,0},{0,0,0,1}}};
}
inline bool Desired(unsigned side,unsigned vertices,const gloves::Pose& grip,
                    V centre,float centreYaw,float baseYaw,float units,float size,
                    const Mat& component,const Mat& nativeView,Mat& out){
    const Mat* reference=side==0?&dualhandreference::Left:dualhandreference::Right(vertices);
    if(side>1||!reference||!armik::Finite(centre)||!gloves::Finite(grip.position)||
       !std::isfinite(size)||size<.025f||size>16||!std::isfinite(units)||units<10||units>1000||
       !std::isfinite(centreYaw)||!std::isfinite(baseYaw))return false;
    gloves::Q q;if(!gloves::Normalize(grip.orientation,q))return false;
    if(!armbind::Rigid(nativeView))return false;
    float localScale=0;if(!gloves::UniformScale(component.m,localScale))return false;
    Mat local=component;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)local.m[i][j]/=localScale;
    for(unsigned j=0;j<3;++j)local.m[3][j]=0;
    if(!armbind::Rigid(local))return false;
    Mat inverseView,mount,neutral;
    if(!skeletal::Inverse(inverseView.m,nativeView.m))return false;
    skeletal::Multiply(mount.m,local.m,inverseView.m);
    skeletal::Multiply(neutral.m,reference->m,mount.m);
    const auto vector=[&](V native){
        // Native neutral-view axes: +X forward, +Y right, +Z up.
        const auto xr=gloves::Rotate(q,{native.y,native.z,-native.x});
        // Geometry is already UU; tracking conversion must not scale it again.
        const auto world=vrpose::TrackingDeltaToWorldUU({xr.x,xr.y,xr.z},centreYaw,baseYaw,size);
        return V{world.x,world.y,world.z};
    };
    Mat desired;desired.m[3][3]=1;
    // Preserve the measured native grip attachment, including each weapon's wrist.
    for(unsigned r=0;r<3;++r){
        const auto v=vector({neutral.m[r][0],neutral.m[r][1],neutral.m[r][2]});
        desired.m[r][0]=v.x;desired.m[r][1]=v.y;desired.m[r][2]=v.z;
    }
    const auto p=vrpose::TrackingDeltaToWorldUU({grip.position.x-centre.x,
        grip.position.y-centre.y,grip.position.z-centre.z},centreYaw,baseYaw,units);
    desired.m[3][0]=p.x;desired.m[3][1]=p.y;desired.m[3][2]=p.z;
    if(!armbind::Finite(desired))return false;
    out=desired;return true;
}
inline bool Target(const Mat& sourceWorld,const Mat& desired,float out[4][4]){
    Mat inverse,result;
    if(!armbind::Finite(sourceWorld)||!armbind::Finite(desired)||
       !skeletal::Inverse(inverse.m,sourceWorld.m))return false;
    skeletal::Multiply(result.m,inverse.m,desired.m);
    if(!armbind::Finite(result))return false;
    std::memcpy(out,result.m,sizeof(result));return true;
}
}
