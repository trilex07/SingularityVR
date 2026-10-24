#define NOMINMAX
#include "../seeker_index_identity.h"
#include <cassert>
#include <cstdio>
#include <vector>
// Hidden standalone D3D9/Ex device; NEVER loads or launches the game.
static void Run(const std::vector<unsigned char>& data,bool ex){
    HWND window=CreateWindowA("STATIC","Seeker IB test",WS_OVERLAPPED,0,0,128,64,
        nullptr,nullptr,GetModuleHandle(nullptr),nullptr);assert(window);
    IDirect3D9* api=nullptr;IDirect3D9Ex* apiEx=nullptr;
    if(ex){assert(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&apiEx)));api=apiEx;}
    else api=Direct3DCreate9(D3D_SDK_VERSION);assert(api);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=window;pp.BackBufferWidth=128;pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_X8R8G8B8;
    IDirect3DDevice9* dev=nullptr;
    if(ex){IDirect3DDevice9Ex* d=nullptr;assert(SUCCEEDED(apiEx->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&d)));dev=d;}
    else assert(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev)));
    seekergeometry::IndexIdentity identity;
    assert(!identity.Match(dev));
    for(unsigned round=0;round<3;++round){
        IDirect3DIndexBuffer9* ib=nullptr;
        assert(SUCCEEDED(dev->CreateIndexBuffer((UINT)data.size(),0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr)));
        void* bytes=nullptr;assert(SUCCEEDED(ib->Lock(0,0,&bytes,0)));memcpy(bytes,data.data(),data.size());
        if(round==1)((unsigned char*)bytes)[12]^=1;
        assert(SUCCEEDED(ib->Unlock()));assert(SUCCEEDED(dev->SetIndices(ib)));
        for(unsigned eye=0;eye<2;++eye)assert(identity.Match(dev)==(round!=1));
        IDirect3DIndexBuffer9* bound=nullptr;assert(SUCCEEDED(dev->GetIndices(&bound))&&bound==ib);bound->Release();
        // Cache must not modify binding or retain a DEFAULT resource across Reset.
        identity.Reset();dev->SetIndices(nullptr);ib->Release();
        assert(SUCCEEDED(dev->Reset(&pp)));
    }
    dev->Release();api->Release();DestroyWindow(window);
    printf("PASS D3D9%s Seeker exact IB: valid/corrupt/cache/binding unchanged/reset/recreate\n",ex?"Ex":"");
}
int main(int argc,char** argv){assert(argc==2);FILE* f=nullptr;assert(fopen_s(&f,argv[1],"rb")==0);
    std::vector<unsigned char> data(seekergeometry::IndexBytes);
    assert(fread(data.data(),1,data.size(),f)==data.size()&&fgetc(f)==EOF);fclose(f);
    Run(data,false);Run(data,true);
}
