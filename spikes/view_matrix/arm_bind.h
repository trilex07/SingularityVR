#pragma once
#include "arm_ik.h"
#include "skeletal_palette.h"
#include <cstring>

namespace armbind {
struct Mat { float m[4][4]{}; };
struct ReferenceBone { int parent=0; float pskQuaternion[4]{}; armik::V pskPosition; };
inline bool Finite(const Mat& a){for(auto& row:a.m)for(float f:row)if(!std::isfinite(f))return false;return true;}
inline armik::V Position(const Mat& a){return {a.m[3][0],a.m[3][1],a.m[3][2]};}
// Exact UEViewer PSK->internal reversal, followed by its root conjugation.
// UE3 unit-scale reference skeleton only; exporter sources are research evidence.
inline bool PskLocal(const ReferenceBone& b,bool root,Mat& out){
    float x=b.pskQuaternion[0],y=-b.pskQuaternion[1],z=b.pskQuaternion[2],w=-b.pskQuaternion[3];
    double n=double(x)*x+double(y)*y+double(z)*z+double(w)*w;if(!std::isfinite(n)||n<1e-12||!armik::Finite(b.pskPosition))return false;
    float inv=(float)(1/std::sqrt(n));x*=inv;y*=inv;z*=inv;w*=inv;
    if(root){x=-x;y=-y;z=-z;}
    Mat a;a.m[0][0]=1-2*(y*y+z*z);a.m[0][1]=2*(x*y-w*z);a.m[0][2]=2*(x*z+w*y);
    a.m[1][0]=2*(x*y+w*z);a.m[1][1]=1-2*(x*x+z*z);a.m[1][2]=2*(y*z-w*x);
    a.m[2][0]=2*(x*z-w*y);a.m[2][1]=2*(y*z+w*x);a.m[2][2]=1-2*(x*x+y*y);
    a.m[3][0]=b.pskPosition.x;a.m[3][1]=-b.pskPosition.y;a.m[3][2]=b.pskPosition.z;a.m[3][3]=1;
    if(!Finite(a))return false;out=a;return true;
}
inline bool BuildReference(const ReferenceBone* bones,unsigned count,Mat* absolute){
    if(!bones||!absolute||!count||count>256)return false;
    for(unsigned i=0;i<count;++i){Mat local;if(!PskLocal(bones[i],i==0,local))return false;if(i==0)absolute[i]=local;else{if(bones[i].parent<0||(unsigned)bones[i].parent>=i)return false;skeletal::Multiply(absolute[i].m,local.m,absolute[bones[i].parent].m);if(!Finite(absolute[i]))return false;}}return true;
}
inline bool FromGpu(const float gpu[3][4],Mat& out){Mat a;for(int r=0;r<3;++r)for(int c=0;c<4;++c)a.m[c][r]=gpu[r][c];a.m[3][3]=1;if(!Finite(a))return false;out=a;return true;}
inline void ToGpu(const Mat& a,float gpu[3][4]){for(int r=0;r<3;++r)for(int c=0;c<4;++c)gpu[r][c]=a.m[c][r];}
inline bool CurrentJoint(const Mat& bind,const float skin[3][4],Mat& out){Mat s;if(!FromGpu(skin,s)||!Finite(bind))return false;Mat a;skeletal::Multiply(a.m,bind.m,s.m);if(!Finite(a))return false;out=a;return true;}
inline bool Rigid(const Mat& a){if(!Finite(a))return false;for(int r=0;r<3;++r)for(int c=0;c<3;++c){float dot=0;for(int k=0;k<3;++k)dot+=a.m[r][k]*a.m[c][k];if(std::fabs(dot-(r==c?1.f:0.f))>.005f)return false;}const float determinant=a.m[0][0]*(a.m[1][1]*a.m[2][2]-a.m[1][2]*a.m[2][1])-a.m[0][1]*(a.m[1][0]*a.m[2][2]-a.m[1][2]*a.m[2][0])+a.m[0][2]*(a.m[1][0]*a.m[2][1]-a.m[1][1]*a.m[2][0]);return determinant>.995f&&std::fabs(a.m[0][3])+std::fabs(a.m[1][3])+std::fabs(a.m[2][3])+std::fabs(a.m[3][3]-1)<1e-5f;}
struct Deltas { Mat upper,fore,wrist; armik::Solution solution; };
inline bool Delta(const Mat& before,const Mat& after,Mat& out){Mat inv;if(!skeletal::Inverse(inv.m,before.m))return false;skeletal::Multiply(out.m,inv.m,after.m);return Finite(out);}
// Called only on an intentional calibration action with tracked controller and
// verified weapon pose. Returns a weapon-local point; never auto-enables locking.
inline bool CalibrateSupportPoint(const Mat& weaponPose,armik::V controller,armik::V& localPoint){
    if(!Rigid(weaponPose)||!armik::Finite(controller))return false;
    Mat inverse;if(!skeletal::Inverse(inverse.m,weaponPose.m))return false;
    armik::V p{controller.x*inverse.m[0][0]+controller.y*inverse.m[1][0]+controller.z*inverse.m[2][0]+inverse.m[3][0],controller.x*inverse.m[0][1]+controller.y*inverse.m[1][1]+controller.z*inverse.m[2][1]+inverse.m[3][1],controller.x*inverse.m[0][2]+controller.y*inverse.m[1][2]+controller.z*inverse.m[2][2]+inverse.m[3][2]};
    if(!armik::Finite(p))return false;localPoint=p;return true;
}
// All joint inputs must be recovered from ORIGINAL constants, before ride/eye transforms.
// Compute ONCE per frame; reuse this result for every section, colour/depth and eye.
inline bool BuildArmDeltas(const Mat& shoulderBind,const Mat& elbowBind,const Mat& wristBind,
        const float shoulderSkin[3][4],const float elbowSkin[3][4],const float wristSkin[3][4],
        const Mat& controllerWrist,armik::V preferredBend,armik::State& state,Deltas& out){
    Mat shoulder,elbow,wrist;if(!CurrentJoint(shoulderBind,shoulderSkin,shoulder)||!CurrentJoint(elbowBind,elbowSkin,elbow)||!CurrentJoint(wristBind,wristSkin,wrist)||!Rigid(shoulder)||!Rigid(elbow)||!Rigid(wrist)||!Rigid(controllerWrist))return false;
    using namespace armik;V a=Position(shoulder),b=Position(elbow),c=Position(wrist);
    float upper=Length(Position(elbowBind)-Position(shoulderBind)),fore=Length(Position(wristBind)-Position(elbowBind));
    if(std::fabs(Length(b-a)-upper)>std::max(.01f,upper*.02f)||std::fabs(Length(c-b)-fore)>std::max(.01f,fore*.02f))return false;
    State next=state;Deltas result;if(!Solve(a,Position(controllerWrist),upper,fore,preferredBend,next,result.solution))return false;
    V line,oldBend;if(!Unit(c-a,line))line=next.direction;
    if(!Unit((b-a)-line*Dot(b-a,line),oldBend))oldBend=next.bend;
    Mat oldUpper,newUpper,oldFore,newFore,targetWrist=controllerWrist;
    if(!Frame(a,b-a,oldBend,oldUpper.m)||!Frame(result.solution.shoulder,result.solution.elbow-result.solution.shoulder,next.bend,newUpper.m)||!Frame(b,c-b,oldBend,oldFore.m)||!Frame(result.solution.elbow,result.solution.wrist-result.solution.elbow,next.bend,newFore.m))return false;
    targetWrist.m[3][0]=result.solution.wrist.x;targetWrist.m[3][1]=result.solution.wrist.y;targetWrist.m[3][2]=result.solution.wrist.z;
    if(!Delta(oldUpper,newUpper,result.upper)||!Delta(oldFore,newFore,result.fore)||!Delta(wrist,targetWrist,result.wrist))return false;
    out=result;state=next;return true;
}
enum class Region { Keep, Upper, Fore, Wrist };
inline bool Descends(unsigned bone,unsigned ancestor,const ReferenceBone* refs,unsigned count){if(bone>=count||ancestor>=count)return false;for(unsigned n=0;n<count;++n){if(bone==ancestor)return true;if(bone==0||refs[bone].parent<0||(unsigned)refs[bone].parent>=bone)return false;bone=(unsigned)refs[bone].parent;}return false;}
inline Region Role(unsigned bone,unsigned shoulder,unsigned elbow,unsigned wrist,const ReferenceBone* refs,unsigned count){if(Descends(bone,wrist,refs,count))return Region::Wrist;if(Descends(bone,elbow,refs,count))return Region::Fore;if(Descends(bone,shoulder,refs,count))return Region::Upper;return Region::Keep;}
// Works on a verified section's skinning palette. Transactional: failures leave
// output untouched. Original palette must be restored after native draw sequence.
inline bool ApplyArmChain(float (*out)[3][4],const float (*original)[3][4],const unsigned* sectionBoneMap,unsigned slots,
        const ReferenceBone* refs,unsigned bones,unsigned shoulder,unsigned elbow,unsigned wrist,const Deltas& d){
    if(!out||!original||!sectionBoneMap||!refs||slots>75||!bones||bones>256||shoulder>=bones||elbow>=bones||wrist>=bones||!Finite(d.upper)||!Finite(d.fore)||!Finite(d.wrist))return false;
    float staged[75][3][4];for(unsigned i=0;i<slots;++i){if(sectionBoneMap[i]>=bones)return false;Mat s;if(!FromGpu(original[i],s))return false;const Region region=Role(sectionBoneMap[i],shoulder,elbow,wrist,refs,bones);const Mat* delta=region==Region::Upper?&d.upper:region==Region::Fore?&d.fore:region==Region::Wrist?&d.wrist:nullptr;if(delta)skeletal::TransformBone(staged[i],original[i],delta->m);else std::memcpy(staged[i],original[i],sizeof(staged[i]));if(!FromGpu(staged[i],s))return false;}
    std::memcpy(out,staged,slots*sizeof(staged[0]));return true;
}
}
