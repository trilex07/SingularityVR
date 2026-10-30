#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cassert>
#include <cstdio>
#include "../wrist_equipment_occlusion.h"
static HRESULT STDMETHODCALLTYPE Bind(IDirect3DDevice9* d,IDirect3DSurface9* s){return d->SetDepthStencilSurface(s);}
static HRESULT STDMETHODCALLTYPE Draw(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT n,const void* p,UINT stride){return d->DrawPrimitiveUP(t,n,p,stride);}
static HRESULT STDMETHODCALLTYPE FailBind(IDirect3DDevice9*,IDirect3DSurface9*){return D3DERR_INVALIDCALL;}
static void Quad(IDirect3DDevice9* d,float x0,float x1,float z,DWORD color,float y0=0,float y1=128){
    struct V{float x,y,z,rhw;DWORD c;};V v[4]{{x0-.5f,y0-.5f,z,1,color},{x1-.5f,y0-.5f,z,1,color},
        {x0-.5f,y1-.5f,z,1,color},{x1-.5f,y1-.5f,z,1,color}};
    assert(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(V))));
}
static void States(IDirect3DDevice9* d){
    d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetTexture(0,nullptr);
    d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);d->SetRenderState(D3DRS_LIGHTING,FALSE);
    d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ZENABLE,TRUE);
    d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
    d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
    d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
    d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
    d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
    d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
    d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
}
static void Run(bool ex){
    HWND w=CreateWindowA("STATIC","Offline wrist depth pixels",WS_OVERLAPPED,0,0,256,128,0,0,GetModuleHandle(nullptr),0);assert(w);
    IDirect3D9* api=nullptr;IDirect3D9Ex* ax=nullptr;
    if(ex){assert(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&ax)));api=ax;}else api=Direct3DCreate9(D3D_SDK_VERSION);
    assert(api);D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.BackBufferWidth=256;pp.BackBufferHeight=128;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.hDeviceWindow=w;
    IDirect3DDevice9* d=nullptr;
    if(ex){IDirect3DDevice9Ex* dx=nullptr;assert(SUCCEEDED(ax->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,0,&dx)));d=dx;}
    else assert(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&d)));
    wristdepth::State s;s.native=Bind;
    // OFF is safe even without a device: no resource queries or bindings.
    for(int i=0;i<20000;++i){s.Eye(nullptr,0,0,nullptr,0,{});assert(!s.BeforeClear(nullptr,true,0,D3DCLEAR_ZBUFFER));}
    IDirect3DSurface9 *world=nullptr,*shadow=nullptr,*back=nullptr;
    assert(SUCCEEDED(d->CreateDepthStencilSurface(256,128,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&world,0)));
    assert(SUCCEEDED(d->CreateDepthStencilSurface(256,128,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&shadow,0)));
    d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back);d->SetDepthStencilSurface(world);
    D3DVIEWPORT9 vp{0,0,256,128,0,1};d->SetViewport(&vp);s.enabled=true;
    float m[16]{};m[0]=m[5]=m[10]=m[15]=1;
    // Unknown/discardable surfaces cannot establish a preservation witness.
    s.Eye(d,0,0,m,0,vp);assert(!s.world);wristdepth::Created(world,TRUE);
    s.Eye(d,0,0,m,0,vp);assert(!s.world);wristdepth::Created(world,FALSE);
    assert(SUCCEEDED(d->BeginScene()));States(d);
    d->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0xFF000000,1,0);
    // Native world wall covers first region in each eye.
    for(int eye=0;eye<2;++eye)Quad(d,float(eye*128),float(eye*128+32),.2f,0xFF202020);
    s.Eye(d,0,0,m,0,vp);s.Eye(d,0,1,m,0,vp);assert(s.world==world);
    assert(!s.BeforeClear(d,true,1,D3DCLEAR_ZBUFFER)); // partial clears never switch
    d->SetDepthStencilSurface(shadow);assert(!s.BeforeClear(d,true,0,D3DCLEAR_ZBUFFER));
    d->SetDepthStencilSurface(world);
    assert(s.BeforeClear(d,true,0,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL));
    assert(s.active&&s.foreground&&s.mask);d->Clear(0,0,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,1,0);
    // Foreground glove and carried magazine cover separate regions per eye.
    for(int eye=0;eye<2;++eye){Quad(d,float(eye*128+32),float(eye*128+64),.2f,0xFF404040);
        Quad(d,float(eye*128+64),float(eye*128+96),.3f,0xFF606060);}
    s.Eye(d,1,0,m,0,vp);s.Eye(d,1,1,m,0,vp);assert(s.Ready());
    s.Bind(d,world);IDirect3DSurface9* bound=nullptr;d->GetDepthStencilSurface(&bound);assert(bound==s.foreground);bound->Release();
    s.Bind(d,shadow);d->GetDepthStencilSurface(&bound);assert(bound==shadow);bound->Release();s.Bind(d,world);
    wristdepth::PixelWitness witness;assert(!witness.Start(nullptr,false));assert(witness.Start(d,true));
    // Use actual preserved native depth to generate the HUD world visibility mask.
    s.internal=true;d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(0,s.maskSurface);d->SetDepthStencilSurface(world);
    d->SetViewport(&vp);d->Clear(0,0,D3DCLEAR_TARGET,0,1,0);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    for(int eye=0;eye<2;++eye){witness.Begin(0,eye);
        Quad(d,float(eye*128),float((eye+1)*128),.5f,0xFFFFFFFF);witness.End(0,eye);}
    d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(0,back);d->SetDepthStencilSurface(s.foreground);d->SetViewport(&vp);
    d->SetTexture(0,s.mask);d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
    d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
    d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    for(int eye=0;eye<2;++eye){wristdepth::Projected p[4];
        for(int k=0;k<4;++k){float v[3]{-1.f+float(eye)+(k&1),1.f-2.f*(k>>1),.5f};assert(s.Project(0,eye,v,p[k]));}
        witness.Begin(1,eye);assert(SUCCEEDED(wristdepth::CompositeEye(d,Draw,eye,256,128,p,p)));witness.End(1,eye);
    }
    // State blocks alone cannot undo the viewport reset caused by RT0 restore.
    // Check the explicit restore ordering used by the runtime on real drivers.
    IDirect3DStateBlock9* saved=nullptr;D3DVIEWPORT9 narrow{4,6,200,100,.1f,.9f};
    d->SetViewport(&narrow);assert(SUCCEEDED(d->CreateStateBlock(D3DSBT_ALL,&saved)));
    saved->Capture();d->SetViewport(&vp);saved->Apply();d->SetDepthStencilSurface(nullptr);
    d->SetRenderTarget(0,back);d->SetDepthStencilSurface(s.foreground);d->SetViewport(&narrow);
    D3DVIEWPORT9 restored{};d->GetViewport(&restored);assert(!memcmp(&restored,&narrow,sizeof narrow));saved->Release();
    assert(SUCCEEDED(d->EndScene()));
    IDirect3DSurface9* read=nullptr;assert(SUCCEEDED(d->CreateOffscreenPlainSurface(256,128,D3DFMT_X8R8G8B8,D3DPOOL_SYSTEMMEM,&read,0)));
    assert(SUCCEEDED(d->GetRenderTargetData(back,read)));D3DLOCKED_RECT lr{};assert(SUCCEEDED(read->LockRect(&lr,0,D3DLOCK_READONLY)));
    for(int eye=0;eye<2;++eye)for(int region=0;region<4;++region){
        DWORD c=*(DWORD*)((char*)lr.pBits+64*lr.Pitch+(eye*128+region*32+16)*4)&0xFFFFFF;
        const DWORD expect[4]{0x202020,0x404040,0x606060,0xFFFFFF};assert(c==expect[region]);
    }
    read->UnlockRect();
    // Submit offline work, then poll WITHOUT FLUSH exactly as runtime does.
    assert(SUCCEEDED(d->Present(nullptr,nullptr,nullptr,nullptr)));
    bool reported=false;for(unsigned wait=0;wait<100&&!reported;++wait){reported=witness.Poll();if(!reported)Sleep(1);}
    assert(reported&&witness.done&&!witness.q[0][0]);
    for(int e=0;e<2;++e)assert(witness.pixels[0][e]>0&&witness.pixels[1][e]>0);
    // Tilted wrist: the sampled texture is already a screen-space mask.
    // Varying native W must not perspective-warp its UVs a second time.
    assert(SUCCEEDED(d->BeginScene()));d->SetTexture(0,nullptr);
    d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(0,s.maskSurface);d->SetViewport(&vp);States(d);
    d->SetRenderState(D3DRS_ZENABLE,FALSE);d->Clear(0,0,D3DCLEAR_TARGET,0,1,0);
    const DWORD stripes[4]{0xFFAA1100,0xFF00BB22,0xFF2200CC,0xFFDDDD00};
    for(int eye=0;eye<2;++eye)for(int region=0;region<4;++region)
        Quad(d,float(eye*128+region*32),float(eye*128+(region+1)*32),.5f,stripes[region]);
    d->SetRenderTarget(0,back);d->SetViewport(&vp);d->SetTexture(0,s.mask);
    d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
    d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
    d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
    for(int eye=0;eye<2;++eye){wristdepth::Projected p[4];
        for(int k=0;k<4;++k){const float x=float(eye*128+(k&1)*128);
            p[k]={x-.5f,float((k>>1)*128)-.5f,.5f,(k&1)?.15f:1.f,x/256.f,float(k>>1)};}
        assert(SUCCEEDED(wristdepth::CompositeEye(d,Draw,eye,256,128,p,p)));
    }
    assert(SUCCEEDED(d->EndScene()));assert(SUCCEEDED(d->GetRenderTargetData(back,read)));
    assert(SUCCEEDED(read->LockRect(&lr,0,D3DLOCK_READONLY)));
    for(int eye=0;eye<2;++eye)for(int region=0;region<4;++region){
        const DWORD c=*(DWORD*)((char*)lr.pBits+64*lr.Pitch+(eye*128+region*32+16)*4)&0xFFFFFF;
        assert(c==(stripes[region]&0xFFFFFF));
    }
    read->UnlockRect();read->Release();d->SetTexture(0,nullptr);

    d->SetDepthStencilSurface(s.foreground);s.internal=false;s.NewFrame(d);assert(!s.active&&!s.Ready());d->GetDepthStencilSurface(&bound);assert(bound==world);bound->Release();d->SetViewport(&vp);
    s.Eye(d,0,0,m,0,vp);s.Eye(d,0,1,m,0,vp);s.native=FailBind;
    assert(!s.BeforeClear(d,true,0,D3DCLEAR_ZBUFFER)); // stencil preservation gate
    assert(!s.BeforeClear(d,true,0,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL)&&s.failed&&!s.Ready());s.native=Bind;s.Release();
    d->SetDepthStencilSurface(nullptr);world->Release();shadow->Release();back->Release();
    assert(SUCCEEDED(d->Reset(&pp)));
    if(ex)assert(SUCCEEDED(static_cast<IDirect3DDevice9Ex*>(d)->ResetEx(&pp,nullptr)));
    d->Release();api->Release();DestroyWindow(w);
    printf("PASS %s world wall + foreground glove/magazine pixels in both eyes; exact identity, discard, OFF, failed bind, reset\n",ex?"D3D9Ex":"D3D9");
}
int main(){
    // Same logical glyph orientation, viewed from the palm side. Top points
    // toward fingers (-wrist Y); right points across palm (+wrist Z). Their
    // cross product faces inward (+X), not the back of the old outer panel.
    const auto point=[](float x,float y){wristdepth::QuarterTurn(x,y);
        return wristdepth::SurfacePoint((x-.5f)*.155f,(.5f-y)*.104f,0,.094f,.080f,0,true);};
    const auto centre=point(.5f,.5f),right=point(1,.5f)-centre,top=point(.5f,0)-centre;
    assert(right.z>0&&top.y<0&&right.y==0&&top.z==0);
    assert(right.y*top.z-right.z*top.y>0&&centre.x>0);
    assert(fabsf((point(.5f,0).y)-(.0025f))<1e-6f); // front edge at wrist
    float x=.2f,y=.7f;wristdepth::QuarterTurn(x,y);assert(std::fabs(x-.7f)<.00001f&&std::fabs(y-.8f)<.00001f);
    wristdepth::Projected a[4],b[4];for(int k=0;k<4;++k){a[k].rhw=1+k*.1f;b[k].rhw=a[k].rhw*2;}
    assert(wristdepth::CompatibleW(a,b));b[3].rhw*=2;assert(!wristdepth::CompatibleW(a,b));
    wristdepth::State s;D3DSURFACE_DESC bad{};bad.Width=256;bad.Height=128;bad.Format=D3DFMT_D24S8;
    assert(s.Compatible(bad));bad.MultiSampleType=D3DMULTISAMPLE_2_SAMPLES;assert(!s.Compatible(bad));
    auto& c=s.cameras[0][0];c.valid=true;c.vp={0,0,5640,2876,0,1};
    // Same perspective camera in row/column storage, including W != 1.
    float m[16]{1,0,0,0,0,1,0,0,0,0,.8f,1,0,0,-.1f,0};
    memcpy(c.m,m,sizeof m);wristdepth::Projected row,col;float p[3]{-1,0,2};
    assert(s.Project(0,0,p,row));c.conv=1;
    for(int i=0;i<4;++i)for(int j=0;j<4;++j)c.m[i*4+j]=m[j*4+i];
    assert(s.Project(0,0,p,col));assert(std::fabs(row.x-col.x)<.001f&&std::fabs(row.z-col.z)<.00001f);
    assert(std::fabs(row.rhw-.5f)<.00001f&&std::fabs(row.z-.75f)<.00001f);
    p[2]=-2;assert(!s.Project(0,0,p,col));assert(!s.Ready());
    Run(false);Run(true);
}
