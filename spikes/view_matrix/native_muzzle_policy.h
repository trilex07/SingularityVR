#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
namespace nativefx {
inline bool ProbeWeapon(unsigned requested,unsigned current){return !requested||requested==current;}
inline std::uint64_t Hash(const void* data,unsigned bytes){
    auto p=static_cast<const unsigned char*>(data);std::uint64_t h=14695981039346656037ull;
    for(unsigned i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}return h;
}
inline bool Hex(const char* text,std::uint64_t& out){
    if(!text||std::strlen(text)!=16)return false;std::uint64_t n=0;
    for(unsigned i=0;i<16;++i){const char c=text[i];unsigned d=99;
        if(c>='0'&&c<='9')d=c-'0';if(c>='a'&&c<='f')d=c-'a'+10;if(c>='A'&&c<='F')d=c-'A'+10;
        if(d>15)return false;n=(n<<4)|d;}
    if(!n)return false;out=n;return true;
}
struct Selection {
    std::uint64_t vs=0,ps=0;unsigned vsBytes=0,psBytes=0;int mode=0,convention=-1;
    bool Valid()const{return (mode==1||mode==3||mode==4||(mode==2&&(convention==0||convention==1)))&&vs&&ps&&vsBytes>=8&&psBytes>=8&&vsBytes<=32768&&psBytes<=32768&&vsBytes%4==0&&psBytes%4==0;}
    bool Match(std::uint64_t v,std::uint64_t p,unsigned vb,unsigned pb)const{
        return Valid()&&vs==v&&ps==p&&vsBytes==vb&&psBytes==pb;
    }
};
// Captured original emitter ON/OFF comparison; all three VS disassemblies
// multiply particle position by row-vector LocalToWorld c5-c8, then VP c0-c3.
// These are world particles even when an affine c5-c8 write arms the HUD hook.
inline int ParticleFamily(std::uint64_t vs,std::uint64_t ps,unsigned vb,unsigned pb){
    if(vs==0x314D478A9CFDBE37ull&&ps==0xC118540CCDBCB241ull&&vb==3384&&pb==448)return 0;
    if(vs==0x488BAEB579E71A73ull&&ps==0x27DFE7D9C67BCC03ull&&vb==2968&&pb==452)return 1;
    if(vs==0x4FAEF2389750DBFCull&&ps==0x2FEC493CD52D7D78ull&&vb==2908&&pb==336)return 2;
    return -1;
}
template<class Slot> inline bool ParticleCameraReady(const Slot* slots,int n){
    if(!slots||n<1)return false;
    bool found=false;
    for(int i=0;i<n;++i)if(slots[i].valid){
        // A second tracked camera overlapping the particle's LocalToWorld
        // would violate the reason for bypassing HUD. Refuse that state.
        if(slots[i].reg<9&&slots[i].reg+4>5)return false;
        if(slots[i].reg==0){
            if(found||slots[i].conv!=0)return false; // disassembly proves ROW
            for(unsigned k=0;k<16;++k)if(!std::isfinite(slots[i].m[k]))return false;
            found=true;
        }
    }
    return found;
}
template<class Slot> inline int Alias(const float* gpu,unsigned reg,const Slot* slots,int n){
    if(!gpu||reg>252||n<0)return -1;
    for(unsigned k=0;k<16;++k)if(!std::isfinite(gpu[k]))return -1;
    int found=-1;
    for(int i=0;i<n;++i)if(slots[i].valid){
        // Never remap the same register twice or overwrite a partially overlapping slot.
        if(reg<slots[i].reg+4&&slots[i].reg<reg+4)return -1;
        if(!std::memcmp(gpu,slots[i].m,16*sizeof(float)))found=i;
    }
    return found;
}
// Render-thread sequence; external shot serial is the only cross-thread input.
// Require an idle period first, discard a pre-baseline shot, cap total captures.
struct CapturePlan {
    unsigned idle=0,stage=0;long lastShot=0;
    int Tick(bool ready,long shot,bool busy){
        if(stage>=4)return 0;
        if(!ready){idle=0;lastShot=shot;return 0;}
        if(busy){lastShot=shot;return 0;}
        if(!stage){if(shot!=lastShot){idle=0;lastShot=shot;return 0;}
            if(++idle<90)return 0;stage=1;lastShot=shot;return 1;}
        if(shot==lastShot)return 0;
        lastShot=shot;++stage;return 2; // next full frame after native FireAmmunition.
    }
};
} // namespace nativefx
