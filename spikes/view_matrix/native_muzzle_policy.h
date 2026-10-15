#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
namespace nativefx {
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
    std::uint64_t vs=0,ps=0;unsigned vsBytes=0,psBytes=0;int mode=0;
    bool Valid()const{return (mode==1||mode==2)&&vs&&ps&&vsBytes>=8&&psBytes>=8&&vsBytes<=32768&&psBytes<=32768&&vsBytes%4==0&&psBytes%4==0;}
    bool Match(std::uint64_t v,std::uint64_t p,unsigned vb,unsigned pb)const{
        return Valid()&&vs==v&&ps==p&&vsBytes==vb&&psBytes==pb;
    }
};
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
