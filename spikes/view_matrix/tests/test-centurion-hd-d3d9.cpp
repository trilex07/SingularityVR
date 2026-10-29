#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include "../volk_hd_native.h"
#include <fstream>
#include <iterator>
#include <cassert>
#include <cstdio>

static void Run(const std::vector<unsigned char>& data,bool ex){
    HWND w=CreateWindowA("STATIC","Centurion offline",WS_OVERLAPPED,0,0,256,128,0,0,GetModuleHandle(nullptr),0);assert(w);
    IDirect3D9* api=nullptr;IDirect3D9Ex* ax=nullptr;
    if(ex){assert(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&ax)));api=ax;}else api=Direct3DCreate9(D3D_SDK_VERSION);
    assert(api);D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.BackBufferWidth=256;pp.BackBufferHeight=128;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.hDeviceWindow=w;
    IDirect3DDevice9* dev=nullptr;
    if(ex){IDirect3DDevice9Ex* dx=nullptr;assert(SUCCEEDED(ax->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,0,&dx)));dev=dx;}
    else assert(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&dev)));
    centurionhd::Renderer r;assert(r.asset.Load(data.data(),data.size()));
    IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;void* p=nullptr;
    assert(SUCCEEDED(dev->CreateVertexBuffer(5799*32,0,0,D3DPOOL_DEFAULT,&vb,0)));
    assert(SUCCEEDED(vb->Lock(0,0,&p,0)));memcpy(p,data.data()+28,5799*32);assert(SUCCEEDED(vb->Unlock()));
    assert(SUCCEEDED(dev->CreateIndexBuffer(6767*6,0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,0)));
    assert(SUCCEEDED(ib->Lock(0,0,&p,0)));memcpy(p,data.data()+28+centurionhd::Vertices*32,6767*6);assert(SUCCEEDED(ib->Unlock()));
    dev->SetStreamSource(0,vb,0,32);dev->SetIndices(ib);
    centurionhd::IndexWitness witness;assert(witness.Observe(dev));assert(witness.buffer==ib);
    assert(witness.Observe(dev));assert(witness.attempts==1);witness.Reset();assert(witness.Observe(dev));
    witness.Reset();

    D3DVERTEXELEMENT9 el[]={{0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},
        {0,12,D3DDECLTYPE_UBYTE4,0,D3DDECLUSAGE_TANGENT,0},{0,16,D3DDECLTYPE_UBYTE4,0,D3DDECLUSAGE_NORMAL,0},
        {0,20,D3DDECLTYPE_UBYTE4,0,D3DDECLUSAGE_BLENDINDICES,0},{0,24,D3DDECLTYPE_UBYTE4N,0,D3DDECLUSAGE_BLENDWEIGHT,0},
        {0,28,D3DDECLTYPE_FLOAT16_2,0,D3DDECLUSAGE_TEXCOORD,3},
        {0,28,D3DDECLTYPE_FLOAT16_2,0,D3DDECLUSAGE_TEXCOORD,2},
        {0,28,D3DDECLTYPE_FLOAT16_2,0,D3DDECLUSAGE_TEXCOORD,0},
        {0,28,D3DDECLTYPE_FLOAT16_2,0,D3DDECLUSAGE_TEXCOORD,1},D3DDECL_END()};
    IDirect3DVertexDeclaration9* decl=nullptr;assert(SUCCEEDED(dev->CreateVertexDeclaration(el,&decl)));dev->SetVertexDeclaration(decl);
    float lo[3]={1e6f,1e6f,1e6f},hi[3]={-1e6f,-1e6f,-1e6f};
    for(unsigned i=0;i<centurionhd::Vertices;++i){float pos[3];memcpy(pos,data.data()+28+i*32,12);for(unsigned j=0;j<3;++j){lo[j]=std::min(lo[j],pos[j]);hi[j]=std::max(hi[j],pos[j]);}}
    float scale[4]={1.8f/(hi[1]-lo[1]),1.8f/(hi[2]-lo[2]),0,0};
    float centre[4]={(hi[1]+lo[1])*.5f,(hi[2]+lo[2])*.5f,0,0};
    const char* vs="float4 s:register(c0);float4 c:register(c1);float4 main(float3 p:POSITION):POSITION{return float4((p.y-c.x)*s.x,(p.z-c.y)*s.y,.5,1);}";
    const char* ps="float4 main():COLOR{return float4(.1,.8,1,1);}";
    ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;IDirect3DVertexShader9* vertex=nullptr;IDirect3DPixelShader9* pixel=nullptr;
    assert(SUCCEEDED(D3DCompile(vs,strlen(vs),0,0,0,"main","vs_3_0",0,0,&code,&errors)));
    assert(SUCCEEDED(dev->CreateVertexShader((DWORD*)code->GetBufferPointer(),&vertex)));code->Release();
    assert(SUCCEEDED(D3DCompile(ps,strlen(ps),0,0,0,"main","ps_3_0",0,0,&code,&errors)));
    assert(SUCCEEDED(dev->CreatePixelShader((DWORD*)code->GetBufferPointer(),&pixel)));code->Release();
    dev->SetVertexShader(vertex);dev->SetPixelShader(pixel);dev->SetVertexShaderConstantF(0,scale,1);dev->SetVertexShaderConstantF(1,centre,1);
    dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
    dev->Clear(0,0,D3DCLEAR_TARGET,0,1,0);assert(SUCCEEDED(dev->BeginScene()));
    for(unsigned eye=0;eye<2;++eye){
        D3DVIEWPORT9 vp{eye*128,0,128,128,0,1};dev->SetViewport(&vp);bool used=false;
        assert(SUCCEEDED(r.DrawNative(dev,[&](unsigned v,unsigned f){assert(v==8051&&f==7927);return dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,v,0,f);},used,ib))&&used);
        IDirect3DVertexShader9* boundVS=nullptr;IDirect3DPixelShader9* boundPS=nullptr;
        dev->GetVertexShader(&boundVS);dev->GetPixelShader(&boundPS);assert(boundVS==vertex&&boundPS==pixel);boundVS->Release();boundPS->Release();
        D3DVIEWPORT9 kept{};dev->GetViewport(&kept);assert(!memcmp(&kept,&vp,sizeof(vp)));
        DWORD state=0;dev->GetRenderState(D3DRS_ZENABLE,&state);assert(state==FALSE);
        dev->GetRenderState(D3DRS_CULLMODE,&state);assert(state==D3DCULL_NONE);
        dev->GetRenderState(D3DRS_ALPHABLENDENABLE,&state);assert(state==FALSE);
        IDirect3DIndexBuffer9* actual=nullptr;dev->GetIndices(&actual);assert(actual==ib);actual->Release();
        IDirect3DVertexBuffer9* actualV=nullptr;UINT off=0,str=0;dev->GetStreamSource(0,&actualV,&off,&str);assert(actualV==vb&&off==0&&str==32);actualV->Release();
    }
    bool used=false;
    assert(FAILED(r.DrawNative(dev,[](unsigned,unsigned){return E_FAIL;},used))&&used);
    IDirect3DIndexBuffer9* actual=nullptr;dev->GetIndices(&actual);assert(actual==ib);actual->Release();
    assert(SUCCEEDED(dev->EndScene()));
    IDirect3DSurface9 *rt=nullptr,*read=nullptr;dev->GetRenderTarget(0,&rt);
    assert(SUCCEEDED(dev->CreateOffscreenPlainSurface(256,128,D3DFMT_X8R8G8B8,D3DPOOL_SYSTEMMEM,&read,0)));
    assert(SUCCEEDED(dev->GetRenderTargetData(rt,read)));D3DLOCKED_RECT lock{};read->LockRect(&lock,0,D3DLOCK_READONLY);
    unsigned pixels[2]{};for(unsigned y=0;y<128;++y)for(unsigned x=0;x<256;++x)if(((DWORD*)((char*)lock.pBits+y*lock.Pitch))[x]&0xffffff)++pixels[x/128];
    assert(pixels[0]>100&&pixels[0]==pixels[1]);read->UnlockRect();read->Release();rt->Release();
    r.ReleaseGPU();assert(r.Ensure(dev));r.ReleaseGPU();
    // Absent asset and incompatible source declaration safely draw original.
    centurionhd::Renderer empty;used=true;
    assert(SUCCEEDED(empty.DrawNative(dev,[](unsigned v,unsigned f){assert(v==5799&&f==6767);return S_OK;},used))&&!used);
    IDirect3DIndexBuffer9* foreign=nullptr;assert(SUCCEEDED(dev->CreateIndexBuffer(6,0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&foreign,0)));
    used=true;assert(SUCCEEDED(r.DrawNative(dev,[](unsigned v,unsigned){assert(v==5799);return S_OK;},used,foreign))&&!used);
    dev->SetIndices(foreign);assert(!witness.Observe(dev));dev->SetIndices(ib);assert(witness.Observe(dev));witness.Reset();foreign->Release();
    dev->SetStreamSource(0,vb,0,16);used=true;
    assert(SUCCEEDED(r.DrawNative(dev,[](unsigned v,unsigned){assert(v==5799);return S_OK;},used))&&!used);
    // Real failed Steam receipt: raw UBYTE4 tangent/normal and four UV aliases.
    // Reproduce the prior normalized fixture: it must NOT qualify as Centurion.
    el[1].Type=el[2].Type=D3DDECLTYPE_UBYTE4N;
    IDirect3DVertexDeclaration9* normalized=nullptr;assert(SUCCEEDED(dev->CreateVertexDeclaration(el,&normalized)));
    dev->SetStreamSource(0,vb,0,32);dev->SetVertexDeclaration(normalized);used=true;
    assert(SUCCEEDED(r.DrawNative(dev,[](unsigned v,unsigned f){assert(v==5799&&f==6767);return S_OK;},used,ib))&&!used);
    dev->SetVertexDeclaration(decl);normalized->Release();
    el[1].Type=el[2].Type=D3DDECLTYPE_UBYTE4;el[6].UsageIndex=4;
    IDirect3DVertexDeclaration9* wrongUv=nullptr;assert(SUCCEEDED(dev->CreateVertexDeclaration(el,&wrongUv)));
    dev->SetVertexDeclaration(wrongUv);used=true;
    assert(SUCCEEDED(r.DrawNative(dev,[](unsigned v,unsigned){assert(v==5799);return S_OK;},used,ib))&&!used);
    dev->SetVertexDeclaration(decl);wrongUv->Release();
    dev->SetStreamSource(0,nullptr,0,0);dev->SetIndices(nullptr);dev->SetVertexDeclaration(nullptr);dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);
    vb->Release();ib->Release();decl->Release();vertex->Release();pixel->Release();dev->Release();api->Release();DestroyWindow(w);
    printf("D3D9%s real SVCT stereo pixels=%u/%u; restore/failure/reset/fallback PASS\n",ex?"Ex":"",pixels[0],pixels[1]);
}
int main(int argc,char** argv){
    assert(argc==2);std::ifstream in(argv[1],std::ios::binary);std::vector<unsigned char> data{std::istreambuf_iterator<char>(in),{}};assert(!data.empty());
    centurionhd::Asset a;assert(a.Load(data.data(),data.size()));const auto before=a.bytes;
    for(size_t at: {size_t(0),size_t(4),size_t(12),size_t(28),data.size()-1}){auto bad=data;bad[at]^=1;assert(!a.Load(bad.data(),bad.size()));assert(a.bytes==before);}
    Run(data,false);Run(data,true);return 0;
}
