#pragma once
// Pure CPU contract shared by the opt-in D3D9 renderer and L1 tests.
// SVRG is an offline-baked GLB morph, not an arbitrary executable asset.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
#include <utility>

namespace gloves {
struct V { float x=0,y=0,z=0; };
struct Q { float x=0,y=0,z=0,w=1; };
struct Pose { V position; Q orientation; };
inline V operator+(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V operator-(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V operator*(V a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline bool Finite(V a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline bool Normalize(Q in,Q& out){
    const double n=double(in.x)*in.x+double(in.y)*in.y+double(in.z)*in.z+double(in.w)*in.w;
    if(!std::isfinite(n)||n<1e-12)return false;
    const float s=float(1./std::sqrt(n));out={in.x*s,in.y*s,in.z*s,in.w*s};return true;
}
inline V Rotate(Q q,V v){const V t=Cross({q.x,q.y,q.z},v)*2.f;return v+t*q.w+Cross({q.x,q.y,q.z},t);}
inline float Blend(float analog,bool active,float deadzone=.08f){
    if(!active||!std::isfinite(analog)||!std::isfinite(deadzone)||deadzone<0||deadzone>=1)return 0;
    return std::max(0.f,std::min(1.f,(analog-deadzone)/(1.f-deadzone)));
}
template<class Binding,class Action>
inline unsigned WithoutOptional(Binding* bindings,unsigned count,Action pose,Action squeeze){
    unsigned kept=0;
    for(unsigned i=0;i<count;++i)
        if(bindings[i].action!=pose&&bindings[i].action!=squeeze)bindings[kept++]=bindings[i];
    return kept;
}
struct SourceVertex { V open,delta,normal,normalDelta; float u=0,v=0; };
static_assert(sizeof(SourceVertex)==56,"SVRG vertex layout");
struct Vertex { V p; std::uint32_t color=0xFFFFFFFF; float u=0,v=0; };
static_assert(sizeof(Vertex)==24,"D3D XYZ/DIFFUSE/TEX1");
struct Mesh { std::vector<SourceVertex> source; std::vector<std::uint32_t> indices; std::vector<Vertex> scratch;
    float bakedUnitsPerMetre=52.5f; };
inline std::uint32_t U32(const unsigned char* p){std::uint32_t n;std::memcpy(&n,p,4);return n;}
inline bool Decode(const unsigned char* b,std::size_t n,Mesh& out){
    if(!b||n<24||std::memcmp(b,"SVRG",4)||U32(b+4)!=2||U32(b+20)!=0)return false;
    const auto nv=U32(b+8),ni=U32(b+12);
    float units;std::memcpy(&units,b+16,4);
    if(!std::isfinite(units)||units<10.f||units>1000.f)return false;
    if(nv<3||nv>16384||ni<3||ni>98304||ni%3)return false;
    if(n!=24+std::size_t(nv)*sizeof(SourceVertex)+std::size_t(ni)*4)return false;
    Mesh next;next.source.resize(nv);next.indices.resize(ni);next.scratch.resize(nv);
    next.bakedUnitsPerMetre=units;
    std::memcpy(next.source.data(),b+24,nv*sizeof(SourceVertex));
    std::memcpy(next.indices.data(),b+24+nv*sizeof(SourceVertex),ni*4);
    for(const auto& v:next.source){
        if(!Finite(v.open)||!Finite(v.delta)||!Finite(v.normal)||!Finite(v.normalDelta)||
           !std::isfinite(v.u)||!std::isfinite(v.v)||Dot(v.open,v.open)>4.f||Dot(v.delta,v.delta)>4.f||
           Dot(v.normal,v.normal)<.25f||Dot(v.normal,v.normal)>4.f||
           Dot(v.normalDelta,v.normalDelta)>16.f||std::fabs(v.u)>16||std::fabs(v.v)>16)return false;
    }
    for(auto i:next.indices)if(i>=nv)return false;
    out=std::move(next);return true;
}
// Measure, do not assume the component's scale. Uniform rotations/reflections
// are accepted; shear/nonuniform/degenerate state must keep the native draw.
inline bool UniformScale(const float m[4][4],float& out){
    V row[3];float n[3];
    for(unsigned r=0;r<3;++r){row[r]={m[r][0],m[r][1],m[r][2]};
        n[r]=std::sqrt(Dot(row[r],row[r]));if(!std::isfinite(n[r])||n[r]<.001f||n[r]>1000.f)return false;}
    const float mean=(n[0]+n[1]+n[2])/3;
    for(unsigned r=0;r<3;++r)if(std::fabs(n[r]/mean-1.f)>.005f)return false;
    for(unsigned r=0;r<3;++r)for(unsigned c=r+1;c<3;++c)
        if(std::fabs(Dot(row[r],row[c])/(n[r]*n[c]))>.005f)return false;
    out=mean;return true;
}
inline bool NativeSizeFactor(float bakedUnits,float worldUnitsPerMetre,float gunScalePct,
                             float localScale,float& factor){
    if(!std::isfinite(bakedUnits)||bakedUnits<10||bakedUnits>1000||
       !std::isfinite(worldUnitsPerMetre)||worldUnitsPerMetre<=0||
       !std::isfinite(gunScalePct)||gunScalePct<25||gunScalePct>400||
       !std::isfinite(localScale)||localScale<.001f||localScale>1000)return false;
    factor=bakedUnits/worldUnitsPerMetre*(gunScalePct*.01f)*localScale;
    return std::isfinite(factor)&&factor>0&&factor<=100;
}
// Wrist-local geometry inherits the SAME size knobs as the native gun rig.
// Position and IPD are not scaled: only mesh offsets from the tracked wrist.
inline bool Transform(Mesh& mesh,float blend,const Pose& hand,const Pose& eye,float sizeFactor=1.f){
    Q h,e;
    if(!std::isfinite(blend)||blend<0||blend>1||!Finite(hand.position)||!Finite(eye.position)||
       !std::isfinite(sizeFactor)||sizeFactor<=0||sizeFactor>100||
       !Normalize(hand.orientation,h)||!Normalize(eye.orientation,e)||mesh.scratch.size()!=mesh.source.size())return false;
    e={-e.x,-e.y,-e.z,e.w};
    for(std::size_t i=0;i<mesh.source.size();++i){
        const auto& s=mesh.source[i];auto& o=mesh.scratch[i];
        const V world=hand.position+Rotate(h,(s.open+s.delta*blend)*sizeFactor);
        const V p=Rotate(e,world-eye.position);
        // Preserve source albedo. The old normal-Y multiplier recoloured the
        // glove with a fake world-up light, unrelated to native UE3 lighting.
        o.p={p.x,p.y,-p.z};o.u=s.u;o.v=s.v;o.color=0xFFFFFFFFu;
    }
    return true;
}
inline bool Eligible(bool enabled,bool ready,bool gameplay,bool paused,bool menu,bool cinematic,
                     bool focused,bool stereo,bool eyes,bool trackedLeft,bool trackedRight,
                     bool heldWeapon,bool supportGrip){
    return enabled&&ready&&gameplay&&!paused&&!menu&&!cinematic&&focused&&stereo&&eyes&&
        trackedLeft&&trackedRight&&!heldWeapon&&!supportGrip;
}
// Exact measured noTMD section only; never world/shadow/weapon/TMD meshes.
inline bool ReplaceSection(bool eligible,unsigned verts,unsigned first,unsigned tris,
                           bool scene,bool shadow,int bones,int regs,int local,bool foreground){
    return eligible&&(verts==3314||verts==2924)&&first==0&&tris==4830&&scene&&!shadow&&
        bones==5&&regs==225&&local==230&&foreground;
}
inline bool Projection(float tanX,float tanY,float nearZ,float farZ,float out[4][4]){
    if(!std::isfinite(tanX)||!std::isfinite(tanY)||!std::isfinite(nearZ)||!std::isfinite(farZ)||
       tanX<=0||tanY<=0||nearZ<=0||farZ<=nearZ)return false;
    std::memset(out,0,16*sizeof(float));out[0][0]=1/tanX;out[1][1]=1/tanY;
    out[2][2]=farZ/(farZ-nearZ);out[2][3]=1;out[3][2]=-nearZ*farZ/(farZ-nearZ);return true;
}
} // namespace gloves
