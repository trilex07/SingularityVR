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
using RenderCapture=void (__thiscall*)(void*,void*);
static RenderCapture renderOriginal=nullptr;
static void __fastcall RenderHook(void* self,void*,void* family){
    if(!renderOriginal)return;
    const auto expected=(uintptr_t)(uint32_t)Rd(target);
    // Factory -> capture-info constructor proves +0x10 is the exact owned
    // TextureRenderTarget2D, NOT an inferred dimensions/format match.
    const auto actual=PauseRttProbe::Pointer((uintptr_t)self+0x10);
    scopepip::RenderLease lease(g_scopeNativeDepth,g_sniperPip,expected,actual);
    static unsigned count=0;
    const bool receipt=lease.own&&count<3;
    const auto draws=g_scopeNativeDraws,uploads=g_scopeNativeUploads;
    const auto begin=receipt?GetTickCount64():0;
    renderOriginal(self,family);
    // Capture changes engine GPU state legitimately. Never apply a state block
    // across it (that would disagree with UE3's RHI cache). Next main-view
    // constants must revalidate; optical matrices cannot survive as eye data.
    if(lease.own){InvalidateCamMats();g_camMatrixBound=false;}
    if(receipt){++count;
        Log("scope PiP: own native render isolation target=%08X depth=%u nativeDraws=%u rawUploads=%u elapsedMs=%llu [execution only, not alignment]",
            (unsigned)actual,g_scopeNativeDepth,g_scopeNativeDraws-draws,g_scopeNativeUploads-uploads,GetTickCount64()-begin);}
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
    const auto frame=Snapshot();const bool active=Current(frame);
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
    if(!attached){attach((void*)pawn,(void*)capture);attached=true;}
    view((void*)capture,{g_camPos[0]+relative.x,g_camPos[1]+relative.y,g_camPos[2]+relative.z},pitch,yaw,roll);
    if(notes++<3)Log("scope PiP: native view updated on engine PlayerTick; optical origin=(%.3f,%.3f,%.3f)",relative.x,relative.y,relative.z);
}
static void ResetGpu(){
    completion.Reset();completedTarget=0;completedMs=0;
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
    if(!Current(f)||!rt||rt!=completedTarget||!completion.epoch||now<completedMs||now-completedMs>150)return;
    const auto resource=PauseRttProbe::Pointer(rt+0xa4);
    auto* surface=PauseRttProbe::NativeSurface(rt,resource);if(!surface)return;
    ContentReceipt(dev,surface);
    IDirect3DTexture9* texture=nullptr;
    if(FAILED(surface->GetContainer(__uuidof(IDirect3DTexture9),(void**)&texture))||!texture)return;
    if(!PauseRttProbe::ExactTexture(rt,resource,texture,surface)){texture->Release();return;}
    // The last UE3 target is NOT necessarily the submitted eye backbuffer.
    // D3DSBT_ALL does not capture RT/DS. Use the same tested explicit target
    // guard as the custom hands, and bypass CPU-side observation hooks.
    struct TextureRef {IDirect3DTexture9* value;~TextureRef(){value->Release();}} textureRef{texture};
    IDirect3DSurface9* back=nullptr;
    if(FAILED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))||!back)return;
    struct SurfaceRef {IDirect3DSurface9* value;~SurfaceRef(){value->Release();}} backRef{back};
    D3DSURFACE_DESC desc{};
    if(FAILED(back->GetDesc(&desc))||desc.Width<4||(desc.Width&1)||!desc.Height)return;
    const gloves::Calls calls{shadowNativeIndexedUP,g_origSetRenderTarget,g_origSetVS,
        shadowOriginalPS,g_origSetTexture,shadowNativeClear,g_origSetViewport};
    if(!calls.Valid())return;
    if(!state&&FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state)))return;
    if(!state)return;
    gloves::StateGuard<> guard(dev,state,calls);if(!guard.valid)return;
    if(FAILED(dev->SetDepthStencilSurface(nullptr)))return;
    for(unsigned i=1;i<guard.count;++i)if(FAILED(calls.target(dev,i,nullptr)))return;
    if(FAILED(calls.target(dev,0,back)))return;
    const D3DVIEWPORT9 vp{0,0,desc.Width,desc.Height,0.f,1.f};
    if(FAILED(calls.viewport(dev,&vp)))return;
    V right{},vertical{};
    if(scopepip::Basis(f.forward,f.up,right,vertical)){
        if(FAILED(calls.vs(dev,nullptr))||FAILED(calls.ps(dev,nullptr))||
           FAILED(dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1))||
           FAILED(calls.texture(dev,0,texture)))return;
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
            Vertex vertices[66]{};bool visible=true;
            for(unsigned i=0;i<66;++i){
                const float a=(i?i-1:0)*(6.2831853f/64.f);
                const float x=i?std::cos(a):0.f,y=i?std::sin(a):0.f;
                const V t=f.lens+(right*x+vertical*y)*f.radius;
                uipointer::ScreenPoint p{};scopepip::Pixel pixel{};
                if(!VrUiProjectEye((int)eye,{t.x,t.y,t.z},p)||
                   !scopepip::EyePixel(eye,desc.Width,desc.Height,p,pixel)){visible=false;break;}
                vertices[i]={pixel.x,pixel.y,0,pixel.rhw,0xffffffff,.5f+x*.5f,.5f-y*.5f};
            }
            if(visible){
                const bool opticalEye=int(eye)==f.eye;
                dev->SetTextureStageState(0,D3DTSS_COLORARG1,opticalEye?D3DTA_TEXTURE:D3DTA_DIFFUSE);
                if(!opticalEye)for(auto& v:vertices)v.colour=0xff000000;
                const auto hr=shadowNativeUP(dev,D3DPT_TRIANGLEFAN,64,vertices,sizeof(Vertex));
                static unsigned receipts=0;
                if(opticalEye&&receipts++<3)Log("scope PiP: aperture draw eye=%u epoch=%u hr=%08X back=%ux%u centre=(%.1f,%.1f) tracking projection [PSVR2 verification required]",eye,completion.epoch,(unsigned)hr,desc.Width,desc.Height,vertices[0].x,vertices[0].y);
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
    }
    if(!guard.Restore()){
        g_sniperPip=false; // Tick detaches on its engine thread, not here.
        Log("scope PiP: target/state restore FAILED; optical capture disabled");
    }
}
}
void NativeScopeTick(){NativeRifleFx::MaintainTick();NativeRifleFx::ObserveTick();NativeScopePip::Tick();}
void NativeScopeBind(IDirect3DSurface9* surface){NativeScopePip::BindSurface(surface);}
void NativeScopeReceipt(HRESULT hr){NativeScopePip::Receipt(hr);}
void DrawScopePip(IDirect3DDevice9* dev){NativeScopePip::Draw(dev);}
void ResetScopePip(){NativeScopePip::ResetGpu();}
