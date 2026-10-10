#pragma once
#include <cmath>
#include <algorithm>

// Geometry only. Input/output share one coordinate space. No draw/state side effects.
namespace armik {
struct V { float x=0,y=0,z=0; };
inline V operator+(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V operator-(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V operator*(V a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline bool Finite(V v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline float Length(V v){return std::sqrt(Dot(v,v));}
inline bool Unit(V v,V& out){float l=Length(v);if(!std::isfinite(l)||l<1e-6f)return false;out=v*(1/l);return true;}
inline V Orthogonal(V d){V axis=std::fabs(d.x)<std::fabs(d.y)?V{1,0,0}:V{0,1,0};if(std::fabs(d.z)<std::min(std::fabs(d.x),std::fabs(d.y)))axis={0,0,1};V b;Unit(axis-d*Dot(axis,d),b);return b;}
struct State { bool valid=false; V direction{0,1,0},bend{0,0,-1}; };
struct Solution { V shoulder,elbow,wrist; bool clamped=false; float targetError=0; };
// The bend is parallel transported by the shortest direction rotation.
// preferredBend only seeds a new chain; animated noise cannot flip a live elbow.
// Reset State explicitly on tracking loss, teleport or a new pawn.
inline bool Solve(V shoulder,V target,float upper,float fore,V preferredBend,State& state,Solution& out){
    if(!Finite(shoulder)||!Finite(target)||!Finite(preferredBend)||!std::isfinite(upper)||!std::isfinite(fore)||upper<1e-4f||fore<1e-4f)return false;
    const float sum=upper+fore;if(!std::isfinite(sum))return false;
    V delta=target-shoulder,direction;
    const float requested=Length(delta);if(!std::isfinite(requested))return false;
    if(!Unit(delta,direction))direction=state.valid?state.direction:V{0,1,0};
    V bend;
    V seed=state.valid?state.bend:preferredBend;
    if(state.valid){
        const float cosine=std::max(-1.f,std::min(1.f,Dot(state.direction,direction)));
        if(cosine>-.9999f){V axis=Cross(state.direction,direction);seed=seed+Cross(axis,seed)+Cross(axis,Cross(axis,seed))*(1/(1+cosine));}
        // Antipodal direction: rotating around the prior bend preserves its side.
    }
    if(!Unit(seed-direction*Dot(seed,direction),bend))bend=Orthogonal(direction);
    if(state.valid&&Dot(bend,state.bend)<0)bend=bend*(-1);
    const float eps=std::max(1e-5f,sum*1e-5f);
    const float minimum=std::fabs(upper-fore)+eps,maximum=sum-eps;
    if(minimum>=maximum)return false;
    const float distance=std::max(minimum,std::min(maximum,requested));
    // Double intermediates avoid catastrophic cancellation near full extension.
    const double a=upper,b=fore,d=distance;
    const float along=(float)((a*a-b*b+d*d)/(2*d));
    const float height=(float)std::sqrt(std::max(0.0,a*a-double(along)*along));
    Solution result;result.shoulder=shoulder;result.wrist=shoulder+direction*distance;
    result.elbow=shoulder+direction*along+bend*height;
    result.clamped=distance!=requested;result.targetError=Length(result.wrist-target);
    if(!Finite(result.elbow)||!Finite(result.wrist)||!std::isfinite(result.targetError))return false;
    out=result;state.valid=true;state.direction=direction;state.bend=bend;return true;
}
// Robust row-vector rigid frame, usable for segment delta or wrist/controller pose.
// Never use palette translation as the joint position: matrices include inverse bind.
inline bool Frame(V origin,V forward,V bend,float out[4][4]){
    V y,z,x;if(!Finite(origin)||!Unit(forward,y))return false;
    if(!Unit(bend-y*Dot(bend,y),z))z=Orthogonal(y);
    if(!Unit(Cross(y,z),x))return false;z=Cross(x,y);
    const float m[4][4]={{x.x,x.y,x.z,0},{y.x,y.y,y.z,0},{z.x,z.y,z.z,0},{origin.x,origin.y,origin.z,1}};
    for(int i=0;i<4;++i)for(int j=0;j<4;++j)out[i][j]=m[i][j];return true;
}
// Apply a *common wrist delta* to wrist and finger palette entries, retaining
// their relative animated poses. Upper/forearm entries need distinct segment deltas.
struct SupportCalibration { bool enabled=false; V weaponLocalPoint; float acquireRadius=0.15f,releaseRadius=0.30f; };
inline bool SupportTarget(const SupportCalibration& c,const float weapon[4][4],V controller,bool wasLocked,V& out){
    if(!c.enabled||!Finite(c.weaponLocalPoint)||!Finite(controller)||!std::isfinite(c.acquireRadius)||!std::isfinite(c.releaseRadius)||c.acquireRadius<=0||c.releaseRadius<c.acquireRadius)return false;
    for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(!std::isfinite(weapon[i][j]))return false;
    V p=c.weaponLocalPoint;V world{p.x*weapon[0][0]+p.y*weapon[1][0]+p.z*weapon[2][0]+weapon[3][0],p.x*weapon[0][1]+p.y*weapon[1][1]+p.z*weapon[2][1]+weapon[3][1],p.x*weapon[0][2]+p.y*weapon[1][2]+p.z*weapon[2][2]+weapon[3][2]};
    if(!Finite(world)||Length(world-controller)>(wasLocked?c.releaseRadius:c.acquireRadius))return false;out=world;return true;
}
}
