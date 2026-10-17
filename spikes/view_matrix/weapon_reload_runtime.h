#pragma once
#include "weapon_reload_animation.h"
#include "weapon_reload_prototypes.h"
#include "pose_conversion.h"
#include <cstdint>

// Shared CPU contract used by the real input/native/render dispatchers.
namespace weaponreload {
using Kind=weaponreloadproto::Kind;
using Phase=weaponreloadproto::Phase;
inline unsigned Index(Kind k){return unsigned(k)-1;}
inline unsigned ClipSlot(Kind k){return k==Kind::Valkyrie?3:k==Kind::Volk?0:k==Kind::Kasimov?5:99;}
inline const char* ClassName(Kind k){return k==Kind::Valkyrie?"RvWeaponShared":
    k==Kind::Volk?"RvWeaponShared_Shotgun":k==Kind::Kasimov?"RvWeaponShared_SniperRifle":"";}
inline const char* Stem(Kind k){return k==Kind::Valkyrie?"valkyrie":k==Kind::Volk?"volk":"kasimov";}
inline bool Active(Phase p){return p==Phase::NeedAmmo||p==Phase::Carrying||p==Phase::WaitingNative;}
inline bool Absent(Phase p){return p==Phase::NeedAmmo||p==Phase::Carrying;}
enum class Part {None,Body,Attachment};
inline Part DrawPart(Kind kind,int base,unsigned min,unsigned vertices,unsigned first,unsigned triangles){
    const auto* spec=weaponreloadanim::Find(kind);if(!spec||base)return Part::None;
    const unsigned mainVertices=kind==Kind::Valkyrie?11378:kind==Kind::Kasimov?11292:8297;
    if(!min&&!first&&triangles==spec->triangles&&(vertices==spec->vertices||vertices==mainVertices))return Part::Body;
    const unsigned extra=kind==Kind::Valkyrie?8:kind==Kind::Kasimov?168:0;
    if(extra&&first==spec->triangles*3&&triangles==extra&&(min==0||min==mainVertices)&&
       (vertices==spec->vertices||vertices==spec->vertices-mainVertices))return Part::Attachment;
    return Part::None;
}
inline unsigned Hash(const unsigned char* p,unsigned n){unsigned h=2166136261u;
    for(unsigned i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
struct OriginalAssets {unsigned animation,mesh,albedo,vertices,triangles,width,height;};
inline constexpr OriginalAssets originalAssets[]{
    {1866531932u,1387338662u,2571601141u,547,596,2048,1024},
    {2782548863u,789788386u,2003765126u,242,198,1024,1024},
    {2750451985u,3422279735u,543706490u,170,180,1024,512}
}; // full original cache FNV witnesses; release/deployment uses SHA-256 manifest
inline unsigned ReadableUsage(bool enabled,unsigned bytes,bool index16,unsigned usage,unsigned writeOnly){
    if(enabled&&index16)for(const auto& s:weaponreloadanim::specs)
        if(bytes==s.indexCount*2)return usage&~writeOnly;
    return usage;
}
inline hipknife::V Transform(hipknife::V p,const armbind::Mat& m){return {
    p.x*m.m[0][0]+p.y*m.m[1][0]+p.z*m.m[2][0]+m.m[3][0],
    p.x*m.m[0][1]+p.y*m.m[1][1]+p.z*m.m[2][1]+m.m[3][1],
    p.x*m.m[0][2]+p.y*m.m[1][2]+p.z*m.m[2][2]+m.m[3][2]};}
inline bool PhysicalMatrix(const armbind::Mat& m){
    if(!armbind::Finite(m)||m.m[0][3]||m.m[1][3]||m.m[2][3]||m.m[3][3]!=1)return false;
    for(unsigned r=0;r<3;++r){const float n=hipknife::Length({m.m[r][0],m.m[r][1],m.m[r][2]});
        if(n<.05f||n>16.f)return false;} // includes existing WorldScale/GunScale tuning extremes
    const float determinant=m.m[0][0]*(m.m[1][1]*m.m[2][2]-m.m[1][2]*m.m[2][1])-
        m.m[0][1]*(m.m[1][0]*m.m[2][2]-m.m[1][2]*m.m[2][0])+
        m.m[0][2]*(m.m[1][0]*m.m[2][1]-m.m[1][1]*m.m[2][0]);
    return std::fabs(determinant)>.0001f; // handedness may be negative; never collapse
}
inline bool ClipBind(const weaponreloadanim::Data& d,armbind::Mat& bind){
    for(unsigned i=0;d.ready&&i<d.tracks;++i)if(d.target[i]==ClipSlot(d.kind))
        return skeletal::Inverse(bind.m,d.inverse[i].m)&&armbind::Rigid(bind);
    return false;
}
inline bool ParentPalette(const weaponreloadanim::Data& d,const float* native,unsigned regs,
        const armbind::Mat* held,float* out){
    if(!d.ready||!native||!out||regs!=d.slots*3||d.slots>16||d.root>=d.slots||
       (held&&!armbind::Finite(*held)))return false;
    float staged[16*12];std::memcpy(staged,native,regs*4*sizeof(float));
    if(held)armbind::ToGpu(*held,reinterpret_cast<float(*)[4]>(staged+d.root*12));
    std::memcpy(out,staged,regs*4*sizeof(float));return true;
}
// Actual native seated joint, not a guessed controller-local socket. The root
// skin remains native. C is exactly BuildGunC; local is measured LocalToWorld.
inline bool Socket(const weaponreloadanim::Data& d,const float* palette,
        const armbind::Mat& local,const armbind::Mat& controller,
        hipknife::V centre,float centreYaw,float baseYaw,float units,hipknife::V& out,
        armbind::Mat* clipTracking=nullptr){
    if(!palette||!armbind::Finite(local)||!armbind::Finite(controller)||
       !hipknife::Finite(centre)||!std::isfinite(centreYaw)||!std::isfinite(baseYaw)||
       !std::isfinite(units)||units<=0)return false;
    armbind::Mat bind,root,a,b,c;
    if(!ClipBind(d,bind)||!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(palette+d.root*12),root))return false;
    skeletal::Multiply(a.m,bind.m,root.m);skeletal::Multiply(b.m,a.m,local.m);
    skeletal::Multiply(c.m,b.m,controller.m);if(!armbind::Finite(c))return false;
    // Algebraic inverse of vrpose::TrackingDeltaToWorldUU. The translated-world
    // origin corresponds to XR centre, NOT head or actor camera position.
    const float x=c.m[3][0]/units,y=c.m[3][1]/units,z=c.m[3][2]/units;
    const float f=x*std::cos(baseYaw)+y*std::sin(baseYaw);
    const float r=-x*std::sin(baseYaw)+y*std::cos(baseYaw);
    const hipknife::V p{centre.x-std::sin(centreYaw)*f+std::cos(centreYaw)*r,
        centre.y+z,centre.z-std::cos(centreYaw)*f-std::sin(centreYaw)*r};
    if(!hipknife::Finite(p))return false;
    if(clipTracking){
        armbind::Mat mapped;
        for(unsigned axis=0;axis<3;++axis){
            // SVRK uses original clip-local UU / 52.5. Preserve the actual
            // native mesh/world scale AND handedness, not an arbitrary scale1.
            const float ax=c.m[axis][0]*52.5f/units,ay=c.m[axis][1]*52.5f/units;
            const float af=ax*std::cos(baseYaw)+ay*std::sin(baseYaw);
            const float ar=-ax*std::sin(baseYaw)+ay*std::cos(baseYaw);
            mapped.m[axis][0]=-std::sin(centreYaw)*af+std::cos(centreYaw)*ar;
            mapped.m[axis][1]=c.m[axis][2]*52.5f/units;
            mapped.m[axis][2]=-std::cos(centreYaw)*af-std::sin(centreYaw)*ar;
        }
        mapped.m[3][0]=p.x;mapped.m[3][1]=p.y;mapped.m[3][2]=p.z;mapped.m[3][3]=1;
        if(!PhysicalMatrix(mapped))return false;*clipTracking=mapped;
    }
    out=p;return true;
}
inline bool HandMatrix(const armbind::Mat& native,const hipknife::Pose& aim,
        const hipknife::Pose& hand,hipknife::V centre,armbind::Mat& out,hipknife::V offsetLocal={0,0,-.06f}){
    if(!PhysicalMatrix(native)||!hipknife::Valid(aim.q)||!hipknife::Valid(hand.q)||
       !hipknife::Finite(hand.p)||!hipknife::Finite(centre)||!hipknife::Finite(offsetLocal)||hipknife::Length(offsetLocal)>.3f)return false;
    armbind::Mat result;const hipknife::Q inverse{-aim.q.x,-aim.q.y,-aim.q.z,aim.q.w};
    for(unsigned i=0;i<3;++i){
        const auto axis=hipknife::Rotate(hand.q,hipknife::Rotate(inverse,{native.m[i][0],native.m[i][1],native.m[i][2]}));
        result.m[i][0]=axis.x;result.m[i][1]=axis.y;result.m[i][2]=axis.z;
    }
    result.m[3][3]=1;
    const auto offset=Transform(centre,result);
    const auto target=hand.p+hipknife::Rotate(hand.q,offsetLocal);
    result.m[3][0]=target.x-offset.x;result.m[3][1]=target.y-offset.y;result.m[3][2]=target.z-offset.z;
    if(!PhysicalMatrix(result))return false;out=result;return true;
}
// Native material replay: inverse of the already-tested SVRK -> tracking map.
// No Centurion timeline/cache dependency; each original clip has its own bind.
inline bool TrackingMap(hipknife::V centre,float yaw,float base,float units,armbind::Mat& out){
    if(!hipknife::Finite(centre)||!std::isfinite(yaw)||!std::isfinite(base)||
       !std::isfinite(units)||units<10||units>1000)return false;
    armbind::Mat m;m.m[3][3]=1;
    for(unsigned r=0;r<4;++r){
        vrpose::V3 v{};if(r==0)v.x=1;else if(r==1)v.y=1;else if(r==2)v.z=1;
        else v={-centre.x,-centre.y,-centre.z};
        const auto p=vrpose::TrackingDeltaToWorldUU(v,yaw,base,units);
        m.m[r][0]=p.x;m.m[r][1]=p.y;m.m[r][2]=p.z;
    }
    if(!armbind::Finite(m))return false;out=m;return true;
}
inline bool SparePalette(const weaponreloadanim::Data& d,const armbind::Mat& tracking,
        const armbind::Mat& local,const armbind::Mat& controller,const armbind::Mat& trackingMap,
        unsigned regs,float* out){
    const auto* spec=weaponreloadanim::Find(d.kind);
    if(!out||!d.ready||!spec||d.slots!=spec->slots||ClipSlot(d.kind)>=d.slots||
       d.tracks>8||regs!=d.slots*3||!PhysicalMatrix(tracking)||
       !armbind::Finite(local)||!armbind::Finite(controller)||!armbind::Finite(trackingMap))return false;
    armbind::Mat bind,inverseBind,gun,inverseGun,inverseWorld,metres{},a,b,c,skin;
    if(trackingMap.m[0][3]||trackingMap.m[1][3]||trackingMap.m[2][3]||trackingMap.m[3][3]!=1||
       !skeletal::Inverse(inverseWorld.m,trackingMap.m))return false;
    if(!ClipBind(d,bind)||!skeletal::Inverse(inverseBind.m,bind.m))return false;
    skeletal::Multiply(gun.m,local.m,controller.m);
    if(!skeletal::Inverse(inverseGun.m,gun.m)||!armbind::Finite(inverseWorld))return false;
    metres.m[0][0]=metres.m[1][1]=metres.m[2][2]=1.f/52.5f;metres.m[3][3]=1;
    skeletal::Multiply(a.m,inverseBind.m,metres.m);skeletal::Multiply(b.m,a.m,tracking.m);
    skeletal::Multiply(c.m,b.m,trackingMap.m);skeletal::Multiply(skin.m,c.m,inverseGun.m);
    if(!armbind::Finite(skin))return false;
    float staged[16*12]{};
    for(unsigned slot=0;slot<d.slots;++slot)for(unsigned r=0;r<3;++r)
        staged[slot*12+r*4+3]=skin.m[3][r]; // SAME degenerate point for all other bones
    armbind::ToGpu(skin,reinterpret_cast<float(*)[4]>(staged+ClipSlot(d.kind)*12));
    std::memcpy(out,staged,regs*4*sizeof(float));return true;
}
// Called INSIDE each existing stereo-eye callback with the original DIP/setter.
// No material/texture/RT/depth/IB changes. Restore even after partial upload.
template<class Upload,class Draw>bool Replay(Upload upload,Draw draw,const float* spare,
        const float* attached,bool& restored){
    const bool changed=upload(spare),drawn=changed&&draw();
    restored=upload(attached);return drawn&&restored;
}
struct ReplayPlan {
    unsigned count=0,index=0,regs=0;
    float attached[16*12],spare[2][16*12]; // initialized only for eligible native draws
};
inline bool Palette(const weaponreloadanim::Data& d,float frame,bool absent,const float* native,unsigned regs,float* out){
    float staged[16*12];if(!weaponreloadanim::Palette(d,true,frame,native,regs,staged))return false;
    const unsigned clip=ClipSlot(d.kind);if(clip>=d.slots)return false;
    // Measured original clips are rigid, no mixed-boundary triangles. Zero
    // linear part collapses ONLY those triangles; finite translation retained.
    if(absent)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)staged[clip*12+r*4+c]=0;
    std::memcpy(out,staged,regs*4*sizeof(float));return true;
}
struct Animation {
    static constexpr std::uint64_t RefillSeatMs=180;
    float outFrame=0,inFrame=0;
    bool Measure(const weaponreloadanim::Data& d){
        armbind::Mat bind;if(!ClipBind(d,bind))return false;
        const hipknife::V origin=Transform({},bind);float distance[256]{},maximum=0;
        unsigned peak=0;
        for(unsigned f=0;f<d.frames;++f){armbind::Mat delta[8];if(!weaponreloadanim::Sample(d,float(f),delta))return false;
            for(unsigned i=0;i<d.tracks;++i)if(d.target[i]==ClipSlot(d.kind)){
                distance[f]=hipknife::Length(Transform(origin,delta[i])-origin);
                if(distance[f]>maximum){maximum=distance[f];peak=f;}
            }}
        if(maximum<.01f||peak==0||peak>=d.frames-1)return false;
        unsigned returned=d.frames-1;
        for(unsigned f=peak+1;f<d.frames;++f)if(distance[f]<=maximum*.05f){returned=f;break;}
        outFrame=float(peak);inFrame=float(returned);return true;
    }
    float Frame(const weaponreloadanim::Data& d,Phase phase,std::uint64_t since,std::uint64_t now)const{
        const float elapsed=now>=since?float(now-since)*.001f*d.fps:0;
        if(phase==Phase::NeedAmmo)return std::min(outFrame,elapsed);
        if(phase==Phase::Carrying)return outFrame;
        if(phase==Phase::WaitingNative)return std::min(float(d.frames-1),inFrame+elapsed);
        return 0;
    }
    float RefillFrame(const weaponreloadanim::Data& d,Phase phase,std::uint64_t since,
                      std::uint64_t inserted,std::uint64_t now)const{
        if(!inserted||now<inserted||(Active(phase)&&phase!=Phase::WaitingNative))
            return Frame(d,phase,since,now);
        const auto elapsed=now-inserted;
        if(elapsed>=RefillSeatMs)return 0; // no native tail; rest exactly frame0
        const float t=float(elapsed)/float(RefillSeatMs),smooth=t*t*(3.f-2.f*t);
        // The already-seated original return segment only; root stays native.
        // Rendering interpolation is NOT an engine animation/timer write.
        return inFrame+(float(d.frames-1)-inFrame)*smooth;
    }
};
struct Witness {
    bool valid=false;Kind kind=Kind::None;std::uint32_t pawn=0,weapon=0;
    std::uint64_t sampledMs=0;hipknife::V socket{};
    armbind::Mat clipTracking{};
    armbind::Mat nativeRoot{};
    bool Current(Kind k,std::uint32_t p,std::uint32_t w,std::uint64_t now)const{
        return valid&&kind==k&&p&&w&&p==pawn&&w==weapon&&now>=sampledMs&&
            now-sampledMs<=150&&hipknife::Finite(socket);
    }
};
} // namespace weaponreload
