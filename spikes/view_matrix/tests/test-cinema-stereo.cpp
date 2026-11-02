#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../cinema_stereo.h"
static void require(bool b,const char* what){if(!b){printf("FAIL: %s\n",what);exit(1);}}
static HRESULT set(IDirect3DDevice9* d,UINT r,const float* m,UINT n){return d->SetVertexShaderConstantF(r,m,n);}
static ID3DBlob* compile(const char* text,const char* target){ID3DBlob* b=nullptr,*e=nullptr;
 const HRESULT hr=D3DCompile(text,strlen(text),nullptr,nullptr,nullptr,"main",target,0,0,&b,&e);
 if(e){puts((const char*)e->GetBufferPointer());e->Release();}require(SUCCEEDED(hr),"shader compile");return b;}
static bool run(bool ex){
 const int W=320,H=160;HWND wnd=CreateWindowA("STATIC","stereo cinema fixture",WS_OVERLAPPEDWINDOW,0,0,W,H,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);
 require(wnd!=nullptr,"window");IDirect3D9* d9=nullptr;IDirect3D9Ex* d9ex=nullptr;
 if(ex){require(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&d9ex)),"D3D9Ex");d9=d9ex;}else d9=Direct3DCreate9(D3D_SDK_VERSION);
 require(d9!=nullptr,"D3D9");char module[MAX_PATH]{};GetModuleFileNameA(GetModuleHandleA("d3d9.dll"),module,MAX_PATH);printf("D3D9 module %s\n",module);D3DDISPLAYMODE display{};d9->GetAdapterDisplayMode(0,&display);printf("display %ux%u format=%u\n",display.Width,display.Height,(unsigned)display.Format);D3DPRESENT_PARAMETERS pp{};pp.BackBufferCount=1;pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=wnd;
 pp.BackBufferWidth=W;pp.BackBufferHeight=H;pp.BackBufferFormat=D3DFMT_UNKNOWN;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 IDirect3DDevice9* dev=nullptr;HRESULT created;
 if(ex){IDirect3DDevice9Ex* de=nullptr;created=d9ex->CreateDeviceEx(0,D3DDEVTYPE_HAL,wnd,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&de);dev=de;}
 else created=d9->CreateDevice(0,D3DDEVTYPE_HAL,wnd,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,&dev);printf("device hr=%08lX ex=%d\n",(unsigned long)created,(int)ex);if(!ex&&created==D3DERR_INVALIDCALL){
  puts("NOT RUN: legacy D3D9 HAL device rejected by host; D3D9Ex pixel test is separate");d9->Release();DestroyWindow(wnd);return false;
 }
 require(SUCCEEDED(created),"device");
 ID3DBlob* vs=compile("row_major float4x4 m:register(c0); float4 main(float4 p:POSITION):POSITION{return mul(p,m);}","vs_2_0");
 ID3DBlob* ps=compile("float4 main():COLOR{return float4(0,1,0,1);}","ps_2_0");
 IDirect3DVertexShader9* v=nullptr;IDirect3DPixelShader9* p=nullptr;
 require(SUCCEEDED(dev->CreateVertexShader((DWORD*)vs->GetBufferPointer(),&v)),"vs");require(SUCCEEDED(dev->CreatePixelShader((DWORD*)ps->GetBufferPointer(),&p)),"ps");vs->Release();ps->Release();
 const float m[16]={1,0,0,0, 0,1,0,0, 0,0,1,1, 0,0,-.1f,0};float eyes[2][16];
 require(cinemastereo::EyeMatrix(m,0,-.24f,0,eyes[0])&&cinemastereo::EyeMatrix(m,0,.24f,1,eyes[1]),"native eye matrices");
 float transposed[16],col[16];for(int r=0;r<4;++r)for(int c=0;c<4;++c)transposed[r*4+c]=m[c*4+r];
 require(cinemastereo::EyeMatrix(transposed,1,-.24f,0,col),"column convention");
 for(int r=0;r<4;++r)for(int c=0;c<4;++c)require(fabs(col[r*4+c]-eyes[0][c*4+r])<1e-6f,"ROW/COL parity");
 require(!cinemastereo::EyeMatrix(m,3,0,0,col),"invalid convention");
 D3DVIEWPORT9 vp{0,0,W,H,0,1};RECT rect{1,2,319,159};dev->SetViewport(&vp);dev->SetScissorRect(&rect);dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
 dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetVertexShader(v);dev->SetPixelShader(p);dev->SetFVF(D3DFVF_XYZ);
 dev->SetVertexShaderConstantF(0,m,4);dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xff000000,1,0);
 const float vertices[9]={-.15f,-.3f,2, .15f,-.3f,2, 0,.3f,2};require(SUCCEEDED(dev->BeginScene()),"begin");
 require(SUCCEEDED(cinemastereo::Dispatch(dev,set,0,m,eyes,vp,rect,FALSE,[&](){return dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,vertices,12);})),"actual stereo dispatch");
 require(SUCCEEDED(dev->EndScene()),"end");float restored[16]{};dev->GetVertexShaderConstantF(0,restored,4);require(memcmp(m,restored,64)==0,"constants restored");
 RECT rr{};DWORD sc=1;dev->GetScissorRect(&rr);dev->GetRenderState(D3DRS_SCISSORTESTENABLE,&sc);require(memcmp(&rr,&rect,sizeof rect)==0&&!sc,"scissor restored");
 IDirect3DSurface9* target=nullptr,*read=nullptr;dev->GetRenderTarget(0,&target);D3DSURFACE_DESC desc{};target->GetDesc(&desc);require(SUCCEEDED(dev->CreateOffscreenPlainSurface(W,H,desc.Format,D3DPOOL_SYSTEMMEM,&read,nullptr)),"read surface");
 require(SUCCEEDED(dev->GetRenderTargetData(target,read)),"actual pixels");D3DLOCKED_RECT lr{};require(SUCCEEDED(read->LockRect(&lr,nullptr,D3DLOCK_READONLY)),"lock");
 unsigned count[2]{};double sums[2]{};for(int y=0;y<H;++y)for(int x=0;x<W;++x){const DWORD c=((DWORD*)((char*)lr.pBits+y*lr.Pitch))[x];if((c&0x00ff0000)==0&&(c&0x0000ff00)>0x00008000){++count[x/(W/2)];sums[x/(W/2)]+=x%(W/2);}}
 read->UnlockRect();require(count[0]>30&&count[1]>30,"both eyes real pixels");const double a=sums[0]/count[0],b=sums[1]/count[1];require(a-b>10,"independent eye disparity (not duplicated mono)");
 require(FAILED(cinemastereo::Dispatch(dev,set,0,m,eyes,vp,rect,FALSE,[](){return E_FAIL;})),"draw failure propagated");
 dev->GetVertexShaderConstantF(0,restored,4);dev->GetScissorRect(&rr);dev->GetRenderState(D3DRS_SCISSORTESTENABLE,&sc);
 require(memcmp(m,restored,64)==0&&memcmp(&rr,&rect,sizeof rect)==0&&!sc,"failed draw restores state");
 printf("%s pixels PASS: left=%u right=%u centroids=%.2f/%.2f; restored success/failure\n",ex?"D3D9Ex":"D3D9",count[0],count[1],a,b);
 read->Release();target->Release();v->Release();p->Release();dev->Release();d9->Release();DestroyWindow(wnd);return true;
}
static void conventionLifetime(){
 cinemastereo::NativeConvention witness;float x=0,y=0;
 auto shape=[](int c,float* tx,float* ty){*tx=1;*ty=.75f;return c==0;};
 require(witness.Resolve(shape,&x,&y)<0,"cold start must not guess convention");
 witness.Observe(false,0);
 require(witness.Resolve(shape,&x,&y)<0,"unverified gameplay matrix rejected");
 witness.Observe(true,0);
 // Reproduce native-camera early return followed by gameplay's 61-frame expiry.
 for(int frame=0;frame<180;++frame){
  witness.Observe(false,1);
  require(witness.Resolve(shape,&x,&y)==0,"cinema survives gameplay lock expiry");
 }
 require(x==1&&y==.75f,"current matrix aspect returned");
 require(witness.Resolve([](int,float*,float*){return false;},&x,&y)<0,"current malformed matrix rejected");
 witness.Observe(true,1);
 require(witness.Resolve(shape,&x,&y)<0,"new measured convention replaces old");
 witness.Reset();require(witness.Resolve(shape,&x,&y)<0,"reset clears convention evidence");
 puts("native cinema convention lifetime PASS (180 frames; expiry, shape, reset)");
}
int main(){conventionLifetime();require(run(true),"D3D9Ex coverage");const bool legacy=run(false);printf("cinema fixture: D3D9Ex PASS; legacy D3D9 %s; game/HMD NOT RUN\n",legacy?"PASS":"NOT RUN");}

