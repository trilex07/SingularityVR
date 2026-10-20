// Native method resolved from the reflected UFunction and its audited virtual
// dispatch. No executable VA, replacement asset, template edit or shader guess.
#include "native_rifle_fx_policy.h"
namespace NativeRifleFx {
using Method=void (__thiscall*)(void*,const hipknife::V*,int);
static Method original=nullptr;
static unsigned notes=0;
static uintptr_t hooked=0;
using SetLocation=int (__thiscall*)(void*,hipknife::V);
using SetRotation=int (__thiscall*)(void*,int,int,int);
static_assert(sizeof(hipknife::V)==12&&sizeof(int)==4,"audited x86 native pose ABI");
static SetLocation setLocation=nullptr;
static SetRotation setRotation=nullptr;
static uintptr_t movingFx=0,movingWeapon=0,movingPawn=0,movingEmitter=0,movingPsc=0,movingTemplate=0;
static int pscOffset=-1,templateOffset=-1;
static uintptr_t emitterLayout=0,pscLayout=0;
static ULONGLONG firedAt=0;
static unsigned moveNotes=0;
// Engine-thread-only follow-through: PlayMuzzleFX can return an existing looping
// emitter without touching its transform. Record both the call and later state,
// rather than treating a changed spawn argument as proof of visible particles.
static uintptr_t observedFx=0,observedWeapon=0,observedEmitter=0;
static ULONGLONG observeAfter=0;
static unsigned observations=0;
using SourceGetter=hipknife::V* (__thiscall*)(void*,hipknife::V*);
static SourceGetter sourceOriginal=nullptr;
static uintptr_t tracerCaller=0;
static SRWLOCK tracerFxLock=SRWLOCK_INIT;
static uintptr_t tracerFx=0,tracerPawn=0,tracerWeapon=0;
static bool Scene();
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
static bool FloatReceipt(uintptr_t object,const char* property,float& value){
    if(!object||!IsLive(object))return false;
    const auto cls=PauseRttProbe::Pointer(object+OBJ_CLASS);
    const int offset=PauseRttProbe::Property(cls,property,"FloatProperty");
    if(offset<0||!PauseRttProbe::SafeRange(object+offset,sizeof(value)))return false;
    memcpy(&value,(void*)(object+offset),sizeof(value));
    return std::isfinite(value);
}
static void BallisticsReceipt(uintptr_t weapon){
    // Called only for the eight exact source replacements below. No new hook,
    // polling loop, native spread write or forced cosmetic particle spawning.
    uintptr_t fx=0;
    if(TryAcquireSRWLockShared(&tracerFxLock)){
        if(tracerWeapon==weapon&&tracerPawn==g_playerPawn)fx=tracerFx;
        ReleaseSRWLockShared(&tracerFxLock);
    }
    if(!fx||!IsLive(fx)||PauseRttProbe::Pointer(fx+OBJ_CLASS)!=g_weaponFxClass||
       PauseRttProbe::Pointer(fx+0x90)!=weapon)return;
    // The full PlayTracerFX contract reads template+48 / chance+4C, AFTER
    // this source getter. Reflection establishes their names in this session.
    const bool contract=PauseRttProbe::Property(g_weaponFxClass,"mTracerChance","FloatProperty")==0x4c&&
        PauseRttProbe::Property(g_weaponFxClass,"mTracerParticles","ObjectProperty")==0x48;
    float chance=0,current=0,base=0,minimum=0,maximum=0;
    const bool haveChance=contract&&FloatReceipt(fx,"mTracerChance",chance);
    const bool haveCurrent=FloatReceipt(weapon,"mSpreadCurrentTotal",current);
    const bool haveBase=FloatReceipt(weapon,"mSpreadBase",base);
    const bool haveMin=FloatReceipt(weapon,"mSpreadMin",minimum);
    const bool haveMax=FloatReceipt(weapon,"mSpreadMax",maximum);
    Log("rifle native ballistics: weapon=%08X fx=%08X chanceValid=%d chance=%.6f template=%08X spreadValid=%d/%d/%d/%d current=%.6f base=%.6f min=%.6f max=%.6f [read-only authored values; units/pixels NOT proven]",
        (unsigned)weapon,(unsigned)fx,haveChance?1:0,chance,
        contract?(unsigned)PauseRttProbe::Pointer(fx+0x48):0,
        haveCurrent?1:0,haveBase?1:0,haveMin?1:0,haveMax?1:0,current,base,minimum,maximum);
}
static hipknife::V* __fastcall SourceHook(void* self,void*,hipknife::V* output){
    const auto caller=(uintptr_t)_ReturnAddress();
    auto* result=sourceOriginal(self,output);
    if(result!=output||!output||!PauseRttProbe::SafeRange((uintptr_t)output,sizeof(*output)))return result;
    weaponmuzzle::Witness w;
    if(!TryAcquireSRWLockShared(&g_nextVisualLock))return result;
    w=g_nextMuzzle;ReleaseSRWLockShared(&g_nextVisualLock);
    hipknife::V replacement{};
    if(riflefx::TracerSource(g_nativeRifleTracer,Scene(),caller==tracerCaller,
       (uintptr_t)self,(uint32_t)g_playerPawn,(uint32_t)Rd(g_weaponObj),GetTickCount64(),w,
       {g_camPos[0],g_camPos[1],g_camPos[2]},*output,replacement)){
        static unsigned count=0;if(count<8){++count;
            Log("rifle native tracer: exact source (%.3f,%.3f,%.3f)->(%.3f,%.3f,%.3f); endpoint unchanged [not pixels]",
                output->x,output->y,output->z,replacement.x,replacement.y,replacement.z);
            BallisticsReceipt((uintptr_t)(uint32_t)Rd(g_weaponObj));
        }
        *output=replacement;
    }
    return result;
}
static uintptr_t Helper(uintptr_t wrapper,unsigned offset,size_t bytes,unsigned long long hash){
    if(!wrapper||!PauseRttProbe::SafeRange(wrapper+offset,5)||*(const unsigned char*)(wrapper+offset)!=0xe8)return 0;
    const auto address=wrapper+offset+5+*(const int32_t*)(wrapper+offset+1);
    return Code(address,bytes,hash)?address:0;
}
static bool BindMovement(){
    if(setLocation&&setRotation)return true; // No repeated image scans while waiting for a live FX object.
    const auto location=NativeRegistry::Find("intAActorexecSetLocation");
    const auto rotation=NativeRegistry::Find("intAActorexecSetRotation");
    if(!Code(location,121,0xD75050CD7801E36Bull)||!Code(rotation,109,0xCB407E96A5DEE5B3ull)){
        Log("rifle native pose: REFUSE wrapper locations=%08X rotation=%08X",(unsigned)location,(unsigned)rotation);return false;}
    // Exact exec -> E8 helper -> full body proves FVector/FRotator BY VALUE,
    // ret 0xC. These are native movement functions, not reflected exec ABIs.
    setLocation=(SetLocation)Helper(location,0x67,29,0x8A32507F9F7A934Aull);
    setRotation=(SetRotation)Helper(rotation,0x5b,139,0xAA63F1F63DEFF3D8ull);
    if(!setLocation||!setRotation)Log("rifle native pose: REFUSE helper location=%08X rotation=%08X",(unsigned)(uintptr_t)setLocation,(unsigned)(uintptr_t)setRotation);
    return setLocation&&setRotation;
}
static bool Scene(){return VrModeOn()&&!CutsceneNativeCameraArmed()&&!Rd(g_gamePaused)&&
    !Rd(g_panelOpen)&&!Rd(g_weaponWheelOpen)&&!Rd(g_weaponHolstered);}
static void InstallTracer(){
    if(!g_nativeRifleTracer||sourceOriginal||!Scene()||!g_weaponFxClass)return;
    weaponmuzzle::Witness w;
    if(!TryAcquireSRWLockShared(&g_nextVisualLock))return;
    w=g_nextMuzzle;ReleaseSRWLockShared(&g_nextVisualLock);
    const auto weapon=(uintptr_t)(uint32_t)Rd(g_weaponObj);
    if(w.kind!=weaponreload::Kind::Valkyrie||!w.Current((uint32_t)g_playerPawn,(uint32_t)weapon,GetTickCount64()))return;
    static unsigned attempts=0;if(attempts>=3)return;
    uintptr_t object=0;
    uintptr_t received=0,receivedPawn=0,receivedWeapon=0;
    if(TryAcquireSRWLockShared(&tracerFxLock)){
        received=tracerFx;receivedPawn=tracerPawn;receivedWeapon=tracerWeapon;
        ReleaseSRWLockShared(&tracerFxLock);
    }
    // The suppression census is capped at eight global FX instances. Its pool
    // can be full BEFORE the player's FX is spawned. The native muzzle callback
    // has already proved the exact current local object: prefer that receipt,
    // revalidating ownership/class/liveness before using its vtable.
    if(received&&receivedWeapon==weapon&&receivedPawn==g_playerPawn&&IsLive(received)&&
       PauseRttProbe::Pointer(received+OBJ_CLASS)==g_weaponFxClass&&
       PauseRttProbe::Pointer(received+0x90)==weapon)object=received;
    for(int i=0;i<g_fxN;++i)if(g_fx[i].have&&IsLive(g_fx[i].obj)&&
       PauseRttProbe::Pointer(g_fx[i].obj+OBJ_CLASS)==g_weaponFxClass&&
       PauseRttProbe::Pointer(g_fx[i].obj+0x90)==weapon){object=g_fx[i].obj;break;}
    if(!object){static unsigned missing=0;if(missing++<3)
        Log("rifle native tracer: waiting exact local FX; census=%d full=%d receivedLocal=%d",g_fxN,g_fxN==kFxMax?1:0,received?1:0);return;}
    ++attempts;
    const auto wrapper=Function(g_weaponFxClass,"PlayTracerFX");
    const auto tracer=PauseRttProbe::Pointer(PauseRttProbe::Pointer(object)+0x128);
    const auto getter=PauseRttProbe::Pointer(PauseRttProbe::Pointer(weapon+0x2d4)+8);
    // Wrapper: one const FVector& ENDPOINT, virtual +128. Native body calls
    // source-interface +8 at +8C (return +8E). Never replace the endpoint.
    // Getter contract includes BOTH return branches (156 bytes), not the
    // 114-byte prefix ending at its first conditional-path ret.
    if(!Code(wrapper,141,0xF637C8C8C78F8557ull)||
       !Code(tracer,1694,0xB7DDF9B80E425AABull)||
       !Code(getter,156,0xDC2F29F0817B2902ull)){
        Log("rifle native tracer: REFUSE exact wrapper/method/interface contracts");return;
    }
    auto status=MH_CreateHook((void*)getter,(void*)&SourceHook,(void**)&sourceOriginal);
    if(status!=MH_OK){sourceOriginal=nullptr;return;}
    tracerCaller=tracer+0x8e;
    status=MH_EnableHook((void*)getter);
    if(status!=MH_OK){MH_RemoveHook((void*)getter);sourceOriginal=nullptr;tracerCaller=0;return;}
    Log("rifle native tracer: source-only hook armed; impact/spread/endpoint untouched");
}
static void MaintainTick(){
    if(!firedAt)return;
    static thread_local bool running=false;if(running)return;
    weaponmuzzle::Witness w;
    if(!TryAcquireSRWLockShared(&g_nextVisualLock))return;
    w=g_nextMuzzle;ReleaseSRWLockShared(&g_nextVisualLock);
    const auto now=GetTickCount64();
    if(!setLocation||!setRotation||movingPawn!=g_playerPawn||
       movingWeapon!=(uintptr_t)(uint32_t)Rd(g_weaponObj)||
       !riflefx::Lease(g_nativeRifleMuzzle,Scene(),firedAt,now,(uint32_t)movingPawn,(uint32_t)movingWeapon,w)||
       !IsLive(movingPawn)||!IsLive(movingWeapon)||!IsLive(movingFx)||!IsLive(movingEmitter)||!IsLive(movingPsc)||
       PauseRttProbe::Pointer(movingEmitter+OBJ_CLASS)!=emitterLayout||
       PauseRttProbe::Pointer(movingPsc+OBJ_CLASS)!=pscLayout||
       PauseRttProbe::Pointer(movingFx+0x90)!=movingWeapon||
       PauseRttProbe::Pointer(movingFx+0x88)!=movingEmitter||
       PauseRttProbe::Pointer(movingEmitter+pscOffset)!=movingPsc||
       PauseRttProbe::Pointer(movingPsc+templateOffset)!=movingTemplate){firedAt=0;return;}
    const hipknife::V camera{g_camPos[0],g_camPos[1],g_camPos[2]};
    hipknife::V position{};float aim[3]{};int pitch=0,yaw=0;
    GunAimDir(aim); // Same engine-space axis as the accepted bullet/laser path.
    if(!riflefx::Origin(3,true,true,(uint32_t)movingWeapon,(uint32_t)movingPawn,
       (uint32_t)movingWeapon,now,w,camera,camera,position)||
       !riflefx::Rotation({aim[0],aim[1],aim[2]},pitch,yaw)){firedAt=0;return;}
    running=true;
    const int moved=setLocation((void*)movingEmitter,position);
    const int turned=moved?setRotation((void*)movingEmitter,pitch,yaw,0):0;
    running=false;
    if(moveNotes++<12)Log("rifle native pose: emitter=%08X template=%08X moved=%d turned=%d position=(%.3f,%.3f,%.3f) rot=(%d,%d,0) [native transform receipt, NOT draw proof]",
        (unsigned)movingEmitter,(unsigned)movingTemplate,moved,turned,position.x,position.y,position.z,pitch,yaw);
    if(!moved||!turned)firedAt=0;
}
static void TrackRefusal(const char* stage){static unsigned count=0;if(count<8){++count;Log("rifle native pose: REFUSE emitter/%s; no movement",stage);}}
static void Track(uintptr_t fx,uintptr_t weapon,const weaponmuzzle::Witness& w){
    if(g_nativeRifleMuzzle!=3||!setLocation||!setRotation||!Scene()||
       !w.Current((uint32_t)g_playerPawn,(uint32_t)weapon,GetTickCount64())||
       w.kind!=weaponreload::Kind::Valkyrie)return;
    const auto emitter=PauseRttProbe::Pointer(fx+0x88);
    if(!emitter||!IsLive(emitter)){TrackRefusal("live");return;}
    const auto cls=PauseRttProbe::Pointer(emitter+OBJ_CLASS);char name[64]{};
    if(!GuardedNameOf(cls,name,sizeof(name))||strcmp(name,"RvEmitter")){TrackRefusal("class");return;}
    const int component=emitterLayout==cls?pscOffset:
        PauseRttProbe::Property(cls,"ParticleSystemComponent","ComponentProperty");
    const auto psc=component>=0?PauseRttProbe::Pointer(emitter+component):0;
    if(!psc||!IsLive(psc)){TrackRefusal("component");return;}
    const auto pscClass=PauseRttProbe::Pointer(psc+OBJ_CLASS);
    const int templ=pscLayout==pscClass?templateOffset:PauseRttProbe::Property(pscClass,"Template","ObjectProperty");
    const auto asset=templ>=0?PauseRttProbe::Pointer(psc+templ):0;
    // These two original first-person slots are proven by the full native
    // PlayMuzzleFX fingerprint: regular loop + iron-sight loop. Never move AI
    // effects, impact actors, templates or an unrelated pooled emitter.
    if(!asset||(asset!=PauseRttProbe::Pointer(fx+0x6c)&&asset!=PauseRttProbe::Pointer(fx+0x84))){TrackRefusal("first-person-template");return;}
    movingFx=fx;movingWeapon=weapon;movingPawn=g_playerPawn;movingEmitter=emitter;
    movingPsc=psc;movingTemplate=asset;pscOffset=component;templateOffset=templ;
    emitterLayout=cls;pscLayout=pscClass;
    firedAt=GetTickCount64();MaintainTick();
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
    const bool scene=Scene();
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
    if(changed&&TryAcquireSRWLockExclusive(&tracerFxLock)){
        tracerFx=(uintptr_t)self;tracerPawn=g_playerPawn;tracerWeapon=owner;
        ReleaseSRWLockExclusive(&tracerFxLock);
    }
    if(changed)Track((uintptr_t)self,owner,witness);
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
    InstallTracer();
    if(!g_nativeRifleMuzzle||original||!g_weaponFxClass||g_funcOffset<0||g_upChildren<=0||g_upNext<=0)return;
    static unsigned attempts=0;static ULONGLONG next=0;
    if(attempts>=3||GetTickCount64()<next)return;
    ++attempts;next=GetTickCount64()+2000;
    if(g_nativeRifleMuzzle==3&&!BindMovement()){
        Log("rifle native pose: refused exact SetLocation/SetRotation contracts; no movement hook");return;}
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
