// Own-process original clip/albedo pixels. No game, proxy, Steam or OpenXR.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../knife_renderer.h"
#include <cstdio>
#include <cstdlib>
#include <set>
#pragma comment(lib,"user32.lib")
static void Check(bool yes,const char* why){if(!yes){puts(why);std::exit(1);}}
static unsigned draws=0,failAt=0;
static HRESULT STDMETHODCALLTYPE Draw(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT min,UINT n,UINT count,
    const void* ix,D3DFORMAT format,const void* vertices,UINT stride){
    ++draws;if(failAt&&draws==failAt)return E_FAIL;
    return d->DrawIndexedPrimitiveUP(t,min,n,count,ix,format,vertices,stride);
}
static HRESULT STDMETHODCALLTYPE RT(IDirect3DDevice9* d,DWORD n,IDirect3DSurface9* s){return d->SetRenderTarget(n,s);}
static HRESULT STDMETHODCALLTYPE VS(IDirect3DDevice9* d,IDirect3DVertexShader9* s){return d->SetVertexShader(s);}
static HRESULT STDMETHODCALLTYPE PS(IDirect3DDevice9* d,IDirect3DPixelShader9* s){return d->SetPixelShader(s);}
static HRESULT STDMETHODCALLTYPE Tex(IDirect3DDevice9* d,DWORD n,IDirect3DBaseTexture9* s){return d->SetTexture(n,s);}
static HRESULT STDMETHODCALLTYPE Clear(IDirect3DDevice9* d,DWORD n,const D3DRECT* r,DWORD flags,D3DCOLOR c,float z,DWORD s){return d->Clear(n,r,flags,c,z,s);}
static HRESULT STDMETHODCALLTYPE VP(IDirect3DDevice9* d,const D3DVIEWPORT9* v){return d->SetViewport(v);}
int main(int argc,char** argv){
    Check(argc==2,"requires explicit private prepared root");
    HWND w=CreateWindowExW(0,L"STATIC",L"offline original clips",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Check(w!=nullptr,"hidden window");
    char path[MAX_PATH]{};Check(GetSystemDirectoryA(path,MAX_PATH)>0,"system path");strcat_s(path,"\\d3d9.dll");
    HMODULE lib=LoadLibraryA(path);Check(lib!=nullptr,"system D3D9");
    using Create=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);
    auto create=reinterpret_cast<Create>(GetProcAddress(lib,"Direct3DCreate9Ex"));Check(create!=nullptr,"D3D9Ex entry");
    IDirect3D9Ex* api=nullptr;Check(SUCCEEDED(create(D3D_SDK_VERSION,&api)),"D3D9Ex");
    D3DPRESENT_PARAMETERS p{};p.BackBufferWidth=512;p.BackBufferHeight=256;p.BackBufferFormat=D3DFMT_A8R8G8B8;
    p.BackBufferCount=1;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.Windowed=TRUE;p.hDeviceWindow=w;
    IDirect3DDevice9Ex* dev=nullptr;
    Check(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,nullptr,&dev)),"own device");
    const char* names[]{"valkyrie","volk","kasimov"};const unsigned vertexCounts[]{547,242,170},triCounts[]{596,198,180};
    hipknife::Calls calls{Draw,RT,VS,PS,Tex,Clear,VP};
    const hipknife::Pose eyes[2]{{{-.032f,0,0},{}},{{.032f,0,0},{}}};
    for(unsigned profile=0;profile<3;++profile){
        const std::string prefix=std::string(argv[1])+"/"+names[profile]+"/"+names[profile]+"_clip";
        hipknife::Renderer renderer;Check(renderer.LoadWithAlbedo(prefix+".svrk",prefix+".svrt"),"original UV/texture decode");
        Check(renderer.Vertices()==vertexCounts[profile]&&renderer.Triangles()==triCounts[profile],"profile topology");
        std::vector<unsigned char> raw;std::vector<hipknife::Vertex> vertices;std::vector<WORD> indices;
        Check(hipknife::Read(prefix+".svrk",raw,1024*1024)&&hipknife::Decode(raw.data(),raw.size(),vertices,indices),"source bounds");
        hipknife::V lo{1,1,1},hi{-1,-1,-1};
        for(const auto& v:vertices){lo.x=std::min(lo.x,v.x);lo.y=std::min(lo.y,v.y);lo.z=std::min(lo.z,v.z);
            hi.x=std::max(hi.x,v.x);hi.y=std::max(hi.y,v.y);hi.z=std::max(hi.z,v.z);}
        const auto centre=(lo+hi)*.5f;
        const hipknife::Pose pose{{-centre.x,-centre.y,-.8f-centre.z},{}}; // test camera framing, NOT a controller calibration
        unsigned previous[2]{};
        for(unsigned round=0;round<2;++round){
            Check(renderer.Ensure(dev,512,256),"upload via SYSTEMMEM -> DEFAULT");
            IDirect3DSurface9* back=nullptr;Check(SUCCEEDED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)),"backbuffer");
            IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;
            Check(SUCCEEDED(dev->CreateVertexBuffer(36,0,D3DFVF_XYZ,D3DPOOL_DEFAULT,&vb,nullptr)),"sentinel VB");
            Check(SUCCEEDED(dev->CreateIndexBuffer(6,0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr)),"sentinel IB");
            Check(SUCCEEDED(dev->SetStreamSource(0,vb,0,12))&&SUCCEEDED(dev->SetIndices(ib)),"sentinel bindings");
            const D3DVIEWPORT9 full{0,0,512,256,0,1},initial{7,9,400,200,0,1};
            Check(SUCCEEDED(dev->SetViewport(&full))&&SUCCEEDED(dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xff102030,1,0)),"whole target background");
            Check(SUCCEEDED(dev->SetViewport(&initial))&&SUCCEEDED(dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE)),"sentinel state");
            auto restored=[&](){
                D3DVIEWPORT9 v{};DWORD blend=0;Check(SUCCEEDED(dev->GetViewport(&v))&&!std::memcmp(&v,&initial,sizeof(v)),"viewport restore");
                Check(SUCCEEDED(dev->GetRenderState(D3DRS_ALPHABLENDENABLE,&blend))&&blend==TRUE,"blend restore");
                IDirect3DVertexBuffer9* stream=nullptr;IDirect3DIndexBuffer9* index=nullptr;UINT off=0,stride=0;
                Check(SUCCEEDED(dev->GetStreamSource(0,&stream,&off,&stride))&&stream==vb&&off==0&&stride==12,"UP stream restore");
                Check(SUCCEEDED(dev->GetIndices(&index))&&index==ib,"UP indices restore");hipknife::Release(stream);hipknife::Release(index);
                IDirect3DSurface9* target=nullptr;Check(SUCCEEDED(dev->GetRenderTarget(0,&target))&&target==back,"RT restore");hipknife::Release(target);
                IDirect3DSurface9* depth=nullptr;Check(dev->GetDepthStencilSurface(&depth)==D3DERR_NOTFOUND&&!depth,"DS restore");
            };
            draws=failAt=0;Check(SUCCEEDED(dev->BeginScene()),"begin");
            Check(renderer.Draw(dev,calls,back,pose,eyes,1,1)&&draws==2,"one draw per eye");
            restored();Check(SUCCEEDED(dev->EndScene()),"end");
            IDirect3DSurface9* read=nullptr;Check(SUCCEEDED(dev->CreateOffscreenPlainSurface(512,256,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)),"readback");
            Check(SUCCEEDED(dev->GetRenderTargetData(back,read)),"pixel copy");D3DLOCKED_RECT r{};
            Check(SUCCEEDED(read->LockRect(&r,nullptr,D3DLOCK_READONLY)),"pixel lock");
            unsigned pixels[2]{};double sum[2]{};std::set<DWORD> colours[2];
            for(unsigned y=0;y<256;++y)for(unsigned x=0;x<512;++x){
                const DWORD colour=*reinterpret_cast<const DWORD*>(static_cast<const unsigned char*>(r.pBits)+y*r.Pitch+x*4)&0xffffff;
                if(colour!=0x102030){const unsigned eye=x/256;++pixels[eye];sum[eye]+=x%256;colours[eye].insert(colour);}
            }
            Check(SUCCEEDED(read->UnlockRect()),"pixel unlock");hipknife::Release(read);
            Check(pixels[0]>30&&pixels[1]>30&&colours[0].size()>4&&colours[1].size()>4,"textured pixels BOTH eyes");
            Check(sum[0]/pixels[0]>sum[1]/pixels[1]+3,"stereo disparity");
            if(round)Check(pixels[0]==previous[0]&&pixels[1]==previous[1],"identical Reset recreation pixels");
            previous[0]=pixels[0];previous[1]=pixels[1];
            std::printf("own GPU %s round%u L/R pixels=%u/%u colours=%zu/%zu\n",names[profile],round,pixels[0],pixels[1],colours[0].size(),colours[1].size());
            Check(SUCCEEDED(dev->BeginScene()),"fault begin");draws=0;failAt=2;
            Check(!renderer.Draw(dev,calls,back,pose,eyes,1,1),"injected second-eye failure");restored();
            Check(SUCCEEDED(dev->EndScene()),"fault end");renderer.ReleaseGPU();
            Check(SUCCEEDED(dev->SetStreamSource(0,nullptr,0,0))&&SUCCEEDED(dev->SetIndices(nullptr)),"unbind");
            hipknife::Release(vb);hipknife::Release(ib);hipknife::Release(back);
            Check(SUCCEEDED(dev->ResetEx(&p,nullptr)),"reset without retained DEFAULT resources");
        }
    }
    dev->Release();api->Release();FreeLibrary(lib);DestroyWindow(w);
    puts("PASS own-process original clips/albedo/stereo/state/Reset; NOT native animated mesh, lighting, Steam or PSVR2");
}
