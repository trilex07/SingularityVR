#pragma once
#include "arm_bind.h"
#include "dual_hand_reference.h"
#include <cstdint>

// Render-thread source witness, never a guessed controller-space replacement.
// All children retain their current animation relative to the original wrist.
namespace dualaction {
using armbind::Mat;
inline Mat Product(const Mat& a,const Mat& b){Mat r;skeletal::Multiply(r.m,a.m,b.m);return r;}
inline bool Affine(const Mat& a){return armbind::Finite(a)&&
    std::fabs(a.m[0][3])+std::fabs(a.m[1][3])+std::fabs(a.m[2][3])+std::fabs(a.m[3][3]-1)<1e-5f;}
struct Key {
    uint32_t frame=0,pawn=0,weapon=0,vertices=0;
    bool operator==(const Key& b)const{return frame==b.frame&&pawn==b.pawn&&weapon==b.weapon&&vertices==b.vertices;}
};
struct Context {
    bool enabled=false,dual=false,tracked=false,gameplay=false,nativeAnimation=false,
         holstered=false,support=false,carrying=false,rightHanded=false;
};
inline bool Eligible(const Context& c){return c.enabled&&c.dual&&c.tracked&&c.gameplay&&
    !c.nativeAnimation&&!c.holstered&&!c.support&&!c.carrying&&c.rightHanded;}
// A shared parent may carry the native component's uniform scale. Retain it;
// singular/nonuniform/sheared poses are not eligible for this rigid correction.
inline bool ReferenceWithScale(const Mat& current,const Mat& reference,Mat& out){
    if(!Affine(current)||!armbind::Rigid(reference))return false;
    float scale=0;
    for(unsigned j=0;j<3;++j)scale+=current.m[0][j]*current.m[0][j];
    scale=std::sqrt(scale);
    if(!(scale>.1f&&scale<4.f))return false;
    Mat unit=current;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)unit.m[i][j]/=scale;
    if(!armbind::Rigid(unit))return false;
    Mat result=reference;
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<3;++j)result.m[i][j]*=scale;
    out=result;return true;
}
inline bool Correction(const Mat& bind,const float skin[3][4],const Mat& local,
                       const Mat& reference,Mat& out){
    Mat current,target,inverse;
    if(!Affine(local)||!armbind::CurrentJoint(bind,skin,current)||
       !ReferenceWithScale(current,reference,target))return false;
    const Mat source=Product(current,local);
    if(!skeletal::Inverse(inverse.m,source.m))return false;
    const Mat result=Product(inverse,Product(target,local));
    if(!Affine(result))return false;
    out=result;return true;
}
struct Frame {
    Key key{};bool valid=false;Mat local{},left{},right{};
    void Clear(){valid=false;}
    bool Capture(Key k,const Mat& sourceLocal,const Mat& leftBind,const float leftSkin[3][4],
                 const Mat& rightBind,const float rightSkin[3][4]){
        valid=false;
        const Mat* reference=dualhandreference::Right(k.vertices);
        Mat l,r;
        if(!k.pawn||!k.weapon||!reference||
           !Correction(leftBind,leftSkin,sourceLocal,dualhandreference::Left,l)||
           !Correction(rightBind,rightSkin,sourceLocal,*reference,r))return false;
        key=k;local=sourceLocal;left=l;right=r;valid=true;return true;
    }
    bool Current(Key k)const{return valid&&key==k;}
    bool SameLocal(const Mat& other)const{
        if(!Affine(other))return false;
        for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)
            if(std::fabs(local.m[i][j]-other.m[i][j])>.0001f)return false;
        return true;
    }
    bool Apply(Key k,bool aimHand,float controller[4][4])const{
        if(!Current(k))return false;
        Mat c;std::memcpy(c.m,controller,sizeof(c));
        if(!Affine(c))return false;
        const Mat result=Product(aimHand?right:left,c);
        if(!Affine(result))return false;
        std::memcpy(controller,result.m,sizeof(result));return true;
    }
};
}
