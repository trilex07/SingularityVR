// OWN hidden D3D9Ex process. Synthetic skin shader/material, not game shader.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include "../weapon_reload_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#pragma comment(lib,"user32.lib")
static void Check(bool b,const char* s){if(!b){puts(s);std::exit(1);}}
template<class T>static void Drop(T*& p){if(p)p->Release();p=nullptr;}
static armbind::Mat Identity(){armbind::Mat m;for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
int main(){
    HWND w=CreateWindowExW(0,L"STATIC",L"OWN native spare fixture",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Check(w!=nullptr,"window");char path[MAX_PATH]{};Check(GetSystemDirectoryA(path,MAX_PATH)>0,"system dir");
    std::string sys(path);HMODULE dx=LoadLibraryA((sys+"/d3d9.dll").c_str());
    HMODULE compiler=LoadLibraryA((sys+"/d3dcompiler_47.dll").c_str());Check(dx&&compiler,"system libraries");
    auto compile=reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(compiler,"D3DCompile"));
    using Factory=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);
    auto factory=reinterpret_cast<Factory>(GetProcAddress(dx,"Direct3DCreate9Ex"));Check(compile&&factory,"exports");
    IDirect3D9Ex* api=nullptr;Check(SUCCEEDED(factory(D3D_SDK_VERSION,&api)),"api");
    D3DPRESENT_PARAMETERS p{};p.BackBufferWidth=512;p.BackBufferHeight=256;p.BackBufferFormat=D3DFMT_A8R8G8B8;
    p.BackBufferCount=1;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.Windowed=TRUE;p.hDeviceWindow=w;
    p.EnableAutoDepthStencil=TRUE;p.AutoDepthStencilFormat=D3DFMT_D24S8;
    IDirect3DDevice9Ex* dev=nullptr;Check(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,nullptr,&dev)),"device");
    const char* shader=
        "float4 bones[18]:register(c8);struct V{float3 p:POSITION;float b:BLENDINDICES;float4 c:COLOR;};"
        "struct O{float4 p:POSITION;float4 c:COLOR;};O vs(V v){O o;int b=(int)v.b*3;float4 p=float4(v.p,1);"
        "o.p=float4(dot(p,bones[b]),dot(p,bones[b+1]),dot(p,bones[b+2]),1);o.c=v.c;return o;}"
        "sampler2D albedo:register(s0);float4 tint:register(c0);float4 ps(O v):COLOR{return tex2D(albedo,float2(.5,.5))*v.c*tint;}";
    ID3DBlob *vcode=nullptr,*pcode=nullptr,*errors=nullptr;
    Check(SUCCEEDED(compile(shader,strlen(shader),nullptr,nullptr,nullptr,"vs","vs_3_0",0,0,&vcode,&errors)),"compile VS");Drop(errors);
    Check(SUCCEEDED(compile(shader,strlen(shader),nullptr,nullptr,nullptr,"ps","ps_3_0",0,0,&pcode,&errors)),"compile PS");Drop(errors);
    IDirect3DVertexShader9* vs=nullptr;IDirect3DPixelShader9* ps=nullptr;
    Check(SUCCEEDED(dev->CreateVertexShader((DWORD*)vcode->GetBufferPointer(),&vs))&&
        SUCCEEDED(dev->CreatePixelShader((DWORD*)pcode->GetBufferPointer(),&ps)),"shaders");Drop(vcode);Drop(pcode);
    struct Vertex{float x,y,z,b;DWORD c;};
    Vertex vertices[]{{-.8f,-.8f,.6f,0,0xffffffff},{0,.8f,.6f,0,0xffffffff},{.8f,-.8f,.6f,0,0xffffffff},
        {-.9f,-.9f,.2f,1,0xffff0000},{0,.9f,.2f,1,0xffff0000},{.9f,-.9f,.2f,1,0xffff0000}};
    const WORD indices[]{0,1,2,3,4,5};
    const D3DVERTEXELEMENT9 elements[]{{0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},
        {0,12,D3DDECLTYPE_FLOAT1,0,D3DDECLUSAGE_BLENDINDICES,0},{0,16,D3DDECLTYPE_D3DCOLOR,0,D3DDECLUSAGE_COLOR,0},D3DDECL_END()};
    for(const auto& spec:weaponreloadanim::specs)for(unsigned round=0;round<2;++round){
        const unsigned clip=weaponreload::ClipSlot(spec.kind),regs=spec.slots*3;
        for(unsigned v=0;v<6;++v)vertices[v].b=float(v<3?clip:(clip+1)%spec.slots);
        IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;
        IDirect3DVertexDeclaration9* decl=nullptr;IDirect3DTexture9* texture=nullptr;
        Check(SUCCEEDED(dev->CreateVertexBuffer(sizeof(vertices),0,0,D3DPOOL_DEFAULT,&vb,nullptr))&&
            SUCCEEDED(dev->CreateIndexBuffer(sizeof(indices),0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr))&&
            SUCCEEDED(dev->CreateVertexDeclaration(elements,&decl))&&
            SUCCEEDED(dev->CreateTexture(1,1,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,nullptr)),"resources");
        void* data=nullptr;Check(SUCCEEDED(vb->Lock(0,sizeof(vertices),&data,0)),"VB lock");memcpy(data,vertices,sizeof(vertices));Check(SUCCEEDED(vb->Unlock()),"VB unlock");
        Check(SUCCEEDED(ib->Lock(0,sizeof(indices),&data,0)),"IB lock");memcpy(data,indices,sizeof(indices));Check(SUCCEEDED(ib->Unlock()),"IB unlock");
        D3DLOCKED_RECT lock{};Check(SUCCEEDED(texture->LockRect(0,&lock,nullptr,D3DLOCK_DISCARD)),"texture lock");*(DWORD*)lock.pBits=0xffffffff;Check(SUCCEEDED(texture->UnlockRect(0)),"texture unlock");
        Check(SUCCEEDED(dev->SetVertexShader(vs))&&SUCCEEDED(dev->SetPixelShader(ps))&&SUCCEEDED(dev->SetVertexDeclaration(decl))&&
            SUCCEEDED(dev->SetStreamSource(0,vb,0,sizeof(Vertex)))&&SUCCEEDED(dev->SetIndices(ib))&&SUCCEEDED(dev->SetTexture(0,texture)),"native material/geometry bindings");
        const float tint[]{.2f,.4f,.6f,1};Check(SUCCEEDED(dev->SetPixelShaderConstantF(0,tint,1)),"native light/tint");
        Check(SUCCEEDED(dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE))&&SUCCEEDED(dev->SetRenderState(D3DRS_ZENABLE,TRUE))&&
            SUCCEEDED(dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE))&&SUCCEEDED(dev->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESS)),"native depth state");
        const D3DVIEWPORT9 full{0,0,512,256,0,1};Check(SUCCEEDED(dev->SetViewport(&full)),"full viewport");
        Check(SUCCEEDED(dev->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff010203,1,0)),"initial frame");
        // Pretend near gun/glove already populated half of each eye's depth.
        const D3DRECT nearRects[]{{0,0,128,256},{256,0,384,256}};
        Check(SUCCEEDED(dev->Clear(2,nearRects,D3DCLEAR_ZBUFFER,0,.3f,0)),"near occluder depth");
        float native[18*4]{},spare[18*4];for(unsigned s=0;s<spec.slots;++s)
            armbind::ToGpu(Identity(),reinterpret_cast<float(*)[4]>(native+s*12));
        auto d=std::make_unique<weaponreloadanim::Data>();d->ready=true;d->kind=spec.kind;
        d->slots=spec.slots;d->tracks=1;d->target[0]=clip;d->inverse[0]=Identity();
        // Shader fixture coordinates are UU. Cache tracking metres must invert
        // their original UU/52.5 exactly, for EACH real palette slot/count.
        auto metresToWorld=Identity();for(unsigned r=0;r<3;++r)metresToWorld.m[r][r]=52.5f;
        Check(weaponreload::SparePalette(*d,Identity(),Identity(),Identity(),metresToWorld,regs,spare),"spare palette");
        Check(SUCCEEDED(dev->BeginScene()),"scene");
        for(unsigned eye=0;eye<2;++eye){
            const D3DVIEWPORT9 vp{eye*256,0,256,256,0,1};Check(SUCCEEDED(dev->SetViewport(&vp)),"eye viewport");
            Check(SUCCEEDED(dev->SetVertexShaderConstantF(8,native,regs)),"original palette");bool restored=false;
            Check(weaponreload::Replay([&](const float* pal){return SUCCEEDED(dev->SetVertexShaderConstantF(8,pal,regs));},
                [&](){return SUCCEEDED(dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,6,0,2));},spare,native,restored)&&restored,"native replay");
            float got[18*4];Check(SUCCEEDED(dev->GetVertexShaderConstantF(8,got,regs))&&!memcmp(got,native,regs*4*sizeof(float)),"palette restored");
        }
        Check(SUCCEEDED(dev->EndScene()),"end");
        IDirect3DSurface9 *back=nullptr,*read=nullptr,*depth=nullptr;IDirect3DPixelShader9* bound=nullptr;IDirect3DBaseTexture9* tex=nullptr;
        Check(SUCCEEDED(dev->GetPixelShader(&bound))&&bound==ps&&SUCCEEDED(dev->GetTexture(0,&tex))&&tex==texture&&
            SUCCEEDED(dev->GetDepthStencilSurface(&depth))&&depth,"material/texture/shared depth retained");Drop(bound);Drop(tex);Drop(depth);
        Check(SUCCEEDED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))&&
            SUCCEEDED(dev->CreateOffscreenPlainSurface(512,256,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr))&&
            SUCCEEDED(dev->GetRenderTargetData(back,read))&&SUCCEEDED(read->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"read pixels");
        unsigned visible[2]{},bad=0;
        for(unsigned y=0;y<256;++y)for(unsigned x=0;x<512;++x){
            const DWORD c=*(DWORD*)((unsigned char*)lock.pBits+y*lock.Pitch+x*4)&0xffffff;
            if(c!=0x010203){++visible[x/256];if(x%256<128||c!=0x336699)++bad;}
        }
        Check(SUCCEEDED(read->UnlockRect()),"read unlock");
        Check(visible[0]>1000&&visible[0]==visible[1]&&bad==0,"both eyes native tint; near occlusion; no other gun faces");
        printf("OWN GPU %s native replay round%u pixels=%u/%u; wrong colour/occlusion/body pixels=%u\n",weaponreload::Stem(spec.kind),round,visible[0],visible[1],bad);
        Drop(read);Drop(back);dev->SetTexture(0,nullptr);dev->SetIndices(nullptr);dev->SetStreamSource(0,nullptr,0,0);dev->SetVertexDeclaration(nullptr);
        Drop(texture);Drop(vb);Drop(ib);Drop(decl);
        if(round==0)Check(SUCCEEDED(dev->ResetEx(&p,nullptr)),"ResetEx");
    }
    dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);Drop(vs);Drop(ps);Drop(dev);Drop(api);
    FreeLibrary(compiler);FreeLibrary(dx);DestroyWindow(w);
    puts("PASS own GPU THREE clip slots / shared depth / material / palette replay BOTH eyes + Reset. Synthetic shader/geometry, NOT game lighting or PSVR2.");
}
