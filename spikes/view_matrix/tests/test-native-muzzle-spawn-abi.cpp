#include "../native_muzzle_spawn_abi.h"
#include "../native_rifle_fx_policy.h"
#include <cassert>
static void* seenSelf;static const hipknife::V* seenPosition;static const hipknife::V* seenDirection;
static std::uint32_t seen[9];
static unsigned detached;
static __declspec(noinline) void __fastcall BaseSpy(void* self,void*,void* base,hipknife::V normal,
    int notify,void* mesh,std::uint32_t name,std::uint32_t number){
    assert(self&&base==nullptr&&mesh==nullptr&&notify==1&&name==0&&number==0);
    assert(normal.x==0&&normal.y==0&&normal.z==1);++detached;
}
static __declspec(noinline) void __fastcall Spy(void* self,void*,const hipknife::V* p,const hipknife::V* d,
    std::uint32_t a3,std::uint32_t a4,std::uint32_t a5,std::uint32_t a6,std::uint32_t a7,
    std::uint32_t a8,std::uint32_t a9,std::uint32_t a10,std::uint32_t a11){
    seenSelf=self;seenPosition=p;seenDirection=d;
    const std::uint32_t args[]{a3,a4,a5,a6,a7,a8,a9,a10,a11};
    for(unsigned i=0;i<9;++i)seen[i]=args[i];
}
int main(){
    static_assert(sizeof(void*)==4,"actual x86 ABI test");
    union Adapter {decltype(&Spy) hook;muzzleabi::Method native;} adapter{};
    adapter.hook=&Spy;
    hipknife::V position{1,2,3},nativeDirection{1,0,0},measured{0,1,0};
    unsigned before=0,after=0;
    for(unsigned frame=0;frame<1000;++frame){
        const auto* direction=frame%2?&measured:&nativeDirection;
        __asm mov before,esp
        muzzleabi::Forward(adapter.native,&position,&position,direction,3,4,5,6,7,8,9,10,11);
        __asm mov after,esp
        assert(before==after&&seenSelf==&position&&seenPosition==&position&&seenDirection==direction);
        for(unsigned i=0;i<9;++i)assert(seen[i]==i+3);
        const auto attachment=riflefx::WorldMuzzleAttachment(frame%2!=0,{8,9,10,11});
        __asm mov before,esp
        muzzleabi::Forward(adapter.native,&position,&position,direction,3,4,5,6,7,
            attachment.base,attachment.mesh,attachment.name,attachment.number);
        __asm mov after,esp
        assert(before==after);
        for(unsigned i=0;i<5;++i)assert(seen[i]==i+3); // template, owner, DPG, flags
        for(unsigned i=5;i<9;++i)assert(seen[i]==(frame%2?0:i+3));
        union BaseAdapter {decltype(&BaseSpy) spy;muzzleabi::SetBase native;} base{};
        base.spy=&BaseSpy;
        __asm mov before,esp
        muzzleabi::Detach(base.native,&position);
        __asm mov after,esp
        assert(before==after);
    }
    assert(detached==1000);
}
