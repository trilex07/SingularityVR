#pragma once
#include "gore_policy.h"
// Reviewed subset of SIDE 9c75c68. No CDO edits, packed-bool guesses,
// global censorship bypass or permanently modified shared limb settings.
namespace singularity_gore {
static bool enabled=false;
static int chancePct=100;
static unsigned notes=0,refusals=0;
static volatile LONG damageThread=0;
inline bool Writable(uintptr_t p,size_t n) {
    MEMORY_BASIC_INFORMATION m{};
    if(!p||!n||p+n<p||!VirtualQuery((void*)p,&m,sizeof(m))||m.State!=MEM_COMMIT||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const DWORD access=m.Protect&0xff;
    return (access==PAGE_READWRITE||access==PAGE_WRITECOPY||
        access==PAGE_EXECUTE_READWRITE||access==PAGE_EXECUTE_WRITECOPY)&&
        p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;
}
inline uintptr_t Ptr(uintptr_t p) {
    return Readable((void*)p,4)?*(const uintptr_t*)p:0;
}
inline bool Enemy(uintptr_t object) {
    if(!object||!IsLive(object))return false;
    char name[96]{};
    if(!NameOf(object,name,sizeof(name))||!strncmp(name,"Default__",9))return false;
    bool enemy=false;
    auto cls=Ptr(object+OBJ_CLASS);
    for(unsigned d=0;cls&&d<64;++d){
        if(!NameOf(cls,name,sizeof(name)))return false;
        if(gorepolicy::Excluded(name))return false;
        enemy|=gorepolicy::EnemyBase(name);
        if(!g_upNext)return false;
        cls=Ptr(cls+g_upNext-4);
    }
    return !cls&&enemy;
}
inline int Field(uintptr_t cls,const char* wanted,const char* type) {
    if(!g_upChildren||!g_upNext||!g_upOffset)return -1;
    for(unsigned d=0;cls&&d<64;++d){
        auto f=Ptr(cls+g_upChildren);
        for(unsigned n=0;f&&n<400;++n){
            char name[96]{},kind[64]{};
            if(!Readable((void*)f,0x100))return -1;
            if(NameOf(f,name,sizeof(name))&&!strcmp(name,wanted)){
                if(!NameOf(Ptr(f+OBJ_CLASS),kind,sizeof(kind))||strcmp(kind,type))return -1;
                const int off=*(const int*)(f+g_upOffset);
                return off>=0x40&&off<=0x8000&&(off%4)==0?off:-1;
            }
            f=Ptr(f+g_upNext);
        }
        cls=Ptr(cls+g_upNext-4);
    }
    return -1;
}
struct DamageScope {
    uintptr_t limb=0,cls=0;
    int offsets[2]{-1,-1};
    float before[2]{},after[2]{};
    bool active=false;
    explicit DamageScope(uintptr_t pawn,bool damage) {
        if(!enabled||!damage||!Enemy(pawn))return;
        const LONG thread=(LONG)GetCurrentThreadId();
        const LONG owner=InterlockedCompareExchange(&damageThread,thread,0);
        if(owner&&owner!=thread)return; // Shared settings must not be edited by concurrent damage workers.
        const auto pawnClass=Ptr(pawn+OBJ_CLASS);
        const int settings=Field(pawnClass,"mLimbSettings","ObjectProperty");
        if(settings<0){Refuse("limb-property-type");return;}
        limb=Ptr(pawn+settings);
        if(!limb||!IsLive(limb)){Refuse("limb-live");return;}
        cls=Ptr(limb+OBJ_CLASS);char name[96]{};
        if(!NameOf(cls,name,sizeof(name))||strcmp(name,"RvLimbSettings")){Refuse("limb-class");return;}
        offsets[0]=Field(cls,"mRandomChanceToSeverLimb","FloatProperty");
        offsets[1]=Field(cls,"mHealthPercentToTryRandomSever","FloatProperty");
        if(offsets[0]<0||offsets[1]<0||offsets[0]==offsets[1]){Refuse("float-properties");return;}
        for(unsigned i=0;i<2;++i){
            if(!Writable(limb+offsets[i],4)){Refuse("writable");return;}
            before[i]=*(const float*)(limb+offsets[i]);
        }
        if(!gorepolicy::Settings(before[0],before[1])){Refuse("float-range");return;}
        after[0]=gorepolicy::Chance(chancePct);after[1]=1.0f;
        for(unsigned i=0;i<2;++i)*(float*)(limb+offsets[i])=after[i];
        active=true;
        if(notes++<12)Log("gore scoped: pawn=%08X limb=%08X offsets=%X/%X before=%.3f/%.3f chance=%.3f [native damage window only; NOT sever proof]",
            (unsigned)pawn,(unsigned)limb,offsets[0],offsets[1],before[0],before[1],after[0]);
    }
    void Refuse(const char* stage){if(refusals++<12)Log("gore REFUSE stage=%s; no writes",stage);}
    ~DamageScope(){
        if(!active||!IsLive(limb)||Ptr(limb+OBJ_CLASS)!=cls)return;
        for(unsigned i=0;i<2;++i)if(Writable(limb+offsets[i],4)&&*(const float*)(limb+offsets[i])==after[i])
            *(float*)(limb+offsets[i])=before[i];
    }
    DamageScope(const DamageScope&)=delete;
    DamageScope& operator=(const DamageScope&)=delete;
};
inline void LoadConfig(const char* path){
    enabled=GetPrivateProfileIntA("Gameplay","GoreDismemberment",0,path)!=0;
    chancePct=GetPrivateProfileIntA("Gameplay","GoreChancePct",100,path);
    Log("gore reviewed: enabled=%d chance=%.2f; scoped enemy floats only; CDO/bool/censorship edits disabled",enabled,gorepolicy::Chance(chancePct));
}
}
