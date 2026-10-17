// OWN process; exact private cooked VB/IB, synthetic shader. Never game/proxy.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include "../centurion_tracked_cylinder.h"
#include "../centurion_shells.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#pragma comment(lib,"user32.lib")
using namespace centuriontracked;
static void Check(bool b,const char* s){if(!b){puts(s);std::exit(1);}}
template<class T>static void Drop(T*& p){if(p)p->Release();p=nullptr;}
static std::vector<unsigned char> Read(const char* path){
    FILE* f=nullptr;Check(!fopen_s(&f,path,"rb")&&f,"fixture open");
    Check(!fseek(f,0,SEEK_END),"seek");const long size=ftell(f);
    Check(size>0&&size<1000000&&!fseek(f,0,SEEK_SET),"size");
    std::vector<unsigned char> b(size);Check(fread(b.data(),1,b.size(),f)==b.size(),"read");fclose(f);return b;
}
int main(int argc,char** argv){
    Check(argc==3,"usage: own-fixture animation.svca native.svnv");
    const auto cache=Read(argv[1]),geometry=Read(argv[2]);centurionanim::Data legacy,d;
    Check(centurionanim::Decode(cache.data(),cache.size(),legacy)&&centurionanim::ToNative(legacy,d),"native conversion");
    centurionanim::Data again;Check(centurionanim::ToNative(d,again)&&again.nativeSpace&&
        !memcmp(d.parent.m,again.parent.m,sizeof(d.parent.m))&&!memcmp(d.inverse,again.inverse,sizeof(d.inverse))&&
        !memcmp(d.keys,again.keys,sizeof(d.keys)),"idempotent native cache");
    unsigned header[3];Check(geometry.size()>=16&&!memcmp(geometry.data(),"SVNV",4),"geometry header");
    memcpy(header,geometry.data()+4,sizeof(header));
    Check(header[0]==1&&header[1]==5799&&header[2]==20301&&geometry.size()==16+5799*32+20301*2,"exact original topology");
    const auto* vertices=geometry.data()+16;const auto* indices=vertices+5799*32;
    Check(centurionanim::Hash(indices,20301*2)==d.indexHash,"native index hash");
    Mat bind,rawBind;Check(Inverse(d.inverse[0],bind)&&Inverse(legacy.inverse[0],rawBind),"bind");
    Check(hipknife::Length(Point({},bind)-Point({},rawBind))>226,"negative control has actual 226UU mismatch");
    // Native clip points must really be local to the native bind, NOT just a
    // roundtrip made from the same wrong source convention on both sides.
    unsigned clipVertices=0;
    for(unsigned i=0;i<5799;++i){const auto* v=vertices+i*32;
        if(v[20]==0&&v[24]==255){V p;memcpy(&p,v,12);
            Check(hipknife::Length(Point(p,d.inverse[0]))<10,"actual cooked point near native joint");
            Check(hipknife::Length(Point(p,legacy.inverse[0]))>220,"old exported bind fails actual cooked points");++clipVertices;}
    }
    Check(clipVertices==1067,"all original rigid clip GPU wedges (seams included)");
    HWND w=CreateWindowExW(0,L"STATIC",L"OWN original native cylinder",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Check(w!=nullptr,"window");char path[MAX_PATH]{};Check(GetSystemDirectoryA(path,MAX_PATH)>0,"system dir");
    std::string sys(path);HMODULE dx=LoadLibraryA((sys+"/d3d9.dll").c_str());
    HMODULE compiler=LoadLibraryA((sys+"/d3dcompiler_47.dll").c_str());Check(dx&&compiler,"system libraries");
    auto compile=reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(compiler,"D3DCompile"));
    using Factory=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);
    auto factory=reinterpret_cast<Factory>(GetProcAddress(dx,"Direct3DCreate9Ex"));Check(compile&&factory,"exports");
    IDirect3D9Ex* api=nullptr;Check(SUCCEEDED(factory(D3D_SDK_VERSION,&api)),"api");
    D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=512;pp.BackBufferHeight=256;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    pp.BackBufferCount=1;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.Windowed=TRUE;pp.hDeviceWindow=w;
    IDirect3DDevice9Ex* dev=nullptr;Check(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,nullptr,&dev)),"device");
    const char* shader=
        "float4 bones[15]:register(c8);float4 lc[4]:register(c40);float4 vp[4]:register(c44);"
        "float4 vs(float3 p:POSITION,float4 bi:BLENDINDICES,float4 bw:BLENDWEIGHT):POSITION{"
        "float3 s=0;[unroll]for(int i=0;i<4;i++){int b=(int)bi[i]*3;float4 q=float4(p,1);"
        "s+=bw[i]*float3(dot(q,bones[b]),dot(q,bones[b+1]),dot(q,bones[b+2]));}"
        "float4 a=s.x*lc[0]+s.y*lc[1]+s.z*lc[2]+lc[3];"
        "return a.x*vp[0]+a.y*vp[1]+a.z*vp[2]+a.w*vp[3];}"
        "float4 ps():COLOR{return float4(0,1,0,1);}";
    ID3DBlob *vcode=nullptr,*pcode=nullptr,*errors=nullptr;
    HRESULT hr=compile(shader,strlen(shader),nullptr,nullptr,nullptr,"vs","vs_3_0",0,0,&vcode,&errors);
    if(FAILED(hr)&&errors)puts((const char*)errors->GetBufferPointer());Check(SUCCEEDED(hr),"compile VS");Drop(errors);
    Check(SUCCEEDED(compile(shader,strlen(shader),nullptr,nullptr,nullptr,"ps","ps_3_0",0,0,&pcode,&errors)),"compile PS");Drop(errors);
    IDirect3DVertexShader9* vs=nullptr;IDirect3DPixelShader9* ps=nullptr;
    Check(SUCCEEDED(dev->CreateVertexShader((DWORD*)vcode->GetBufferPointer(),&vs))&&SUCCEEDED(dev->CreatePixelShader((DWORD*)pcode->GetBufferPointer(),&ps)),"shaders");Drop(vcode);Drop(pcode);
    const D3DVERTEXELEMENT9 elements[]{{0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},
        {0,20,D3DDECLTYPE_UBYTE4,0,D3DDECLUSAGE_BLENDINDICES,0},{0,24,D3DDECLTYPE_UBYTE4N,0,D3DDECLUSAGE_BLENDWEIGHT,0},D3DDECL_END()};
    IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;IDirect3DVertexDeclaration9* decl=nullptr;
    Check(SUCCEEDED(dev->CreateVertexBuffer(5799*32,0,0,D3DPOOL_DEFAULT,&vb,nullptr))&&
        SUCCEEDED(dev->CreateIndexBuffer(20301*2,0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr))&&SUCCEEDED(dev->CreateVertexDeclaration(elements,&decl)),"resources");
    void* data=nullptr;Check(SUCCEEDED(vb->Lock(0,0,&data,0)),"VB lock");memcpy(data,vertices,5799*32);Check(SUCCEEDED(vb->Unlock()),"VB unlock");
    Check(SUCCEEDED(ib->Lock(0,0,&data,0)),"IB lock");memcpy(data,indices,20301*2);Check(SUCCEEDED(ib->Unlock()),"IB unlock");
    Check(SUCCEEDED(dev->SetVertexShader(vs))&&SUCCEEDED(dev->SetPixelShader(ps))&&SUCCEEDED(dev->SetVertexDeclaration(decl))&&
        SUCCEEDED(dev->SetStreamSource(0,vb,0,32))&&SUCCEEDED(dev->SetIndices(ib)),"bindings");
    Check(SUCCEEDED(dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE))&&SUCCEEDED(dev->SetRenderState(D3DRS_ZENABLE,FALSE)),"raster state");
    auto root=Identity();for(unsigned i=0;i<3;++i)root.m[3][i]=-bind.m[3][i];
    auto chain=Identity();chain.m[0][0]=chain.m[1][1]=chain.m[2][2]=1/52.5f;
    auto projection=Identity();projection.m[0][0]=projection.m[1][1]=10;
    Check(SUCCEEDED(dev->SetVertexShaderConstantF(40,&chain.m[0][0],4)),"local chain");
    for(unsigned mode=0;mode<3;++mode){
        const Pose target{{0,0,.6f},{}};float pal[15][4];
        Check(centurionvisual::Spare(mode==0?legacy:d,root,Identity(),chain,Identity(),{},target,pal),"carried native palette");
        // Same original geometry and chain in both negative and positive tests.
        if(mode==2){
            float original[15][4];for(unsigned slot=0;slot<5;++slot)armbind::ToGpu(root,reinterpret_cast<float(*)[4]>(original+slot*3));
            V socket;Check(Attached(d,{true,false,12},1,original,root,Identity(),chain,Identity(),{},pal,socket),"attached native palette");
            Check(socket.x<-.10f&&std::fabs(socket.y)<.0001f&&std::fabs(socket.z)<.0001f,"socket left beside actual gun");
            centurionreload::Frame f;f.enabled=f.context=f.tracked=f.supported=f.ammoFresh=true;
            f.pawn=1;f.weapon=2;f.nowMs=1000;f.clip=3;f.reserve=12;f.capacity=6;f.socket=socket;
            const centurionshells::WorldFrame world{{200,-500,70},{.1f,.2f,-.3f},.4f,-.7f,52.5f};
            centurionshells::Burst burst;Pose cases[6];
            Check(centurionshells::Launch(burst,f,centurionreload::Event::Dumped,true,world)&&
                centurionshells::Sample(burst,world,1,2,1000,cases)==3,"corrected rendered joint feeds actual case launch");
            for(unsigned i=0;i<3;++i)Check(hipknife::Length(cases[i].p-socket)<.0051f,"cases start within5mm of corrected joint, not metres away");
            // Isolate clip from body for this position oracle, using same palette.
            for(unsigned slot=1;slot<5;++slot)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)pal[slot*3+r][c]=0;
            projection.m[3][0]=-socket.x*10;projection.m[3][2]=.6f;
        }
        const D3DVIEWPORT9 full{0,0,512,256,0,1};Check(SUCCEEDED(dev->SetViewport(&full))&&SUCCEEDED(dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xff010203,1,0)),"clear");
        Check(SUCCEEDED(dev->BeginScene()),"scene");
        for(unsigned eye=0;eye<2;++eye){const D3DVIEWPORT9 view{eye*256,0,256,256,0,1};
            auto eyeProjection=projection;eyeProjection.m[3][0]+=(eye?-.02f:.02f);
            Check(SUCCEEDED(dev->SetViewport(&view))&&SUCCEEDED(dev->SetVertexShaderConstantF(44,&eyeProjection.m[0][0],4))&&
                SUCCEEDED(dev->SetVertexShaderConstantF(8,&pal[0][0],15))&&SUCCEEDED(dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,5799,0,6767)),"two-eye draw");}
        Check(SUCCEEDED(dev->EndScene()),"end");
        IDirect3DSurface9 *back=nullptr,*read=nullptr;D3DLOCKED_RECT lock{};
        Check(SUCCEEDED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))&&SUCCEEDED(dev->CreateOffscreenPlainSurface(512,256,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr))&&
            SUCCEEDED(dev->GetRenderTargetData(back,read))&&SUCCEEDED(read->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"readback");
        unsigned pixels[2]{},bad=0;int minX[2]{256,256},maxX[2]{},minY[2]{256,256},maxY[2]{};
        for(int y=0;y<256;++y)for(int x=0;x<512;++x){const auto colour=*(DWORD*)((unsigned char*)lock.pBits+y*lock.Pitch+x*4)&0xffffff;
            if(colour!=0x010203){const auto e=x/256,px=x%256;++pixels[e];if(colour!=0x00ff00)++bad;
                minX[e]=std::min(minX[e],px);maxX[e]=std::max(maxX[e],px);minY[e]=std::min(minY[e],y);maxY[e]=std::max(maxY[e],y);}}
        Check(SUCCEEDED(read->UnlockRect())&&bad==0,"readback colour");Drop(back);Drop(read);
        printf("native cooked mode%u pixels=%u/%u bounds L=%d,%d..%d,%d R=%d,%d..%d,%d\n",mode,pixels[0],pixels[1],minX[0],minY[0],maxX[0],maxY[0],minX[1],minY[1],maxX[1],maxY[1]);
        if(mode==0)Check(!pixels[0]&&!pixels[1],"old cache falsifier: invisible actual native clip");
        else for(unsigned e=0;e<2;++e)Check(pixels[e]>500&&minX[e]>35&&maxX[e]<225&&minY[e]>35&&maxY[e]<225,"actual native clip visible at target in BOTH eyes");
    }
    dev->SetIndices(nullptr);dev->SetStreamSource(0,nullptr,0,0);dev->SetVertexDeclaration(nullptr);dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);
    Drop(decl);Drop(vb);Drop(ib);Drop(vs);Drop(ps);Drop(dev);Drop(api);FreeLibrary(compiler);FreeLibrary(dx);DestroyWindow(w);
    puts("PASS exact native VB/IB space: old path invisible, converted carried+attached BOTH eyes visible. NOT game lighting/Steam/PSVR2.");
}
