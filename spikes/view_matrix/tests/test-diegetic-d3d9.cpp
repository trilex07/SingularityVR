// Standalone D3D9/Ex device, actual atlas and production draw helper. No game.
#define DIEGETIC_FIXTURE_INCLUDE_ONLY
#include "test-diegetic-projection.cpp"
#include <wincodec.h>
#include <vector>

static IDirect3DTexture9* Atlas(IDirect3DDevice9* dev) {
    IWICImagingFactory* factory=nullptr;
    assert(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))));
    IWICBitmapDecoder* decoder=nullptr;
    assert(SUCCEEDED(factory->CreateDecoderFromFilename(L"../../assets/processed/hud/diegetic/diegetic_hud_atlas.png",
        nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)));
    IWICBitmapFrameDecode* frame=nullptr;assert(SUCCEEDED(decoder->GetFrame(0,&frame)));
    IWICFormatConverter* converter=nullptr;assert(SUCCEEDED(factory->CreateFormatConverter(&converter)));
    assert(SUCCEEDED(converter->Initialize(frame,GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)));
    UINT w=0,h=0;assert(SUCCEEDED(converter->GetSize(&w,&h)));assert(w==1024 && h==512);
    std::vector<BYTE> pixels(w*h*4);assert(SUCCEEDED(converter->CopyPixels(nullptr,w*4,UINT(pixels.size()),pixels.data())));
    IDirect3DTexture9* tex=nullptr;assert(SUCCEEDED(dev->CreateTexture(w,h,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex,nullptr)));
    D3DLOCKED_RECT lr{};assert(SUCCEEDED(tex->LockRect(0,&lr,nullptr,D3DLOCK_DISCARD)));
    for(unsigned y=0;y<h;++y)memcpy(static_cast<BYTE*>(lr.pBits)+y*lr.Pitch,pixels.data()+y*w*4,w*4);
    assert(SUCCEEDED(tex->UnlockRect(0)));converter->Release();frame->Release();decoder->Release();factory->Release();return tex;
}
static std::vector<DWORD> Pixels(IDirect3DDevice9* dev,IDirect3DSurface9* target,unsigned w,unsigned h) {
    IDirect3DSurface9* read=nullptr;
    assert(SUCCEEDED(dev->CreateOffscreenPlainSurface(w,h,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)));
    assert(SUCCEEDED(dev->GetRenderTargetData(target,read)));
    D3DLOCKED_RECT lr{};assert(SUCCEEDED(read->LockRect(&lr,nullptr,D3DLOCK_READONLY)));
    std::vector<DWORD> pixels(w*h);
    for(unsigned y=0;y<h;++y)memcpy(pixels.data()+y*w,static_cast<BYTE*>(lr.pBits)+y*lr.Pitch,w*4);
    read->UnlockRect();read->Release();return pixels;
}
static void Preview(const std::vector<DWORD>& pixels,unsigned w,unsigned h) {
    IWICImagingFactory* factory=nullptr;assert(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))));
    IWICStream* stream=nullptr;assert(SUCCEEDED(factory->CreateStream(&stream)));
    assert(SUCCEEDED(stream->InitializeFromFilename(L"diegetic-d3d9-stereo.png",GENERIC_WRITE)));
    IWICBitmapEncoder* encoder=nullptr;assert(SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder)));
    assert(SUCCEEDED(encoder->Initialize(stream,WICBitmapEncoderNoCache)));
    IWICBitmapFrameEncode* frame=nullptr;assert(SUCCEEDED(encoder->CreateNewFrame(&frame,nullptr)));
    assert(SUCCEEDED(frame->Initialize(nullptr)));assert(SUCCEEDED(frame->SetSize(w,h)));
    auto fmt=GUID_WICPixelFormat32bppBGRA;assert(SUCCEEDED(frame->SetPixelFormat(&fmt)));
    assert(SUCCEEDED(frame->WritePixels(h,w*4,w*h*4,reinterpret_cast<BYTE*>(const_cast<DWORD*>(pixels.data())))));
    assert(SUCCEEDED(frame->Commit()));assert(SUCCEEDED(encoder->Commit()));
    frame->Release();encoder->Release();stream->Release();factory->Release();
}
static void Run(bool ex) {
    HWND window=CreateWindowA("STATIC","HUD offline fixture",WS_OVERLAPPED,0,0,128,64,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);assert(window);
    IDirect3D9* api=nullptr;IDirect3D9Ex* apiEx=nullptr;
    if(ex){assert(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&apiEx)));api=apiEx;}else api=Direct3DCreate9(D3D_SDK_VERSION);assert(api);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=window;pp.BackBufferWidth=128;pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_X8R8G8B8;
    IDirect3DDevice9* dev=nullptr;IDirect3DDevice9Ex* devEx=nullptr;
    if(ex){assert(SUCCEEDED(apiEx->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&devEx)));dev=devEx;}
    else assert(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev)));
    unsigned cases=0;
    for(unsigned cycle=0;cycle<2;++cycle) {
        auto* atlas=Atlas(dev);IDirect3DStateBlock9* state=nullptr;
        for(unsigned width:{1024u,4128u,5264u}) {
            const unsigned height=width==1024?512:width==4128?2208:2684;
            IDirect3DSurface9 *target=nullptr,*original=nullptr;
            assert(SUCCEEDED(dev->CreateRenderTarget(width,height,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&target,nullptr)));
            assert(SUCCEEDED(dev->GetRenderTarget(0,&original)));
            D3DVIEWPORT9 savedVp{3,4,100,50,0,1};RECT savedScissor{9,11,23,29};
            for(long mesh:{5799L,11387L,8297L})for(float roll:{0.f,.785398f,1.570796f}) {
                const auto& mo=*MountForVerts(mesh);Quad3D q[16];const int n=BuildFixtureQuads3D(mo,6,24,q,16);
                const auto plan=Project(q,n,FixturePose(mo,roll),width,height,100);assert(plan.ready);
                std::vector<DWORD> reference;
                for(int hostile=0;hostile<2;++hostile) {
                    assert(SUCCEEDED(dev->ColorFill(target,nullptr,0xff000000)));
                    assert(SUCCEEDED(dev->SetViewport(&savedVp)));assert(SUCCEEDED(dev->SetScissorRect(&savedScissor)));
                    // Inherit a deliberately hostile last-game-draw state.
                    dev->SetRenderState(D3DRS_COLORWRITEENABLE,hostile?0:15);
                    dev->SetRenderState(D3DRS_ALPHATESTENABLE,hostile);dev->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_NEVER);
                    dev->SetRenderState(D3DRS_BLENDOP,hostile?D3DBLENDOP_REVSUBTRACT:D3DBLENDOP_ADD);
                    dev->SetTextureStageState(1,D3DTSS_COLOROP,hostile?D3DTOP_MODULATE:D3DTOP_DISABLE);
                    dev->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_CURRENT);
                    dev->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_TFACTOR);dev->SetRenderState(D3DRS_TEXTUREFACTOR,0);
                    assert(SUCCEEDED(dev->BeginScene()));
                    const auto result=Render(dev,target,atlas,state,plan,
                        [&](IDirect3DSurface9* s){return dev->SetRenderTarget(0,s);},
                        [&](const ScreenVertex* v){return dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(*v));});
                    assert(SUCCEEDED(dev->EndScene()));assert(SUCCEEDED(result.hr)&&result.handledEyes==3);
                    assert(result.submitted[0]>=7 && result.submitted[1]>=7);
                    IDirect3DSurface9* restored=nullptr;assert(SUCCEEDED(dev->GetRenderTarget(0,&restored)));assert(restored==original);restored->Release();
                    D3DVIEWPORT9 vp{};dev->GetViewport(&vp);assert(memcmp(&vp,&savedVp,sizeof(vp))==0);
                    RECT sc{};dev->GetScissorRect(&sc);assert(EqualRect(&sc,&savedScissor));
                    DWORD value=99;dev->GetRenderState(D3DRS_COLORWRITEENABLE,&value);assert(value==(hostile?0u:15u));
                    dev->GetTextureStageState(1,D3DTSS_COLOROP,&value);assert(value==DWORD(hostile?D3DTOP_MODULATE:D3DTOP_DISABLE));
                    auto pixels=Pixels(dev,target,width,height);unsigned lit[2]{};
                    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)if(pixels[y*width+x]&0xffffff)++lit[x/(width/2)];
                    assert(lit[0]>50 && lit[1]>50);
                    if(!hostile)reference=pixels;else assert(pixels==reference);
                    if(!ex && !cycle && mesh==11387 && roll==0.f && width==1024)Preview(pixels,width,height);
                    ++cases;
                }
                // Failed/missing draws do not grant ownership to the new HUD.
                assert(SUCCEEDED(dev->BeginScene()));
                const auto failed=Render(dev,target,atlas,state,plan,[&](IDirect3DSurface9* s){return dev->SetRenderTarget(0,s);},
                    [](const ScreenVertex*){return E_FAIL;});
                assert(FAILED(failed.hr)&&failed.handledEyes==0);
                assert(Render(dev,target,nullptr,state,plan,[](IDirect3DSurface9*){assert(false);return E_FAIL;},
                    [](const ScreenVertex*){assert(false);return E_FAIL;}).handledEyes==0);
                DrawPlan bad=plan;bad.ready=false;
                assert(Render(dev,target,atlas,state,bad,[](IDirect3DSurface9*){assert(false);return E_FAIL;},
                    [](const ScreenVertex*){assert(false);return E_FAIL;}).handledEyes==0);
                assert(SUCCEEDED(dev->EndScene()));
            }
            target->Release();original->Release();
        }
        // Same lifetime policy as runtime: release atlas/state before Reset.
        dev->SetTexture(0,nullptr);if(state)state->Release();atlas->Release();
        if(!cycle){if(ex)assert(SUCCEEDED(devEx->ResetEx(&pp,nullptr)));else assert(SUCCEEDED(dev->Reset(&pp)));}
    }
    dev->Release();api->Release();DestroyWindow(window);
    printf("PASS D3D9%s: %u actual-atlas pixel cases, 3 weapons, 3 rolls, 3 resolutions, hostile state/restoration, failure paths, Reset\n",ex?"Ex":"",cases);
}
int main(){assert(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)));Run(false);Run(true);CoUninitialize();}
