// MAIN-only experimental backend. Owns a fresh engine SceneCapture2DComponent
// and TextureRenderTarget2D; never borrows a game's capture or UI texture.
// Native helpers are derived from reflected wrappers and fingerprinted against
// the offline archive, not called through committed executable addresses.
namespace NativeScopePip {
using V=hipknife::V;
using Construct=void* (__cdecl*)(void*,void*,uint32_t,uint32_t,uint32_t,uint32_t,void*,void*,int,int);
using InitTarget=void (__thiscall*)(void*,int,int,int);
using Component=void (__thiscall*)(void*,void*);
using SetView=void (__thiscall*)(void*,V,int,int,int);
using Rate=void (__thiscall*)(void*,float);
using Rebuild=void (__thiscall*)(void*);
static uintptr_t captureClass=0,targetClass=0,owner=0,capture=0;
static volatile LONG target=0;
static Construct construct=nullptr;static InitTarget init=nullptr;
static Component attach=nullptr,detach=nullptr;static SetView view=nullptr;
static Rate rate=nullptr;static Rebuild rebuild=nullptr;static void* errors=nullptr;
static bool attached=false,failed=false;
static bool bound=false;
static unsigned allocations=0;
static unsigned notes=0;
static scopepip::Completion completion;
static IDirect3DStateBlock9* state=nullptr;
static uintptr_t completedTarget=0;
static ULONGLONG completedMs=0;
static volatile LONG presentationFault=0;
static scopepip::PresentationBudget presentationBudget;
static volatile LONG resolvedTarget=0;
// SetView changes the COMPONENT matrix (+A0). The fingerprinted info ctor
// COPIES it into the render proxy (+50), which is what Render actually reads.
// Transfer only our owned view through a mailbox, never share engine pointers
// across threads or modify another capture's matrix.
static SRWLOCK viewLock=SRWLOCK_INIT;
struct ViewSample{uintptr_t target=0;ULONGLONG tick=0;unsigned sequence=0;float matrix[16]{},projection[16]{};};
static ViewSample viewSample;
static ViewSample renderedViewSample;
struct OpticalSample{uintptr_t target=0;ULONGLONG tick=0;float degrees=0;};
static OpticalSample opticalSample;
static scopepip::CaptureCadence cadence;
using RenderCapture=void (__thiscall*)(void*,void*);
static RenderCapture renderOriginal=nullptr;
static void __fastcall RenderHook(void* self,void*,void* family){
    if(!renderOriginal)return;
    const auto expected=(uintptr_t)(uint32_t)Rd(target);
    // Factory -> capture-info constructor proves +0x10 is the exact owned
    // TextureRenderTarget2D, NOT an inferred dimensions/format match.
    const auto actual=PauseRttProbe::Pointer((uintptr_t)self+0x10);
    scopepip::RenderLease lease(g_scopeNativeDepth,g_sniperPip,expected,actual);
    ViewSample submitted;
    if(lease.own){
        if(!cadence.Due(actual,GetTickCount64(),g_scopeOpticalCalibration!=0))return;
        ViewSample latest;
        if(TryAcquireSRWLockShared(&viewLock)){
            latest=viewSample;ReleaseSRWLockShared(&viewLock);
        }
        const auto now=GetTickCount64();
        if(latest.target!=actual||!latest.sequence||now<latest.tick||now-latest.tick>150||
           !PauseRttProbe::SafeRange((uintptr_t)self+0x50,128))return;
        float difference=0;const auto* prior=(const float*)((uintptr_t)self+0x50);
        for(unsigned i=0;i<16;++i)difference=std::max(difference,std::fabs(prior[i]-latest.matrix[i]));
        static unsigned views=0;
        if(views<8&&difference>.00001f){++views;
            Log("scope PiP: owned proxy view delta=%.7g sequence=%u age=%llu ms; component A0 -> exact info50",difference,latest.sequence,now-latest.tick);}
        std::memcpy((void*)((uintptr_t)self+0x50),latest.matrix,64);
        // Same fingerprinted ctor copies component E0 -> info90. Updating
        // only the component leaves the renderer's old projection frozen.
        if(g_scopeOpticalCalibration)std::memcpy((void*)((uintptr_t)self+0x90),latest.projection,64);
        submitted=latest;
    }
    static unsigned count=0;
    const bool receipt=lease.own&&count<3;
    const auto draws=g_scopeNativeDraws,uploads=g_scopeNativeUploads;
    LARGE_INTEGER begin{},end{},frequency{};
    if(lease.own){QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&begin);}
    renderOriginal(self,family);
    // Capture changes engine GPU state legitimately. Never apply a state block
    // across it (that would disagree with UE3's RHI cache). Next main-view
    // constants must revalidate; optical matrices cannot survive as eye data.
    if(lease.own){
        QueryPerformanceCounter(&end);
        const double elapsed=1000.0*double(end.QuadPart-begin.QuadPart)/double(frequency.QuadPart);
        static ULONGLONG window=0;static unsigned samples=0,windows=0;
        static double sum=0,peak=0;
        sum+=elapsed;peak=std::max(peak,elapsed);++samples;
        const auto now=GetTickCount64();if(!window)window=now;
        if(now-window>=1000&&windows<30){++windows;
            Log("scope PiP: CPU callback timing samples=%u meanMs=%.3f maxMs=%.3f draws=%u uploads=%u [GPU execution NOT measured]",
                samples,sum/samples,peak,g_scopeNativeDraws-draws,g_scopeNativeUploads-uploads);
            window=now;samples=0;sum=peak=0;}
        InvalidateCamMats();g_camMatrixBound=false;
        // Full native callback includes the RHI resolve. An RT unbind alone
        // can occur BEFORE this texture has received the completed scene.
        if(g_scopeNativeDraws!=draws){
            AcquireSRWLockExclusive(&viewLock);
            renderedViewSample=submitted;
            ReleaseSRWLockExclusive(&viewLock);
            InterlockedExchange(&resolvedTarget,(LONG)actual);
            completedMs=GetTickCount64();
        }
    }
    if(receipt){++count;
        Log("scope PiP: own native render isolation target=%08X depth=%u nativeDraws=%u rawUploads=%u elapsedMs=%llu [execution only, not alignment]",
            (unsigned)actual,g_scopeNativeDepth,g_scopeNativeDraws-draws,g_scopeNativeUploads-uploads,
            (ULONGLONG)(1000.0*double(end.QuadPart-begin.QuadPart)/double(frequency.QuadPart)));}
}

static bool CodeContract(const char* stage,uintptr_t address,size_t bytes,unsigned long long hash){
    if(NativeRifleFx::Code(address,bytes,hash))return true;
    const bool readable=address>=g_textLo&&address<g_textHi&&bytes<=g_textHi-address&&
        PauseRttProbe::SafeRange(address,bytes);
    const auto actual=readable?uiwitness::Fingerprint((const unsigned char*)address,bytes):0ull;
    Log("scope PiP: REFUSE stage=%s address=%08X bytes=%u readable=%d expected=%016llX actual=%016llX; no engine writes",
        stage,(unsigned)address,(unsigned)bytes,readable?1:0,hash,actual);
    return false;
}
static bool FieldContract(const char* name,uintptr_t cls,const char* type,int expected){
    const int actual=PauseRttProbe::Property(cls,name,type);
    if(actual==expected)return true;
    Log("scope PiP: REFUSE stage=field/%s class=%08X type=%s expected=%X actual=%X; no engine writes",
        name,(unsigned)cls,type,expected,actual);
    return false;
}
static uintptr_t Call(const char* stage,uintptr_t wrapper,unsigned offset,size_t bytes,unsigned long long hash){
    if(!PauseRttProbe::SafeRange(wrapper+offset,5)||*(const unsigned char*)(wrapper+offset)!=0xe8){
        Log("scope PiP: REFUSE stage=%s/direct-call wrapper=%08X offset=%X; no engine writes",stage,(unsigned)wrapper,offset);
        return 0;
    }
    const auto result=wrapper+offset+5+*(const int32_t*)(wrapper+offset+1);
    return CodeContract(stage,result,bytes,hash)?result:0;
}
static bool BindRender(uintptr_t component){
    if(renderOriginal)return true;
    const auto factory=PauseRttProbe::Pointer(PauseRttProbe::Pointer(component)+0x164);
    if(!CodeContract("capture-info/factory",factory,205,0xC87668CD09D29B85ull))return false;
    const auto ctor=Call("capture-info/constructor",factory,0xb7,195,0xF2ECDE1DE85EA63Aull);
    if(!ctor||!PauseRttProbe::SafeRange(ctor+0x63,6)||
       *(const uint16_t*)(ctor+0x63)!=0x06c7)return false;
    const auto table=PauseRttProbe::Pointer(ctor+0x65);
    const auto method=PauseRttProbe::Pointer(table+4);
    if(!CodeContract("capture-info/render",method,737,0x9CCA1CFB4E9B6123ull))return false;
    auto status=MH_CreateHook((void*)method,(void*)&RenderHook,(void**)&renderOriginal);
    if(status!=MH_OK){renderOriginal=nullptr;return false;}
    status=MH_EnableHook((void*)method);
    if(status!=MH_OK){MH_RemoveHook((void*)method);renderOriginal=nullptr;return false;}
    Log("scope PiP: exact owned render isolation armed [not runtime render proof]");return true;
}
static bool FindClasses(){
    if(captureClass&&targetClass)return true;
    static int cursor=0;static uintptr_t data=0;static int count=0;
    if(!data){
        data=PauseRttProbe::Pointer(kGObjectsTArray);
        if(!PauseRttProbe::SafeRange(kGObjectsTArray+4,4))return false;
        count=*(const int32_t*)(kGObjectsTArray+4);
        if(!data||count<=0||count>262144){data=0;return false;}
    }
    // Bounded prewarm, never a failed full walk on every frame or trigger.
    for(unsigned budget=0;cursor<count&&budget<1024;++cursor,++budget){
        const auto obj=PauseRttProbe::Pointer(data+cursor*4);
        char name[80]{},type[48]{};
        if(!obj||!GuardedNameOf(obj,name,sizeof(name)))continue;
        if(strcmp(name,"SceneCapture2DComponent")&&strcmp(name,"TextureRenderTarget2D"))continue;
        if(!GuardedNameOf(PauseRttProbe::Pointer(obj+OBJ_CLASS),type,sizeof(type))||strcmp(type,"Class"))continue;
        if(!strcmp(name,"SceneCapture2DComponent"))captureClass=obj;else targetClass=obj;
    }
    if(cursor>=count&&(!captureClass||!targetClass)){failed=true;Log("scope PiP: class prewarm exhausted; no engine writes");}
    return captureClass&&targetClass;
}
static bool Bind(uintptr_t pawn){
    // Bind the exact engine class's registered exec, not a possibly unbound
    // UFunction::Func or a script override inherited by the current pawn.
    // The registration rows are independently measured by the offline auditor.
    const auto create=NativeRegistry::Find("intUTextureRenderTarget2DexecCreate");
    const auto setView=NativeRegistry::Find("intUSceneCapture2DComponentexecSetView");
    const auto setParams=NativeRegistry::Find("intUSceneCapture2DComponentexecSetCaptureParameters");
    const auto setRate=NativeRegistry::Find("intUSceneCaptureComponentexecSetFrameRate");
    const auto cls=PauseRttProbe::Pointer(pawn+OBJ_CLASS);
    const auto add=NativeRegistry::Find("intAActorexecAttachComponent");
    const auto remove=NativeRegistry::Find("intAActorexecDetachComponent");
    Log("scope PiP: native registry Create=%08X reflected=%08X SetView=%08X reflected=%08X Attach=%08X reflected=%08X [all code/field contracts still required]",
        (unsigned)create,(unsigned)NativeRifleFx::Function(targetClass,"Create"),
        (unsigned)setView,(unsigned)NativeRifleFx::Function(captureClass,"SetView"),
        (unsigned)add,(unsigned)NativeRifleFx::Function(cls,"AttachComponent"));
    Log("scope PiP: preflight captureClass=%08X targetClass=%08X pawnClass=%08X funcOffset=%X children=%X next=%X propertyOffset=%X",
        (unsigned)captureClass,(unsigned)targetClass,(unsigned)cls,g_funcOffset,g_upChildren,g_upNext,g_upOffset);
    if(!CodeContract("wrapper/Create",create,496,0x55FEB9FBD0D3F0A5ull)||
       !CodeContract("wrapper/SetView",setView,165,0x6AE16D2B13BDB57Cull)||
       !CodeContract("wrapper/SetCaptureParameters",setParams,455,0x3EE3F90D93363B69ull)||
       !CodeContract("wrapper/SetFrameRate",setRate,95,0x4E3E6769F20272E3ull)||
       !CodeContract("wrapper/AttachComponent",add,91,0xB40193EDB4C6A38Bull)||
       !CodeContract("wrapper/DetachComponent",remove,91,0x87A7C6584939B487ull))return false;
    construct=(Construct)Call("helper/construct",create,0x159,528,0x80DA10E4224515C4ull);
    init=(InitTarget)Call("helper/init",create,0x1dd,44,0xEC5B63A8BE57148Full);
    view=(SetView)Call("helper/view",setView,0x98,246,0x78C58D8F4194A021ull);
    rebuild=(Rebuild)Call("helper/rebuild",setParams,0x188,373,0x9D3D895A33EACBBDull);
    rate=(Rate)Call("helper/rate",setRate,0x52,31,0x67123D70BB9B9A26ull);
    attach=(Component)Call("helper/attach",add,0x4e,117,0xA43F290EA3F897E0ull);
    detach=(Component)Call("helper/detach",remove,0x4e,41,0x04E0A3B0BBD603DBull);
    const auto errorGlobal=PauseRttProbe::Pointer(create+0x137);
    errors=(void*)PauseRttProbe::Pointer(errorGlobal);
    if(!errors)Log("scope PiP: REFUSE stage=error-device global=%08X; no engine writes",(unsigned)errorGlobal);
    // These exact fields are independently read/written by the audited native
    // helpers. Refuse another engine layout before constructing anything.
    const bool fields=FieldContract("TextureTarget",captureClass,"ObjectProperty",0x80)&&
        FieldContract("FieldOfView",captureClass,"FloatProperty",0x84)&&
        FieldContract("NearPlane",captureClass,"FloatProperty",0x88)&&
        FieldContract("FarPlane",captureClass,"FloatProperty",0x8c)&&
        FieldContract("bUpdateMatrices",captureClass,"BoolProperty",0x90)&&
        FieldContract("SizeX",targetClass,"IntProperty",0xac)&&
        FieldContract("SizeY",targetClass,"IntProperty",0xb0);
    return fields&&construct&&init&&view&&rebuild&&rate&&attach&&detach&&errors;
}
static scopepip::Frame Snapshot(){
    scopepip::Frame frame;
    if(TryAcquireSRWLockShared(&g_nextVisualLock)){
        frame=g_scopePipFrame;ReleaseSRWLockShared(&g_nextVisualLock);
    }
    return frame;
}
static bool Current(const scopepip::Frame& f){
    return g_sniperPip&&g_sniperNearEyeScope&&VrModeOn()&&!CutsceneNativeCameraArmed()&&
        !Rd(g_gamePaused)&&!Rd(g_panelOpen)&&!Rd(g_weaponWheelOpen)&&!Rd(g_weaponHolstered)&&
        f.Current((uint32_t)g_playerPawn,(uint32_t)Rd(g_weaponObj),GetTickCount64());
}
// PiP deliberately does NOT set native ADS: that would alter the main camera,
// hand pose and full-screen postprocess. Native slow-time's energy tick has one
// IsAiming call which otherwise cancels it immediately. Substitute optical aim
// Only audited energy/spread call sites, local pawn and fresh optical frame.
using AimQuery=int (__thiscall*)(void*);
static AimQuery aimOriginal=nullptr;
static uintptr_t energyAimReturn=0,slowWeapon=0;
static uintptr_t spreadAimReturns[3]{};
using SlowAction=void (__thiscall*)(void*);
static SlowAction slowToggle=nullptr,slowStop=nullptr;
static bool slowFailed=false;
using AddSpread=int* (__thiscall*)(void*,int*,int,int,int);
static AddSpread spreadOriginal=nullptr;
struct ShotAxis {uintptr_t pawn=0,weapon=0;ULONGLONG tick=0;LONG sequence=0;V centre{},spread{};};
static thread_local ShotAxis shotAxis;
static V RotatorDirection(int pitch,int yaw){
    const float scale=6.28318530718f/65536.f,p=pitch*scale,y=yaw*scale,c=std::cos(p);
    return {c*std::cos(y),c*std::sin(y),std::sin(p)};
}
static int* __fastcall OpticalAddSpread(void* self,void*,int* output,int pitch,int yaw,int roll){
    shotAxis={};
    auto* result=spreadOriginal(self,output,pitch,yaw,roll);
    const auto frame=Snapshot();
    const bool scoped=g_sniperOpticalAccuracy&&Current(frame)&&(uintptr_t)self==frame.weapon;
    const bool ordinary=(g_weaponNativeAxis||g_twoHandSpreadPercent<100||
        g_vrValkyrieSpreadPercent<100||g_vrVolkSpreadPercent<100)&&VrModeOn()&&InGameplay()&&
        !Rd(g_inCinematic)&&!Rd(g_gamePaused)&&!Rd(g_weaponHolstered)&&Rd(g_handDevValid)&&
        (uintptr_t)self==(uintptr_t)(uint32_t)Rd(g_weaponObj);
    weaponmuzzle::Witness muzzle;
    if(ordinary&&TryAcquireSRWLockShared(&g_nextVisualLock)){
        muzzle=g_nextMuzzle;ReleaseSRWLockShared(&g_nextVisualLock);
    }
    const bool support=Rd(g_twoHandActive)!=0;
    const int gunVertices=(int)Rd(g_gunVerts);
    const bool currentMuzzle=muzzle.Current((uint32_t)g_playerPawn,(uint32_t)(uintptr_t)self,GetTickCount64());
    const bool profileMatch=currentMuzzle&&
        ((gunVertices==11387&&muzzle.kind==weaponreload::Kind::Valkyrie)||
         (gunVertices==8297&&muzzle.kind==weaponreload::Kind::Volk));
    const int percent=profileMatch?weaponaccuracy::EffectivePercent(gunVertices,muzzle.kind,currentMuzzle,
        support,g_twoHandSpreadPercent,g_vrValkyrieSpreadPercent,g_vrVolkSpreadPercent):
        (support?g_twoHandSpreadPercent:100); // preserve the accepted pre-existing supported path
    if(Rd(g_fireDepth)>0&&ordinary&&result==output&&output&&
       PauseRttProbe::SafeRange((uintptr_t)output,12)&&
       twohandspread::Apply(output,pitch,yaw,percent,true,true)){
        static unsigned notes=0;if(notes++<8)Log("VR spread: native result reduced to %d%% weapon=%p seq=%ld profile=%d support=%d; centre/recoil retained",
            percent,self,Rd(g_fireSeq),profileMatch?gunVertices:0,support?1:0);
    }
    if(g_weaponMeasuredAim&&Rd(g_fireDepth)>0&&(scoped||ordinary)&&result==output&&output&&
       PauseRttProbe::SafeRange((uintptr_t)output,12)){
        shotAxis={g_playerPawn,(uintptr_t)self,GetTickCount64(),Rd(g_fireSeq),
            RotatorDirection(pitch,yaw),RotatorDirection(output[0],output[1])};
    }
    return result;
}
static bool BindSpread(uintptr_t weapon){
    if(spreadOriginal)return true;
    const auto method=PauseRttProbe::Pointer(PauseRttProbe::Pointer(weapon)+0x428);
    // Full native body: hidden result pointer, three FRotator ints, ret 0x10.
    if(!CodeContract("spread/AddSpread",method,536,0x90E1F845D48FC976ull))return false;
    if(MH_CreateHook((void*)method,(void*)&OpticalAddSpread,(void**)&spreadOriginal)!=MH_OK){
        spreadOriginal=nullptr;return false;}
    if(MH_EnableHook((void*)method)!=MH_OK){
        MH_RemoveHook((void*)method);spreadOriginal=nullptr;return false;}
    Log("scope ballistics: exact native pre/post spread axis capture ready; no spread writes");
    return true;
}
static int __fastcall OpticalAimQuery(void* self,void*){
    const auto caller=(uintptr_t)_ReturnAddress();
    if(g_sniperOpticalAccuracy&&scopepip::SpreadAimAllowed(true,Current(Snapshot()),
       caller,spreadAimReturns,(uintptr_t)self,g_playerPawn)){
        static unsigned receipts=0;if(receipts++<8)
            Log("scope ballistics: native scoped-spread aim query accepted; native min/max/movement, no camera ADS");
        return 1;
    }
    if(caller==energyAimReturn&&(uintptr_t)self==g_playerPawn&&g_sniperOpticalSlowTime&&
       scopepip::SlowAimAllowed(caller,energyAimReturn,(uintptr_t)self,g_playerPawn,Current(Snapshot()))){
        static unsigned receipts=0;if(receipts++<4)Log("scope slow: native energy aim gate accepted; camera ADS unchanged");
        return 1;
    }
    return aimOriginal?aimOriginal(self):0;
}
static bool BindSlow(uintptr_t pawn,uintptr_t weapon){
    if(aimOriginal)return !g_sniperOpticalAccuracy||BindSpread(weapon);
    const auto wrapper=NativeRegistry::Find("intARvPlayerPawnexecIsAiming");
    if(!CodeContract("slow/IsAiming-wrapper",wrapper,58,0x580673B4EE303833ull))return false;
    const auto aim=PauseRttProbe::Pointer(PauseRttProbe::Pointer(pawn)+0xa98);
    const auto tick=PauseRttProbe::Pointer(PauseRttProbe::Pointer(weapon)+0x21c);
    const auto cls=PauseRttProbe::Pointer(weapon+OBJ_CLASS);
    if(!FieldContract("mSlowMoTimeDilation",cls,"FloatProperty",0x584)||
       !FieldContract("mCurrentSlowMoAmount",cls,"FloatProperty",0x59c))return false;
    if(!CodeContract("slow/IsAiming",aim,13,0x2C74B5D4D5842879ull)||
       !CodeContract("slow/sniper-tick",tick,357,0xD504DE25001FD2C5ull))return false;
    if(g_sniperOpticalAccuracy){
        const auto table=PauseRttProbe::Pointer(weapon);
        const auto minimum=PauseRttProbe::Pointer(table+0x4b4);
        const auto maximum=PauseRttProbe::Pointer(table+0x4b8);
        const auto movement=PauseRttProbe::Pointer(table+0x4bc);
        if(!CodeContract("spread/minimum",minimum,129,0x75C0B13EC73BB9E9ull)||
           !CodeContract("spread/maximum",maximum,129,0x272F8E27E1E67C39ull)||
           !CodeContract("spread/movement",movement,98,0x0BBDCA92A52B52AFull)||
           !FieldContract("mSpreadMin",cls,"FloatProperty",0x450)||
           !FieldContract("mSpreadMax",cls,"FloatProperty",0x454)||
           !FieldContract("mSpreadMovement",cls,"FloatProperty",0x458)||
           !FieldContract("mIronsightSpreadMin",cls,"FloatProperty",0x414)||
           !FieldContract("mIronsightSpreadMax",cls,"FloatProperty",0x410)||
           !FieldContract("mIronsightSpreadMovement",cls,"FloatProperty",0x418))return false;
        spreadAimReturns[0]=minimum+0x36;
        spreadAimReturns[1]=maximum+0x36;
        spreadAimReturns[2]=movement+0x33;
        Log("scope ballistics: audited native aim callers ready; hip min/max=%.6f/%.6f aimed=%.6f/%.6f movement=%.6f",
            *(float*)(weapon+0x450),*(float*)(weapon+0x454),
            *(float*)(weapon+0x414),*(float*)(weapon+0x410),*(float*)(weapon+0x418));
    }
    const auto energy=Call("slow/energy",tick,0x147,512,0xF11C945D7721EDC5ull);
    // Audited adjacent native toggle follows the full tick + alignment padding.
    // Adjacency alone authorizes NOTHING: whole-body and both tail-jump targets
    // must match before a call or hook. No executable VA is embedded here.
    const auto toggle=tick+0x170;
    if(!energy||!CodeContract("slow/toggle",toggle,87,0x05EFAB4C30C5C71Dull)||
       !PauseRttProbe::SafeRange(toggle+0x4a,5)||*(const unsigned char*)(toggle+0x4a)!=0xe9)return false;
    const auto stop=toggle+0x4f+*(const int32_t*)(toggle+0x4b);
    if(!CodeContract("slow/stop",stop,128,0x4DB623B969EF1939ull))return false;
    auto status=MH_CreateHook((void*)aim,(void*)&OpticalAimQuery,(void**)&aimOriginal);
    if(status!=MH_OK){aimOriginal=nullptr;return false;}
    energyAimReturn=energy+0xd4;
    status=MH_EnableHook((void*)aim);
    if(status!=MH_OK){MH_RemoveHook((void*)aim);aimOriginal=nullptr;energyAimReturn=0;return false;}
    slowToggle=(SlowAction)toggle;slowStop=(SlowAction)stop;
    Log("scope slow: fingerprinted native toggle/energy/stop bridge ready; L3 only, no native ADS");
    return !g_sniperOpticalAccuracy||BindSpread(weapon);
}
static void SlowTick(const scopepip::Frame& frame,bool active){
    static LONG consumed=0;const LONG presses=Rd(g_scopeSlowPresses);
    const bool eligible=(g_sniperOpticalSlowTime||g_sniperOpticalAccuracy)&&active&&IsLive(frame.weapon)&&
        PauseRttProbe::Pointer(frame.weapon+0x98)==g_playerPawn;
    if(slowWeapon&&(!eligible||slowWeapon!=frame.weapon)){
        if(slowStop&&IsLive(slowWeapon)&&PauseRttProbe::Pointer(slowWeapon+0x98)==g_playerPawn)
            slowStop((void*)slowWeapon);
        slowWeapon=0;
    }
    if(!eligible||slowFailed){consumed=presses;return;}
    if(!BindSlow(g_playerPawn,frame.weapon)){slowFailed=true;consumed=presses;return;}
    if(!g_sniperOpticalSlowTime){consumed=presses;return;}
    slowWeapon=frame.weapon;
    if(presses!=consumed&&g_sniperOpticalSlowTime){consumed=presses;slowToggle((void*)slowWeapon);
        static unsigned receipts=0;if(receipts++<24)
            Log("scope slow: L3 native toggle submitted energy=%.3f dilation=%.3f [native limits, not visual proof]",
                *(const float*)(slowWeapon+0x59c),*(const float*)(slowWeapon+0x584));}
}
static bool Relative(V p,V& out){return scopepip::Relative(p,
    {g_centrePos[0],g_centrePos[1],g_centrePos[2]},g_centreYaw,
    g_baseYaw*(6.2831853f/65536.f),MetresToUU(),out);}
static bool ObjectsCurrent(){
    const auto rt=(uintptr_t)(uint32_t)Rd(target);
    return capture&&rt&&IsLive(capture)&&IsLive(rt)&&
        PauseRttProbe::Pointer(capture+OBJ_CLASS)==captureClass&&
        PauseRttProbe::Pointer(capture+OBJ_OUTER)==owner&&
        PauseRttProbe::Pointer(capture+0x80)==rt&&
        PauseRttProbe::Pointer(rt+OBJ_CLASS)==targetClass&&
        PauseRttProbe::Pointer(rt+OBJ_OUTER)==owner;
}
static void Tick(){
    if(!g_sniperPip&&!attached)return;
    if(failed||g_funcOffset<0||g_upChildren<=0||g_upNext<=0)return;
    static thread_local bool running=false;
    if(running)return;
    struct Guard{bool& value;Guard(bool& v):value(v){value=true;}~Guard(){value=false;}}guard(running);
    const auto pawn=g_playerPawn;
    if(!g_sniperPip){
        // A live config disable must stop native background work as well as
        // hiding the aperture. Never detach from the render/config thread.
        if(attached&&owner==pawn&&pawn&&IsLive(pawn)&&ObjectsCurrent())
            detach((void*)pawn,(void*)capture);
        attached=false;capture=0;InterlockedExchange(&target,0);return;
    }
    const auto frame=Snapshot();const bool active=Current(frame)&&!Rd(presentationFault);
    SlowTick(frame,active);
    static bool entered=false;
    if(active&&!entered){entered=true;Log("scope PiP: engine PlayerTick reached with fresh measured lens");}
    // Only called after the local controller's PlayerTick, on the engine thread.
    if(!pawn||!IsLive(pawn))return;
    // ABI preflight is read-only and does not need the lens held at an eye.
    // It must finish before construction, and runs once (classes spread over
    // <=1024 objects/tick). A mismatch is now observable without another visual
    // scope attempt, not hidden behind the proximity gate.
    if(!bound){
        if(!FindClasses())return;
        if(!Bind(pawn)){failed=true;Log("scope PiP: native ABI/layout refused; no engine objects created");return;}
        bound=true;Log("scope PiP: complete native ABI/layout preflight ready; no engine objects created yet");
    }
    if(owner&&owner!=pawn){
        // Old pawn and components are owned by UE3 GC. Do not call into a
        // potentially destroyed level. No raw pointer is re-used for the new pawn.
        owner=capture=0;attached=false;allocations=0;InterlockedExchange(&target,0);
    }
    if(capture&&!ObjectsCurrent()){
        // Detached components may be collected. Do not call a stale UObject;
        // recreate under the new live owner, with a strict allocation budget.
        capture=0;attached=false;InterlockedExchange(&target,0);
        Log("scope PiP: collected component invalidated; bounded recreation only");
    }
    if(!active){
        if(attached&&owner==pawn&&ObjectsCurrent())detach((void*)pawn,(void*)capture);
        attached=false;return;
    }
    if(!capture){
        if(allocations>=3){failed=true;Log("scope PiP: allocation budget exhausted; capture disabled");return;}
        // StaticConstructObject's full argument order is proven in execNew and
        // TextureRenderTarget2D.Create. Pawn.Components and TextureTarget own the
        // strong references; no borrowed RT, root-set flag or raw asset writes.
        ++allocations;
        const auto rt=(uintptr_t)construct((void*)targetClass,(void*)pawn,0,0,0,0,nullptr,errors,0,0);
        const auto component=(uintptr_t)construct((void*)captureClass,(void*)pawn,0,0,0,0,nullptr,errors,0,0);
        if(!rt||!component||PauseRttProbe::Pointer(rt+OBJ_CLASS)!=targetClass||
           PauseRttProbe::Pointer(component+OBJ_CLASS)!=captureClass){failed=true;Log("scope PiP: object construction refused");return;}
        if(!BindRender(component)){failed=true;Log("scope PiP: REFUSE capture isolation; component never attached");return;}
        init((void*)rt,512,512,2); // native PF_A8R8G8B8, same format as native Create
        *(uintptr_t*)(component+0x80)=rt;
        *(float*)(component+0x84)=20.f;
        *(float*)(component+0x88)=1.f;
        *(float*)(component+0x8c)=0.f;
        *(uint32_t*)(component+0x90)|=1;rebuild((void*)component);
        // Native SetView owns the view; never rebuild it from the pawn transform.
        *(uint32_t*)(component+0x90)&=~1u;
        rate((void*)component,0.f);
        owner=pawn;capture=component;InterlockedExchange(&target,(LONG)rt);
        Log("scope PiP: own512 RTT/component constructed, 20deg optical view; no pixel proof yet");
    }
    if(!ObjectsCurrent()){
        failed=true;InterlockedExchange(&target,0);Log("scope PiP: component lifetime refused");return;}
    V relative{},ahead{},up{},right{},vertical{};
    if(!Relative(frame.lens,relative)||!Relative(frame.lens+frame.forward,ahead)||
       !Relative(frame.lens+frame.up,up)||!scopepip::Basis(ahead-relative,up-relative,right,vertical))return;
    int pitch=0,yaw=0,roll=0;
    if(!scopepip::Angles(ahead-relative,up-relative,pitch,yaw,roll))return;
    if(g_scopeMeasuredPose){
        static ULONGLONG nextReceipt=0;static unsigned receipts=0;
        const auto now=GetTickCount64();
        if(receipts<16&&now>=nextReceipt){
            nextReceipt=now+1000;++receipts;
            V direction{},barrel{};weaponmuzzle::Witness muzzle;
            if(TryAcquireSRWLockShared(&g_nextVisualLock)){
                muzzle=g_nextMuzzle;ReleaseSRWLockShared(&g_nextVisualLock);
            }
            const bool paired=scopepip::Unit(ahead-relative,direction)&&
                muzzle.Direction(frame.pawn,frame.weapon,now,barrel);
            Log("scope measured pose: paired=%d barrelDot=%.7f lens=(%.3f,%.3f,%.3f) forward=(%.6f,%.6f,%.6f) age=%llu ms twoHand=%ld [input geometry, not pixel alignment]",
                paired?1:0,paired?hipknife::Dot(direction,barrel):-2.f,
                relative.x,relative.y,relative.z,direction.x,direction.y,direction.z,
                now>=frame.tick?now-frame.tick:0,Rd(g_twoHandActive));
        }
    }
    if(g_scopeOpticalCalibration){
        OpticalSample optical;
        if(TryAcquireSRWLockShared(&viewLock)){optical=opticalSample;ReleaseSRWLockShared(&viewLock);}
        const auto now=GetTickCount64();
        if(optical.target==(uintptr_t)(uint32_t)Rd(target)&&now>=optical.tick&&now-optical.tick<=150&&
           optical.degrees>=.1f&&optical.degrees<=80.f&&
           std::fabs(*(float*)(capture+0x84)-optical.degrees)>.02f){
            *(float*)(capture+0x84)=optical.degrees;
            *(uint32_t*)(capture+0x90)|=1;rebuild((void*)capture);
            *(uint32_t*)(capture+0x90)&=~1u;
            const float expected=1.f/(std::tan(optical.degrees*.008726646192f)*.75f);
            const float actual=*(float*)(capture+0xe0);
            if(!std::isfinite(actual)||std::fabs(actual-expected)>expected*.005f){
                failed=true;InterlockedExchange(&presentationFault,1);
                Log("scope PiP: REFUSE optical projection expected=%.6f actual=%.6f",expected,actual);
                if(attached){detach((void*)pawn,(void*)capture);attached=false;}return;
            }
            static unsigned optics=0;if(optics++<12)
                Log("scope PiP: calibrated4x FOV=%.4f projectionX=%.6f capture<=45.5Hz; HMD unchanged",optical.degrees,*(float*)(capture+0xe0));
        }
    }
    if(!attached){attach((void*)pawn,(void*)capture);attached=true;}
    view((void*)capture,{g_camPos[0]+relative.x,g_camPos[1]+relative.y,g_camPos[2]+relative.z},pitch,yaw,roll);
    if(PauseRttProbe::SafeRange(capture+0xa0,128)){
        const auto* matrix=(const float*)(capture+0xa0);bool finite=true;
        for(unsigned i=0;i<32;++i)finite=finite&&std::isfinite(matrix[i]);
        if(finite){AcquireSRWLockExclusive(&viewLock);
            viewSample.target=(uintptr_t)(uint32_t)Rd(target);viewSample.tick=GetTickCount64();
            ++viewSample.sequence;std::memcpy(viewSample.matrix,matrix,64);
            std::memcpy(viewSample.projection,matrix+16,64);
            ReleaseSRWLockExclusive(&viewLock);}
    }
    if(notes++<3)Log("scope PiP: native view updated on engine PlayerTick; optical origin=(%.3f,%.3f,%.3f)",relative.x,relative.y,relative.z);
}
static void ResetGpu(){
    completion.Reset();completedTarget=0;completedMs=0;
    g_scopeAperture={};
    InterlockedExchange(&resolvedTarget,0);
    if(state){state->Release();state=nullptr;}
}
static void BindSurface(IDirect3DSurface9* surface){
    if(!g_sniperPip)return;
    const auto rt=(uintptr_t)(uint32_t)Rd(target);
    const auto resource=rt?PauseRttProbe::Pointer(rt+0xa4):0;
    const auto own=rt?PauseRttProbe::NativeSurface(rt,resource):nullptr;
    if(completedTarget&&completedTarget!=rt){completion.Reset();completedTarget=0;completedMs=0;}
    const auto epoch=completion.epoch;
    completion.Bind(own&&surface==own);
    if(completion.epoch!=epoch){
        completedTarget=rt;completedMs=GetTickCount64();
        static unsigned receipts=0;
        if(receipts++<3)Log("scope PiP: own RTT draw epoch=%u completed binding [not pixel-content proof]",completion.epoch);
    }
}
static void Receipt(HRESULT hr){
    if(NativeScopeRendering())++g_scopeNativeDraws;
    if(g_sniperPip)completion.Draw(SUCCEEDED(hr));
}
struct Vertex{float x,y,z,rhw;DWORD colour;float u,v;};
static void PresentationRefused(unsigned stage,HRESULT hr=S_OK){
    // One receipt per exact stage, not per frame/draw. HRESULT distinguishes a
    // detached render surface from an actual texture and state/target failures.
    static unsigned seen=0;
    if(stage<32&&!(seen&(1u<<stage))){seen|=1u<<stage;
        Log("scope PiP: aperture stopped stage=%u hr=%08X [no presentation proof]",stage,(unsigned)hr);}
}
static void ContentReceipt(IDirect3DDevice9* dev,IDirect3DSurface9* surface){
    static unsigned attempts=0;
    if(attempts>=3)return;
    ++attempts;
    D3DSURFACE_DESC desc{};IDirect3DSurface9* cpu=nullptr;
    if(FAILED(surface->GetDesc(&desc))||desc.Width!=512||desc.Height!=512||
       (desc.Format!=D3DFMT_A8R8G8B8&&desc.Format!=D3DFMT_X8R8G8B8)||
       FAILED(dev->CreateOffscreenPlainSurface(512,512,desc.Format,D3DPOOL_SYSTEMMEM,&cpu,nullptr))){
        Log("scope PiP: bounded RGB readback unavailable");return;
    }
    const auto copy=dev->GetRenderTargetData(surface,cpu);
    D3DLOCKED_RECT lock{};
    if(SUCCEEDED(copy)&&SUCCEEDED(cpu->LockRect(&lock,nullptr,D3DLOCK_READONLY))){
        unsigned nonblack=0,alpha=0,minimum=255,maximum=0;
        for(unsigned y=0;y<512;y+=4){
            const auto row=(const uint32_t*)((const unsigned char*)lock.pBits+y*lock.Pitch);
            for(unsigned x=0;x<512;x+=4){const auto p=row[x];
                nonblack+=(p&0xffffffu)!=0;alpha+=(p>>24)!=0;
                for(unsigned shift=0;shift<24;shift+=8){const auto c=(p>>shift)&255u;
                    if(c<minimum)minimum=c;if(c>maximum)maximum=c;}
            }
        }
        cpu->UnlockRect();
        Log("scope PiP: RGB sample nonblack=%u/16384 range=%u..%u alphaNonzero=%u [content only, not alignment]",nonblack,minimum,maximum,alpha);
    }else Log("scope PiP: RGB readback failed hr=%08X",(unsigned)copy);
    cpu->Release();
}
static void Draw(IDirect3DDevice9* dev){
    if(!g_sniperPip||!shadowNativeUP)return;
    const auto f=Snapshot();const auto now=GetTickCount64();
    const auto rt=(uintptr_t)(uint32_t)Rd(target);
    if(!Current(f)||!rt||rt!=completedTarget||rt!=(uintptr_t)(uint32_t)Rd(resolvedTarget)||
       !completion.epoch||now<completedMs||now-completedMs>150)return;
    struct PresentationAttempt {
        bool shown=false;
        ~PresentationAttempt(){
            if(!presentationBudget.Observe(true,shown)&&!InterlockedExchange(&presentationFault,1))
                Log("scope PiP: 30 completed frames without aperture; native capture disabled on engine tick (GPU work bounded)");
        }
    } attempt;
    const auto resource=PauseRttProbe::Pointer(rt+0xa4);
    auto* surface=PauseRttProbe::NativeSurface(rt,resource);if(!surface){PresentationRefused(1);return;}
    const auto resourceMethod=PauseRttProbe::Pointer(PauseRttProbe::Pointer(resource)+4);
    static uintptr_t validatedResourceMethod=0;
    if(resourceMethod!=validatedResourceMethod){
        if(!CodeContract("resource/separate-render-resolve",resourceMethod,645,0x65B4A8A44C0157E5ull)){
            PresentationRefused(16);return;
        }
        validatedResourceMethod=resourceMethod;
    }
    // InitDynamicRHI owns a render surface AND a resolved texture. Surface
    // GetContainer is not guaranteed for a separately allocated targetable
    // surface. Validate exact native wrappers/COM identities BEFORE any call
    // through the texture pointer; dimensions alone never authorize it.
    const auto textureRhi=PauseRttProbe::Pointer(resource+0x64);
    auto* texture=textureRhi>=12 ? (IDirect3DTexture9*)PauseRttProbe::Pointer(textureRhi-4):nullptr;
    if(!PauseRttProbe::ExactTexture(rt,resource,texture,surface)){PresentationRefused(3);return;}
    texture->AddRef();
    // The last UE3 target is NOT necessarily the submitted eye backbuffer.
    // D3DSBT_ALL does not capture RT/DS. Use the same tested explicit target
    // guard as the custom hands, and bypass CPU-side observation hooks.
    struct TextureRef {IDirect3DTexture9* value;~TextureRef(){value->Release();}} textureRef{texture};
    D3DSURFACE_DESC sampled{};
    const auto textureHr=texture->GetLevelDesc(0,&sampled);
    if(FAILED(textureHr)||sampled.Width!=512||sampled.Height!=512){PresentationRefused(2,textureHr);return;}
    // Bounded readback now measures the SAMPLEABLE resolved texture, not just
    // content in a potentially separate drawing surface.
    IDirect3DSurface9* sampledSurface=nullptr;
    if(SUCCEEDED(texture->GetSurfaceLevel(0,&sampledSurface))&&sampledSurface){
        ContentReceipt(dev,sampledSurface);sampledSurface->Release();
    }
    IDirect3DSurface9* back=nullptr;
    const auto backHr=dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back);
    if(FAILED(backHr)||!back){PresentationRefused(4,backHr);return;}
    struct SurfaceRef {IDirect3DSurface9* value;~SurfaceRef(){value->Release();}} backRef{back};
    D3DSURFACE_DESC desc{};
    if(FAILED(back->GetDesc(&desc))||desc.Width<4||(desc.Width&1)||!desc.Height){PresentationRefused(5);return;}
    const auto aperture=g_scopeAperture;
    if(!aperture.Current(f.pawn,f.weapon,now,desc.Width,desc.Height)){
        PresentationRefused(17);return;
    }
    if(g_scopeOpticalCalibration&&f.eye>=0&&f.eye<2&&aperture.eye[f.eye]){
        float left=aperture.points[f.eye][1].x,right=left;
        for(unsigned i=2;i<66;++i){left=std::min(left,aperture.points[f.eye][i].x);right=std::max(right,aperture.points[f.eye][i].x);}
        float degrees=0;
        if(scopepip::OpticalFov(right-left,desc.Width*.5f,g_targetTanX,4.f,degrees)){
            AcquireSRWLockExclusive(&viewLock);opticalSample={rt,now,degrees};ReleaseSRWLockExclusive(&viewLock);
        }
    }
    const gloves::Calls calls{shadowNativeIndexedUP,g_origSetRenderTarget,g_origSetVS,
        shadowOriginalPS,g_origSetTexture,shadowNativeClear,g_origSetViewport};
    if(!calls.Valid()){PresentationRefused(6);return;}
    if(!state&&FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state))){PresentationRefused(7);return;}
    if(!state){PresentationRefused(8);return;}
    gloves::StateGuard<> guard(dev,state,calls);if(!guard.valid){PresentationRefused(9);return;}
    if(FAILED(dev->SetDepthStencilSurface(nullptr))){PresentationRefused(10);return;}
    for(unsigned i=1;i<guard.count;++i)if(FAILED(calls.target(dev,i,nullptr))){PresentationRefused(11);return;}
    if(FAILED(calls.target(dev,0,back))){PresentationRefused(12);return;}
    const D3DVIEWPORT9 vp{0,0,desc.Width,desc.Height,0.f,1.f};
    if(FAILED(calls.viewport(dev,&vp))){PresentationRefused(13);return;}
    V right{},vertical{};
    if(scopepip::Basis(f.forward,f.up,right,vertical)){
        if(FAILED(calls.vs(dev,nullptr))||FAILED(calls.ps(dev,nullptr))||
           FAILED(dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1))||
           FAILED(calls.texture(dev,0,texture))){PresentationRefused(14);return;}
        dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
        dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);dev->SetRenderState(D3DRS_STENCILENABLE,FALSE);
        dev->SetRenderState(D3DRS_FOGENABLE,FALSE);dev->SetRenderState(D3DRS_CLIPPLANEENABLE,0);
        dev->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);dev->SetRenderState(D3DRS_COLORWRITEENABLE,0xf);
        dev->SetRenderState(D3DRS_LIGHTING,FALSE);dev->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
        dev->SetRenderState(D3DRS_VERTEXBLEND,D3DVBF_DISABLE);
        dev->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE,FALSE);
        dev->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);
        dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
        dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
        dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
        dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
        dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
        dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
        dev->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);dev->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
        dev->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);
        for(unsigned eye=0;eye<2;++eye){
            RECT clip{(LONG)(vp.X+eye*(vp.Width/2)),(LONG)vp.Y,(LONG)(vp.X+(eye+1)*(vp.Width/2)),(LONG)(vp.Y+vp.Height)};
            dev->SetScissorRect(&clip);
            Vertex vertices[66]{};const bool visible=aperture.eye[eye];
            for(unsigned i=0;i<66;++i){
                const float a=(i?i-1:0)*(6.2831853f/64.f);
                const float x=i?std::cos(a):0.f,y=i?std::sin(a):0.f;
                const auto pixel=aperture.points[eye][i];
                vertices[i]={pixel.x,pixel.y,0,pixel.rhw,0xffffffff,.5f+x*.5f,.5f-y*.5f};
            }
            if(visible){
                const bool opticalEye=int(eye)==f.eye;
                dev->SetTextureStageState(0,D3DTSS_COLORARG1,opticalEye?D3DTA_TEXTURE:D3DTA_DIFFUSE);
                if(!opticalEye)for(auto& v:vertices)v.colour=0xff000000;
                const auto hr=shadowNativeUP(dev,D3DPT_TRIANGLEFAN,64,vertices,sizeof(Vertex));
                if(opticalEye&&SUCCEEDED(hr))attempt.shown=true;
                static unsigned receipts=0;
                if(opticalEye&&receipts++<3)Log("scope PiP: aperture draw eye=%u epoch=%u hr=%08X back=%ux%u centre=(%.1f,%.1f) native lens draw projection [PSVR2 verification required]",eye,completion.epoch,(unsigned)hr,desc.Width,desc.Height,vertices[0].x,vertices[0].y);
                if(opticalEye){
                    // Scope-local reticle: projected through the SAME aperture,
                    // never a fullscreen/native ADS crosshair.
                    Vertex lines[4]{vertices[33],vertices[1],vertices[49],vertices[17]};
                    for(auto& v:lines){v.x=vertices[0].x+(v.x-vertices[0].x)*.85f;
                        v.y=vertices[0].y+(v.y-vertices[0].y)*.85f;v.colour=0xff101010;}
                    dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
                    shadowNativeUP(dev,D3DPT_LINELIST,2,lines,sizeof(Vertex));
                }
            }else {static unsigned refused=0;if(refused++<3)Log("scope PiP: aperture refused tracking projection eye=%u haveEyes=%d",eye,g_haveEyes?1:0);}
        }
    }else PresentationRefused(15);
    if(!guard.Restore()){
        g_sniperPip=false; // Tick detaches on its engine thread, not here.
        Log("scope PiP: target/state restore FAILED; optical capture disabled");
    }
}
}
void NativeScopeTick(){
    NativeRifleFx::MaintainTick();NativeRifleFx::ObserveTick();NativeScopePip::Tick();
    if((g_weaponNativeAxis||g_twoHandSpreadPercent<100||g_vrValkyrieSpreadPercent<100||
        g_vrVolkSpreadPercent<100)&&VrModeOn()&&InGameplay()&&!Rd(g_inCinematic)){
        // Audited common RvWeaponShared method, not a guessed subclass offset.
        static ULONGLONG lastAttempt=0;const auto now=GetTickCount64();
        if(!NativeScopePip::spreadOriginal&&now-lastAttempt>=2000){
            lastAttempt=now;const auto weapon=(uintptr_t)(uint32_t)Rd(g_weaponObj);
            if(weapon&&PauseRttProbe::SafeRange(weapon,4))NativeScopePip::BindSpread(weapon);
        }
    }
}
bool NativeScopeAlignShot(float* start,float* end){
    const auto& shot=NativeScopePip::shotAxis;
    const auto now=GetTickCount64();
    const bool scoped=g_sniperOpticalAccuracy&&NativeScopePip::Current(NativeScopePip::Snapshot());
    const bool ordinary=g_weaponNativeAxis&&VrModeOn()&&InGameplay()&&
        !Rd(g_inCinematic)&&!Rd(g_gamePaused)&&!Rd(g_weaponHolstered)&&Rd(g_handDevValid);
    if(!(scoped||ordinary)||!g_weaponMeasuredAim||!shot.weapon||
       shot.weapon!=(uintptr_t)(uint32_t)Rd(g_weaponObj)||shot.pawn!=g_playerPawn||
       shot.sequence!=Rd(g_fireSeq)||now<shot.tick||now-shot.tick>50)return false;
    hipknife::V ray{};
    if(!scopepip::Unit({end[0]-start[0],end[1]-start[1],end[2]-start[2]},ray)||
       hipknife::Dot(ray,shot.spread)<.999999f)return false;
    float measured[3];const bool meshAxis=GunAimDir(measured);
    // Centurion has no next-weapon mesh witness. Its existing controller axis
    // remains the target; never mislabel that fallback as measured geometry.
    if(!meshAxis&&scoped)return false;
    const float centre[]{shot.centre.x,shot.centre.y,shot.centre.z};
    const bool aligned=weapontrace::Align(start,end,centre,measured);
    static unsigned receipts=0;if(aligned&&receipts++<8)
        Log("weapon native axis: matched post-spread ray; cone preserved seq=%ld target=%s scoped=%d",shot.sequence,meshAxis?"measured-barrel":"controller-axis",scoped?1:0);
    return aligned;
}
void NativeScopeShotReceipt(const float* start,const float* end,const float* rawDirection){
    // Same collision callback, not independent pose receipts from other frames.
    // Read only; bounded to eight shots. No projection convention assumed here.
    static unsigned receipts=0;
    if(receipts>=8||!g_sniperOpticalAccuracy||!start||!end||!rawDirection)return;
    const auto frame=NativeScopePip::Snapshot();
    if(!NativeScopePip::Current(frame))return;
    NativeScopePip::ViewSample rendered;
    if(!TryAcquireSRWLockShared(&NativeScopePip::viewLock))return;
    rendered=NativeScopePip::renderedViewSample;
    ReleaseSRWLockShared(&NativeScopePip::viewLock);
    const auto now=GetTickCount64();
    if(!rendered.sequence||now<rendered.tick||now-rendered.tick>150||
       rendered.target!=(uintptr_t)(uint32_t)Rd(NativeScopePip::target))return;
    ++receipts;
    float current=0,base=0;
    const auto weapon=(uintptr_t)(uint32_t)Rd(g_weaponObj);
    const bool haveCurrent=NativeRifleFx::FloatReceipt(weapon,"mSpreadCurrentTotal",current);
    const bool haveBase=NativeRifleFx::FloatReceipt(weapon,"mSpreadBase",base);
    Log("scope paired shot: n=%u seq=%u ageMs=%llu headUU=%d/%d raw=(%.7f,%.7f,%.7f) start=(%.7f,%.7f,%.7f) end=(%.7f,%.7f,%.7f) spreadValid=%d/%d current=%.7f base=%.7f [collision and completed capture, read-only]",
        receipts,rendered.sequence,now-rendered.tick,int(g_wantYaw),int(g_wantPitch),
        rawDirection[0],rawDirection[1],rawDirection[2],start[0],start[1],start[2],end[0],end[1],end[2],
        haveCurrent?1:0,haveBase?1:0,current,base);
    const auto* m=rendered.matrix;const auto* p=rendered.projection;
    Log("scope paired view: n=%u matrix=[%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g] projection=[%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g]",
        receipts,m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],m[8],m[9],m[10],m[11],m[12],m[13],m[14],m[15],
        p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],p[10],p[11],p[12],p[13],p[14],p[15]);
}
void NativeScopeBind(IDirect3DSurface9* surface){NativeScopePip::BindSurface(surface);}
void NativeScopeReceipt(HRESULT hr){NativeScopePip::Receipt(hr);}
void DrawScopePip(IDirect3DDevice9* dev){NativeScopePip::Draw(dev);}
void ResetScopePip(){NativeScopePip::ResetGpu();}
