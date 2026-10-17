// Own hidden-window D3D9Ex pixels/state/lifetime. Never loads Steam/game/proxy.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../knife_renderer.h"
#include "../centurion_shells.h"
#include <cstdio>
#include <cstdlib>
#pragma comment(lib,"user32.lib")
static void Check(bool yes,const char* s){if(!yes){puts(s);std::exit(1);}}
static unsigned draws=0,clears=0,failAt=0;
static HRESULT STDMETHODCALLTYPE Draw(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT min,UINT n,UINT count,
    const void* ix,D3DFORMAT format,const void* vertices,UINT stride){
    ++draws;if(failAt&&draws==failAt)return E_FAIL;
    return d->DrawIndexedPrimitiveUP(t,min,n,count,ix,format,vertices,stride);
}
static HRESULT STDMETHODCALLTYPE Target(IDirect3DDevice9* d,DWORD n,IDirect3DSurface9* s){return d->SetRenderTarget(n,s);}
static HRESULT STDMETHODCALLTYPE VS(IDirect3DDevice9* d,IDirect3DVertexShader9* s){return d->SetVertexShader(s);}
static HRESULT STDMETHODCALLTYPE PS(IDirect3DDevice9* d,IDirect3DPixelShader9* s){return d->SetPixelShader(s);}
static HRESULT STDMETHODCALLTYPE Tex(IDirect3DDevice9* d,DWORD n,IDirect3DBaseTexture9* s){return d->SetTexture(n,s);}
static HRESULT STDMETHODCALLTYPE Clear(IDirect3DDevice9* d,DWORD n,const D3DRECT* r,DWORD flags,D3DCOLOR c,float z,DWORD stencil){
    ++clears;return d->Clear(n,r,flags,c,z,stencil);
}
static HRESULT STDMETHODCALLTYPE Viewport(IDirect3DDevice9* d,const D3DVIEWPORT9* v){return d->SetViewport(v);}
int main(int argc,char** argv){
    Check(argc==2,"requires explicit private shell asset directory");
    HWND w=CreateWindowExW(0,L"STATIC",L"private shell fixture",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Check(w!=nullptr,"hidden window");
    char path[MAX_PATH]{};Check(GetSystemDirectoryA(path,MAX_PATH)>0,"system path");strcat_s(path,"\\d3d9.dll");
    HMODULE lib=LoadLibraryA(path);Check(lib!=nullptr,"system D3D9");
    using Create=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);
    auto create=reinterpret_cast<Create>(GetProcAddress(lib,"Direct3DCreate9Ex"));Check(create!=nullptr,"D3D9Ex export");
    IDirect3D9Ex* api=nullptr;Check(SUCCEEDED(create(D3D_SDK_VERSION,&api)),"D3D9Ex");
    D3DPRESENT_PARAMETERS p{};p.BackBufferWidth=512;p.BackBufferHeight=256;p.BackBufferFormat=D3DFMT_A8R8G8B8;
    p.BackBufferCount=1;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.Windowed=TRUE;p.hDeviceWindow=w;
    IDirect3DDevice9Ex* dev=nullptr;
    Check(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,nullptr,&dev)),"create device");
    hipknife::Renderer renderer;const std::string directory=std::string(argv[1])+"/";
    Check(renderer.LoadWithAlbedo(directory+"centurion_shell.svrk",directory+"centurion_shell.svrt"),"load original case");
    Check(renderer.Vertices()==136&&renderer.Triangles()==100,"original case topology");
    hipknife::Calls calls{Draw,Target,VS,PS,Tex,Clear,Viewport};
    hipknife::Pose poses[6];for(unsigned i=0;i<6;++i)
        poses[i]={{(float(i%3)-1)*.035f,(float(i/3)-.5f)*.04f,-.35f},centurionclip::Degrees(0,float(i)*20,0)};
    const hipknife::Pose eyes[2]{{{-.032f,0,0},{}},{{.032f,0,0},{}}};
    for(unsigned round=0;round<2;++round){
        Check(renderer.Ensure(dev,512,256),"GPU preparation/recreation");
        IDirect3DSurface9* back=nullptr;Check(SUCCEEDED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)),"backbuffer");
        IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;
        Check(SUCCEEDED(dev->CreateVertexBuffer(36,0,D3DFVF_XYZ,D3DPOOL_DEFAULT,&vb,nullptr)),"sentinel stream");
        Check(SUCCEEDED(dev->CreateIndexBuffer(6,0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr)),"sentinel indices");
        Check(SUCCEEDED(dev->SetStreamSource(0,vb,0,12))&&SUCCEEDED(dev->SetIndices(ib)),"bind sentinel buffers");
        const D3DVIEWPORT9 initial{8,4,400,200,0,1};
        const D3DVIEWPORT9 full{0,0,512,256,0,1};
        // Clear the WHOLE target, not the sentinel viewport. Otherwise black
        // uninitialised margins are miscounted as shell pixels/disparity.
        Check(SUCCEEDED(dev->SetViewport(&full))&&SUCCEEDED(dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xff102030,1,0)),"full background");
        Check(SUCCEEDED(dev->SetViewport(&initial))&&SUCCEEDED(dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE)),"seed state");
        Check(SUCCEEDED(dev->BeginScene()),"scene");
        draws=clears=failAt=0;
        Check(!renderer.DrawInstances(dev,calls,back,poses,7,eyes,1,1,.6f)&&draws==0&&clears==0,"invalid count refuses without mutation");
        Check(renderer.Draw(dev,calls,back,poses[0],eyes,1,1,.6f)&&draws==2&&clears==1,"existing one-instance route unchanged");
        draws=clears=0;
        Check(renderer.DrawInstances(dev,calls,back,poses,6,eyes,1,1,.6f),"two-eye case draw");
        Check(draws==12&&clears==1,"six cases / two eyes / one private depth clear");
        auto restored=[&](){
            D3DVIEWPORT9 v{};DWORD blend=0;
            Check(SUCCEEDED(dev->GetViewport(&v))&&!std::memcmp(&v,&initial,sizeof(v)),"viewport restored");
            Check(SUCCEEDED(dev->GetRenderState(D3DRS_ALPHABLENDENABLE,&blend))&&blend==TRUE,"render state restored");
            IDirect3DVertexBuffer9* stream=nullptr;UINT offset=0,stride=0;IDirect3DIndexBuffer9* index=nullptr;
            Check(SUCCEEDED(dev->GetStreamSource(0,&stream,&offset,&stride))&&stream==vb&&offset==0&&stride==12,"UP stream restored");
            Check(SUCCEEDED(dev->GetIndices(&index))&&index==ib,"UP index buffer restored");
            hipknife::Release(stream);hipknife::Release(index);
            IDirect3DSurface9* target=nullptr;Check(SUCCEEDED(dev->GetRenderTarget(0,&target))&&target==back,"RT0 restored");hipknife::Release(target);
            IDirect3DSurface9* depth=nullptr;Check(dev->GetDepthStencilSurface(&depth)==D3DERR_NOTFOUND&&!depth,"original no-depth restored");
        };
        restored();Check(SUCCEEDED(dev->EndScene()),"end scene");
        IDirect3DSurface9* read=nullptr;
        Check(SUCCEEDED(dev->CreateOffscreenPlainSurface(512,256,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)),"readback surface");
        Check(SUCCEEDED(dev->GetRenderTargetData(back,read)),"pixel readback");
        D3DLOCKED_RECT r{};Check(SUCCEEDED(read->LockRect(&r,nullptr,D3DLOCK_READONLY)),"readback lock");
        unsigned pixels[2]{};double sums[2]{};
        for(unsigned y=0;y<256;++y)for(unsigned x=0;x<512;++x){
            const auto colour=*(const DWORD*)((const unsigned char*)r.pBits+y*r.Pitch+x*4);
            if((colour&0xffffff)!=0x102030){const unsigned eye=x/256;++pixels[eye];sums[eye]+=x%256;}
        }
        Check(SUCCEEDED(read->UnlockRect()),"readback unlock");hipknife::Release(read);
        Check(pixels[0]>20&&pixels[1]>20,"actual textured pixels in BOTH eyes");
        std::printf("own GPU round%u: textured pixels L/R %u/%u, centroid %.2f/%.2f\n",round,pixels[0],pixels[1],sums[0]/pixels[0],sums[1]/pixels[1]);
        Check(sums[0]/pixels[0]>sums[1]/pixels[1]+10,"correct positive stereo disparity, not identical eye blits");
        Check(SUCCEEDED(dev->BeginScene()),"fault scene");draws=clears=0;failAt=3;
        Check(!renderer.DrawInstances(dev,calls,back,poses,6,eyes,1,1,.6f),"injected partial draw failure");
        restored();Check(SUCCEEDED(dev->EndScene()),"fault end scene");
        renderer.ReleaseGPU();
        Check(SUCCEEDED(dev->SetStreamSource(0,nullptr,0,0))&&SUCCEEDED(dev->SetIndices(nullptr)),"unbind sentinel");
        hipknife::Release(vb);hipknife::Release(ib);hipknife::Release(back);
        if(round==0)Check(SUCCEEDED(dev->ResetEx(&p,nullptr)),"D3D9Ex Reset after resource release");
    }
    renderer.ReleaseGPU();dev->Release();api->Release();FreeLibrary(lib);DestroyWindow(w);
    puts("PASS own-process original shell GPU pixels, stereo disparity, UP/state restoration/failure/Reset; NOT Steam/world occlusion/PSVR2");
}
