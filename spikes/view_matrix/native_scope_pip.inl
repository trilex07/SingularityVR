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
static unsigned allocations=0;
static unsigned notes=0;
static scopepip::Completion completion;
static IDirect3DStateBlock9* state=nullptr;
static uintptr_t completedTarget=0;
static ULONGLONG completedMs=0;

static uintptr_t Call(uintptr_t wrapper,unsigned offset,size_t bytes,unsigned long long hash){
    if(!PauseRttProbe::SafeRange(wrapper+offset,5)||*(const unsigned char*)(wrapper+offset)!=0xe8)return 0;
    const auto result=wrapper+offset+5+*(const int32_t*)(wrapper+offset+1);
    return NativeRifleFx::Code(result,bytes,hash)?result:0;
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
    const auto create=NativeRifleFx::Function(targetClass,"Create");
    const auto setView=NativeRifleFx::Function(captureClass,"SetView");
    const auto setParams=NativeRifleFx::Function(captureClass,"SetCaptureParameters");
    const auto setRate=NativeRifleFx::Function(captureClass,"SetFrameRate");
    const auto cls=PauseRttProbe::Pointer(pawn+OBJ_CLASS);
    const auto add=NativeRifleFx::Function(cls,"AttachComponent");
    const auto remove=NativeRifleFx::Function(cls,"DetachComponent");
    if(!NativeRifleFx::Code(create,496,0x55FEB9FBD0D3F0A5ull)||
       !NativeRifleFx::Code(setView,165,0x6AE16D2B13BDB57Cull)||
       !NativeRifleFx::Code(setParams,455,0x3EE3F90D93363B69ull)||
       !NativeRifleFx::Code(setRate,95,0x4E3E6769F20272E3ull)||
       !NativeRifleFx::Code(add,91,0xB40193EDB4C6A38Bull)||
       !NativeRifleFx::Code(remove,91,0x87A7C6584939B487ull))return false;
    construct=(Construct)Call(create,0x159,528,0x80DA10E4224515C4ull);
    init=(InitTarget)Call(create,0x1dd,44,0xEC5B63A8BE57148Full);
    view=(SetView)Call(setView,0x98,246,0x78C58D8F4194A021ull);
    rebuild=(Rebuild)Call(setParams,0x188,373,0x9D3D895A33EACBBDull);
    rate=(Rate)Call(setRate,0x52,31,0x67123D70BB9B9A26ull);
    attach=(Component)Call(add,0x4e,117,0xA43F290EA3F897E0ull);
    detach=(Component)Call(remove,0x4e,41,0x04E0A3B0BBD603DBull);
    const auto errorGlobal=PauseRttProbe::Pointer(create+0x137);
    errors=(void*)PauseRttProbe::Pointer(errorGlobal);
    // These exact fields are independently read/written by the audited native
    // helpers. Refuse another engine layout before constructing anything.
    const bool fields=PauseRttProbe::Property(captureClass,"TextureTarget","ObjectProperty")==0x80&&
        PauseRttProbe::Property(captureClass,"FieldOfView","FloatProperty")==0x84&&
        PauseRttProbe::Property(captureClass,"NearPlane","FloatProperty")==0x88&&
        PauseRttProbe::Property(captureClass,"FarPlane","FloatProperty")==0x8c&&
        PauseRttProbe::Property(captureClass,"bUpdateMatrices","BoolProperty")==0x90&&
        PauseRttProbe::Property(targetClass,"SizeX","IntProperty")==0xac&&
        PauseRttProbe::Property(targetClass,"SizeY","IntProperty")==0xb0;
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
    if(!g_sniperPip||failed||g_funcOffset<0||g_upChildren<=0||g_upNext<=0)return;
    static thread_local bool running=false;
    if(running)return;
    struct Guard{bool& value;Guard(bool& v):value(v){value=true;}~Guard(){value=false;}}guard(running);
    const auto pawn=g_playerPawn;
    const auto frame=Snapshot();const bool active=Current(frame);
    static bool entered=false;
    if(active&&!entered){entered=true;Log("scope PiP: engine PlayerTick reached with fresh measured lens");}
    // Only called after the local controller's PlayerTick, on the engine thread.
    if(!pawn||!IsLive(pawn))return;
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
    if(!FindClasses())return;
    if(!capture){
        if(allocations>=3){failed=true;Log("scope PiP: allocation budget exhausted; capture disabled");return;}
        if(!Bind(pawn)){failed=true;Log("scope PiP: native ABI/layout refused; no engine objects created");return;}
        // StaticConstructObject's full argument order is proven in execNew and
        // TextureRenderTarget2D.Create. Pawn.Components and TextureTarget own the
        // strong references; no borrowed RT, root-set flag or raw asset writes.
        ++allocations;
        const auto rt=(uintptr_t)construct((void*)targetClass,(void*)pawn,0,0,0,0,nullptr,errors,0,0);
        const auto component=(uintptr_t)construct((void*)captureClass,(void*)pawn,0,0,0,0,nullptr,errors,0,0);
        if(!rt||!component||PauseRttProbe::Pointer(rt+OBJ_CLASS)!=targetClass||
           PauseRttProbe::Pointer(component+OBJ_CLASS)!=captureClass){failed=true;Log("scope PiP: object construction refused");return;}
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
static void Receipt(HRESULT hr){if(g_sniperPip)completion.Draw(SUCCEEDED(hr));}
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
    if(!g_sniperPip||!g_origDrawPrimUP)return;
    const auto f=Snapshot();const auto now=GetTickCount64();
    const auto rt=(uintptr_t)(uint32_t)Rd(target);
    if(!Current(f)||!rt||rt!=completedTarget||!completion.epoch||now<completedMs||now-completedMs>150)return;
    const auto resource=PauseRttProbe::Pointer(rt+0xa4);
    auto* surface=PauseRttProbe::NativeSurface(rt,resource);if(!surface)return;
    ContentReceipt(dev,surface);
    IDirect3DTexture9* texture=nullptr;
    if(FAILED(surface->GetContainer(__uuidof(IDirect3DTexture9),(void**)&texture))||!texture)return;
    if(!PauseRttProbe::ExactTexture(rt,resource,texture,surface)){texture->Release();return;}
    if(!state&&FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state))){texture->Release();return;}
    if(!state||FAILED(state->Capture())){texture->Release();return;}
    D3DVIEWPORT9 vp{};const bool viewport=SUCCEEDED(dev->GetViewport(&vp));
    V right{},vertical{};
    if(viewport&&scopepip::Basis(f.forward,f.up,right,vertical)){
        dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);
        dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);dev->SetTexture(0,texture);
        dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
        dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);dev->SetRenderState(D3DRS_STENCILENABLE,FALSE);
        dev->SetRenderState(D3DRS_FOGENABLE,FALSE);dev->SetRenderState(D3DRS_CLIPPLANEENABLE,0);
        dev->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);dev->SetRenderState(D3DRS_COLORWRITEENABLE,0xf);
        dev->SetRenderState(D3DRS_LIGHTING,FALSE);dev->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
        dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
        dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
        dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
        dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
        dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
        dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
        dev->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);dev->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
        for(unsigned eye=0;eye<2;++eye){
            if(!g_vfxEyeMatValid[eye])continue;
            RECT clip{(LONG)(vp.X+eye*(vp.Width/2)),(LONG)vp.Y,(LONG)(vp.X+(eye+1)*(vp.Width/2)),(LONG)(vp.Y+vp.Height)};
            dev->SetScissorRect(&clip);
            Vertex vertices[66]{};bool visible=true;
            for(unsigned i=0;i<66;++i){
                const float a=(i?i-1:0)*(6.2831853f/64.f);
                const float x=i?std::cos(a):0.f,y=i?std::sin(a):0.f;
                V p{},t=f.lens+(right*x+vertical*y)*f.radius;
                if(!Relative(t,p)){visible=false;break;}
                float position[3]{p.x,p.y,p.z},c[4]{};
                WorldToClip(g_vfxEyeMat[eye],g_vfxEyeConv[eye],position,c);
                if(!std::isfinite(c[3])||c[3]<=.001f){visible=false;break;}
                vertices[i]={vp.X+(c[0]/c[3]*.5f+.5f)*vp.Width,
                    vp.Y+(-c[1]/c[3]*.5f+.5f)*vp.Height,0,1.f/c[3],0xffffffff,.5f+x*.5f,.5f-y*.5f};
            }
            if(visible){
                const bool opticalEye=int(eye)==f.eye;
                dev->SetTextureStageState(0,D3DTSS_COLORARG1,opticalEye?D3DTA_TEXTURE:D3DTA_DIFFUSE);
                if(!opticalEye)for(auto& v:vertices)v.colour=0xff000000;
                const auto hr=g_origDrawPrimUP(dev,D3DPT_TRIANGLEFAN,64,vertices,sizeof(Vertex));
                static unsigned receipts=0;
                if(opticalEye&&receipts++<3)Log("scope PiP: aperture draw eye=%u epoch=%u hr=%08X [PSVR2 verification required]",eye,completion.epoch,(unsigned)hr);
                if(opticalEye){
                    // Scope-local reticle: projected through the SAME aperture,
                    // never a fullscreen/native ADS crosshair.
                    Vertex lines[4]{vertices[33],vertices[1],vertices[49],vertices[17]};
                    for(auto& v:lines){v.x=vertices[0].x+(v.x-vertices[0].x)*.85f;
                        v.y=vertices[0].y+(v.y-vertices[0].y)*.85f;v.colour=0xff101010;}
                    dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
                    g_origDrawPrimUP(dev,D3DPT_LINELIST,2,lines,sizeof(Vertex));
                }
            }
        }
    }
    state->Apply();texture->Release();
}
}
void NativeScopeTick(){NativeScopePip::Tick();}
void NativeScopeBind(IDirect3DSurface9* surface){NativeScopePip::BindSurface(surface);}
void NativeScopeReceipt(HRESULT hr){NativeScopePip::Receipt(hr);}
void DrawScopePip(IDirect3DDevice9* dev){NativeScopePip::Draw(dev);}
void ResetScopePip(){NativeScopePip::ResetGpu();}
