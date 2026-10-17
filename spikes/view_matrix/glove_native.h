#pragma once
// Native UE3 VB/declaration/material/shaders stay bound. Only an audited
// noTMD index subset + original finger palette are changed, opt-in and scoped.
#include "glove_renderer.h"
#include "arm_bind.h"
#include "glove_isolate.h"
#include "singularity_bind_reference.h"
#include "knife_hand_grip.h"
#include "hand_thumb_pose.h"

namespace nativegloves {
inline bool Thumb(unsigned b){return thumbpose::Thumb(b);}
inline bool Finger(unsigned b){return (b>=10&&b<=25&&b!=13)||(b>=70&&b<=84);}
struct Data {std::vector<std::uint16_t> indices;gloves::Q curl[90]{};unsigned capTriangles=0;};
inline bool Decode(const unsigned char* p,std::size_t bytes,Data& out){
    if(!p||bytes<32||std::memcmp(p,"SVNI",4)||gloves::U32(p+4)!=1||
       gloves::U32(p+12)!=2924||gloves::U32(p+16)!=4830||
       gloves::U32(p+20)!=16512||gloves::U32(p+28)!=90)return false;
    const unsigned n=gloves::U32(p+8),caps=gloves::U32(p+24);
    if(n<6||n%3||n>18000||caps<2||caps>512||caps*3>=n||bytes!=32+std::size_t(n)*2+90*16)return false;
    Data next;next.indices.resize(n);next.capTriangles=caps;
    std::memcpy(next.indices.data(),p+32,n*2);
    for(unsigned i=0;i<n;i+=3){
        const auto a=next.indices[i],b=next.indices[i+1],c=next.indices[i+2];
        if(a>=2924||b>=2924||c>=2924||a==b||b==c||a==c)return false;
    }
    std::memcpy(next.curl,p+32+n*2,90*16);
    for(unsigned b=0;b<90;++b){
        const auto q=next.curl[b];gloves::Q unit;
        if(!gloves::Normalize(q,unit)||!std::isfinite(q.w)||q.w<0||
           std::fabs(q.x-unit.x)+std::fabs(q.y-unit.y)+std::fabs(q.z-unit.z)+std::fabs(q.w-unit.w)>.002f)return false;
        if(!Finger(b)&&(std::fabs(q.x)+std::fabs(q.y)+std::fabs(q.z)+std::fabs(q.w-1))>1e-5f)return false;
    }
    out=std::move(next);return true;
}

// Explicit index binding guard: no state block, target, shader, material,
// texture, vertex buffer, declaration or CPU hook-classification changes.
template<class Dev=IDirect3DDevice9,class Buffer=IDirect3DIndexBuffer9>
struct IndexGuard {
    Dev* dev;Buffer* saved=nullptr;bool valid=false;
    IndexGuard(Dev* d,Buffer* replacement,unsigned minimumBytes=16512*2):dev(d){
        if(!replacement||FAILED(dev->GetIndices(&saved))||!saved)return;
        D3DINDEXBUFFER_DESC desc{};
        if(FAILED(saved->GetDesc(&desc))||desc.Format!=D3DFMT_INDEX16||desc.Size<minimumBytes)return;
        valid=SUCCEEDED(dev->SetIndices(replacement));
    }
    HRESULT Restore(){
        if(!valid)return S_OK;
        valid=false;return dev->SetIndices(saved);
    }
    ~IndexGuard(){if(valid)Restore();gloves::Release(saved);}
};

class Renderer {
    Data data;bool cpu=false,failed=false;
    armbind::Mat bind[90]{},inverse[90]{},local[90]{};
    IDirect3DIndexBuffer9* ib=nullptr;
    thumbpose::Data thumbs{};bool thumbReady=false;
public:
    bool LoadThumbs(const char* path){
        thumbReady=false;std::vector<unsigned char> bytes;thumbpose::Data next;
        if(!gloves::Read(path,bytes,8192)||!thumbpose::Decode(bytes.data(),bytes.size(),next))return false;
        thumbs=next;thumbReady=true;return true;
    }
    unsigned draws=0;
    bool Load(const char* path){
        ReleaseGPU();cpu=false;thumbReady=false;Data next;std::vector<unsigned char> bytes;
        if(!gloves::Read(path,bytes,128*1024)||!Decode(bytes.data(),bytes.size(),next)||
           !armbind::BuildReference(bindassets::noTmd,90,bind))return false;
        for(unsigned b=0;b<90;++b)
            if(!skeletal::Inverse(inverse[b].m,bind[b].m)||
               !armbind::PskLocal(bindassets::noTmd[b],b==0,local[b]))return false;
        data=std::move(next);cpu=true;return true;
    }
    bool Ready()const{return cpu&&!failed;}
    const armbind::Mat& WristReference(unsigned side)const{return bind[side?51:9];}
    unsigned Triangles()const{return (unsigned)data.indices.size()/3;}
    unsigned Caps()const{return data.capTriangles;}
    void ReleaseGPU(){gloves::Release(ib);failed=false;}
    IDirect3DIndexBuffer9* Ensure(IDirect3DDevice9* dev){
        if(!Ready())return nullptr;
        if(ib)return ib;
        const UINT size=(UINT)data.indices.size()*2;void* destination=nullptr;
        if(FAILED(dev->CreateIndexBuffer(size,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr))||!ib){failed=true;return nullptr;}
        if(FAILED(ib->Lock(0,size,&destination,0))){ReleaseGPU();failed=true;return nullptr;}
        std::memcpy(destination,data.indices.data(),size);
        if(FAILED(ib->Unlock())){ReleaseGPU();failed=true;return nullptr;}
        return ib;
    }
    // Transactional, stack-only. Source is original bind rig, not guessed slots.
    // Wrist/controller scale and position are already in the existing palette.
    // The cuff uses the same wrist transform rather than an animated elbow.
    bool Pose(float palette[135][4],const float grip[2],const float (*digits)[5]=nullptr,int knifeSide=-1,unsigned fingerMask=3u,bool originalCurl=false)const{
        if(!Ready()||!palette||!grip)return false;
        if(knifeSide < -1||knifeSide>1)return false;
        for(unsigned side=0;side<2;++side)if(!std::isfinite(grip[side])||grip[side]<0||grip[side]>1)return false;
        if(digits)for(unsigned side=0;side<2;++side)for(float f:digits[side])if(!std::isfinite(f)||f<0||f>1)return false;
        armbind::Mat posed[90],wristSkin[2];int wristSlot[2]={-1,-1};
        for(unsigned s=0;s<gloveisolate::kNoTmdCookedPaletteCount;++s){
            const auto b=gloveisolate::kNoTmdCookedPalette[s];
            if(b==9)wristSlot[0]=(int)s;if(b==51)wristSlot[1]=(int)s;
        }
        for(unsigned side=0;side<2;++side)
            if(wristSlot[side]<0||!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(&palette[wristSlot[side]*3]),wristSkin[side]))return false;
        for(unsigned b=0;b<90;++b){
            auto l=local[b];
            if(Finger(b)){
                const int side=gloveisolate::NoTmdCookedHandSide(b);
                if(side<0)return false;
                const int digit=knifehand::Digit(b);if(digit<0)return false;
                float t=digits?digits[side][digit]:grip[side];auto q=data.curl[b];gloves::Q blend;
                if(thumbReady&&Thumb(b)&&!originalCurl){
                    const bool knife=side==knifeSide;
                    if(!thumbpose::Sample(thumbs,(unsigned)side,thumbpose::Segment(b),grip[side],knife,q))return false;
                    t=1.f;
                }
                // Shortest-path normalized quaternion blend; rigid at all t.
                if(!gloves::Normalize({q.x*t,q.y*t,q.z*t,1+(q.w-1)*t},blend))return false;
                armbind::Mat rotation{};rotation.m[3][3]=1;
                for(unsigned r=0;r<3;++r){gloves::V v{};if(r==0)v.x=1;if(r==1)v.y=1;if(r==2)v.z=1;
                    v=gloves::Rotate(blend,v);rotation.m[r][0]=v.x;rotation.m[r][1]=v.y;rotation.m[r][2]=v.z;}
                armbind::Mat rotated;skeletal::Multiply(rotated.m,l.m,rotation.m);
                // Rotation is local at this joint; never rotate its translation.
                for(unsigned c=0;c<3;++c)rotated.m[3][c]=l.m[3][c];l=rotated;
            }
            if(!b)posed[b]=l;
            else skeletal::Multiply(posed[b].m,l.m,posed[bindassets::noTmd[b].parent].m);
        }
        constexpr unsigned registers=gloveisolate::kNoTmdCookedPaletteCount*3;
        float staged[135][4];std::memcpy(staged,palette,registers*4*sizeof(float));
        for(unsigned s=0;s<gloveisolate::kNoTmdCookedPaletteCount;++s){
            const auto b=gloveisolate::kNoTmdCookedPalette[s];
            const int side=gloveisolate::NoTmdCookedHandSide(b);if(side<0)return false;
            if(Finger(b)&&!(fingerMask&(1u<<unsigned(side))))continue;
            armbind::Mat skin=wristSkin[side];
            if(Finger(b)){armbind::Mat delta;skeletal::Multiply(delta.m,inverse[b].m,posed[b].m);
                skeletal::Multiply(skin.m,delta.m,wristSkin[side].m);}
            if(!armbind::Finite(skin))return false;
            armbind::ToGpu(skin,reinterpret_cast<float(*)[4]>(&staged[s*3]));
        }
        std::memcpy(palette,staged,registers*4*sizeof(float));return true;
    }
};
} // namespace nativegloves
