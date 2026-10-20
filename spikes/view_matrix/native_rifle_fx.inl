// Native method resolved from the reflected UFunction and its audited virtual
// dispatch. No executable VA, replacement asset, template edit or shader guess.
#include "native_rifle_fx_policy.h"
namespace NativeRifleFx {
using Method=void (__thiscall*)(void*,const hipknife::V*,int);
static Method original=nullptr;
static unsigned notes=0;
static uintptr_t hooked=0;
// Engine-thread-only follow-through: PlayMuzzleFX can return an existing looping
// emitter without touching its transform. Record both the call and later state,
// rather than treating a changed spawn argument as proof of visible particles.
static uintptr_t observedFx=0,observedWeapon=0,observedEmitter=0;
static ULONGLONG observeAfter=0;
static unsigned observations=0;
static uintptr_t Function(uintptr_t cls,const char* wanted){
    for(unsigned depth=0;cls&&depth<24;++depth){
        uintptr_t field=PauseRttProbe::Pointer(cls+g_upChildren);
        for(unsigned n=0;field&&n<512;++n){
            char name[80]{},type[48]{};
            if(!GuardedNameOf(field,name,sizeof(name)))return 0;
            if(!strcmp(name,wanted)){
                if(!GuardedNameOf(PauseRttProbe::Pointer(field+OBJ_CLASS),type,sizeof(type))||strcmp(type,"Function"))return 0;
                return PauseRttProbe::Pointer(field+g_funcOffset);
            }
            const auto next=PauseRttProbe::Pointer(field+g_upNext);
            if(next==field)return 0;field=next;
        }
        const auto next=PauseRttProbe::Pointer(cls+g_upNext-4);
        if(next==cls)return 0;cls=next;
    }
    return 0;
}
static bool Code(uintptr_t address,size_t bytes,unsigned long long hash){
    return address>=g_textLo&&address<g_textHi&&bytes<=g_textHi-address&&
        PauseRttProbe::SafeRange(address,bytes)&&
        uiwitness::Fingerprint((const unsigned char*)address,bytes)==hash;
}
static void EmitterReceipt(const char* phase,uintptr_t fx,uintptr_t emitter){
    if(!emitter||!IsLive(emitter)){
        Log("rifle native follow: phase=%s fx=%08X emitter=%08X not-live [read-only]",phase,(unsigned)fx,(unsigned)emitter);
        return;
    }
    const auto cls=PauseRttProbe::Pointer(emitter+OBJ_CLASS);
    const int location=PauseRttProbe::Property(cls,"Location","StructProperty");
    const int base=PauseRttProbe::Property(cls,"Base","ObjectProperty");
    const int component=PauseRttProbe::Property(cls,"ParticleSystemComponent","ComponentProperty");
    const auto psc=component>=0?PauseRttProbe::Pointer(emitter+component):0;
    const auto pscClass=psc?PauseRttProbe::Pointer(psc+OBJ_CLASS):0;
    const int templ=pscClass?PauseRttProbe::Property(pscClass,"Template","ObjectProperty"):-1;
    const auto asset=templ>=0?PauseRttProbe::Pointer(psc+templ):0;
    char name[96]{};if(asset)GuardedNameOf(asset,name,sizeof(name));
    hipknife::V p{};const bool have=location>=0&&PauseRttProbe::SafeRange(emitter+location,sizeof(p));
    if(have)memcpy(&p,(void*)(emitter+location),sizeof(p));
    Log("rifle native follow: phase=%s fx=%08X emitter=%08X base=%08X locationValid=%d location=(%.3f,%.3f,%.3f) componentOff=%X component=%08X templateOff=%X template=%08X name=%s [read-only, not draw proof]",
        phase,(unsigned)fx,(unsigned)emitter,(unsigned)(base>=0?PauseRttProbe::Pointer(emitter+base):0),
        have?1:0,p.x,p.y,p.z,component,(unsigned)psc,templ,(unsigned)asset,name);
    if(!pscClass)return;
    // Shared bool dwords are logged RAW: no guessed bit masks or state writes.
    const char* flags[]={"bIsActive","bWasDeactivated","HiddenGame","bOwnerNoSee","DepthPriorityGroup"};
    const char* types[]={"BoolProperty","BoolProperty","BoolProperty","BoolProperty","ByteProperty"};
    for(unsigned i=0;i<5;++i){
        const int offset=PauseRttProbe::Property(pscClass,flags[i],types[i]);
        const unsigned value=offset>=0&&PauseRttProbe::SafeRange(psc+offset,4)?
            *(const uint32_t*)(psc+offset):0;
        Log("rifle native follow: phase=%s field=%s offset=%X raw=%08X [packed bytes, NOT interpreted bool]",phase,flags[i],offset,value);
    }
}
static void ObserveTick(){
    if(!observedFx||!g_nativeRifleMuzzle||GetTickCount64()<observeAfter)return;
    const auto fx=observedFx;observedFx=0;
    if(observedWeapon!=(uintptr_t)(uint32_t)Rd(g_weaponObj)||!IsLive(fx)||
       PauseRttProbe::Pointer(fx+0x90)!=observedWeapon)return;
    const auto emitter=PauseRttProbe::Pointer(fx+0x88);
    Log("rifle native follow: delayed emitter retained=%d",emitter==observedEmitter?1:0);
    EmitterReceipt("after-tick",fx,emitter);
}
static void __fastcall Hook(void* self,void*,const hipknife::V* location,int firstPerson){
    if(!original)return;
    weaponmuzzle::Witness witness;
    if(TryAcquireSRWLockShared(&g_nextVisualLock)){
        witness=g_nextMuzzle;ReleaseSRWLockShared(&g_nextVisualLock);
    }
    const auto owner=PauseRttProbe::Pointer((uintptr_t)self+0x90);
    hipknife::V replacement{};
    const bool readable=location&&PauseRttProbe::SafeRange((uintptr_t)location,sizeof(*location));
    const bool scene=VrModeOn()&&!CutsceneNativeCameraArmed()&&!Rd(g_gamePaused)&&!Rd(g_weaponHolstered);
    const bool changed=readable&&riflefx::Origin(g_nativeRifleMuzzle,scene,firstPerson!=0,
        (uint32_t)owner,(uint32_t)g_playerPawn,(uint32_t)Rd(g_weaponObj),GetTickCount64(),
        witness,{g_camPos[0],g_camPos[1],g_camPos[2]},*location,replacement);
    const bool rifle=witness.kind==weaponreload::Kind::Valkyrie||Rd(g_gunVerts)==11387;
    const bool receipt=readable&&rifle&&owner==(uintptr_t)(uint32_t)Rd(g_weaponObj)&&notes++<24;
    if(receipt)
        Log("rifle native emitter: mode=%d first=%d fresh=%d kind=%d changed=%d source=(%.3f,%.3f,%.3f) destination=(%.3f,%.3f,%.3f) [call receipt, not pixels]",
            g_nativeRifleMuzzle,firstPerson,witness.Current((uint32_t)g_playerPawn,(uint32_t)owner,GetTickCount64())?1:0,
            int(witness.kind),changed?1:0,location->x,location->y,location->z,
            changed?replacement.x:location->x,changed?replacement.y:location->y,changed?replacement.z:location->z);
    original(self,changed?&replacement:location,firstPerson);
    if(receipt){
        const auto emitter=PauseRttProbe::Pointer((uintptr_t)self+0x88);
        char name[96]{};
        if(emitter)GuardedNameOf(emitter,name,sizeof(name));
        Log("rifle native emitter: after original fx=%08X emitter=%08X name=%s [emitter identity, not visible content]",
            (unsigned)(uintptr_t)self,(unsigned)emitter,name);
        if(emitter&&observations<3&&!observedFx){
            ++observations;observedFx=(uintptr_t)self;observedWeapon=owner;
            observedEmitter=emitter;observeAfter=GetTickCount64()+50;
            EmitterReceipt("after-call",observedFx,emitter);
        }
    }
}
static void Install(){
    if(!g_nativeRifleMuzzle||original||!g_weaponFxClass||g_funcOffset<0||g_upChildren<=0||g_upNext<=0)return;
    static unsigned attempts=0;static ULONGLONG next=0;
    if(attempts>=3||GetTickCount64()<next)return;
    ++attempts;next=GetTickCount64()+2000;
    const auto wrapper=Function(g_weaponFxClass,"PlayMuzzleFX");
    // Full wrapper proves the vector-reference + UBOOL stack order and ret8.
    if(!Code(wrapper,197,0x1153B073558EDE80ull)||
       PauseRttProbe::Property(g_weaponFxClass,"mWeapon","ObjectProperty")!=0x90||
       PauseRttProbe::Property(g_weaponFxClass,"mLoopingMuzzleFXEmitter","ObjectProperty")!=0x88){
        Log("rifle native emitter: refused wrapper/property contract");return;}
    // Use a live object's vtable, never a hardcoded table from the archive.
    uintptr_t object=0;
    for(int i=0;i<g_fxN;++i)if(g_fx[i].have&&IsLive(g_fx[i].obj)&&
        PauseRttProbe::Pointer(g_fx[i].obj+OBJ_CLASS)==g_weaponFxClass){object=g_fx[i].obj;break;}
    if(!object){--attempts;return;}
    const auto method=PauseRttProbe::Pointer(PauseRttProbe::Pointer(object)+0x120);
    if(!Code(method,846,0xDEA566CA454CAFEBull)){
        Log("rifle native emitter: refused native method fingerprint");return;}
    auto status=MH_CreateHook((void*)method,(void*)&Hook,(void**)&original);
    if(status!=MH_OK){original=nullptr;Log("rifle native emitter: CreateHook=%d",int(status));return;}
    status=MH_EnableHook((void*)method);
    if(status!=MH_OK){MH_RemoveHook((void*)method);original=nullptr;Log("rifle native emitter: EnableHook=%d",int(status));return;}
    hooked=method;
    Log("rifle native emitter: audited hook armed mode=%d [not visual proof]",g_nativeRifleMuzzle);
}
}
