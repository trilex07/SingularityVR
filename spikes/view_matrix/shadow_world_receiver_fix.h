#pragma once
#include "shadow_receiver_fix.h"
#include "shadow_world_receiver_math.h"
// Validated world-depth correction, configured by INI (ON by default in loader).
// Separate from the rejected mixed receiver; bounded diagnostic lease optional.
namespace worldshadowfix {
struct Entry {IUnknown* shader=nullptr;bool match=false;};
static Entry cache[128];static unsigned used=0;
static bool enabled=false,vsMatch=false,psMatch=false,fault=false;
static volatile LONG configured=0;
inline void Configure(bool on){InterlockedExchange(&configured,on?1:0);}
inline bool Requested(bool lease){return lease||InterlockedCompareExchange(&configured,0,0)!=0;}
static unsigned appliedLeft=0,appliedRight=0,nearSkipped=0,rejected=0,restoreFailures=0;
inline void Release(){for(unsigned i=0;i<used;++i)cache[i].shader->Release();used=0;vsMatch=psMatch=false;}
template<class T> bool Classify(T* shader,UINT bytes,unsigned long long fingerprint){
    if(!enabled||fault||!shader)return false;
    for(unsigned i=0;i<used;++i)if(cache[i].shader==shader)return cache[i].match;
    if(used==128)return false;
    DWORD code[442]{};UINT size=0;bool match=false;
    if(SUCCEEDED(shader->GetFunction(nullptr,&size)) && size==bytes){
        UINT capacity=sizeof(code);
        if(SUCCEEDED(shader->GetFunction(code,&capacity))&&capacity==size){
            unsigned long long hash=14695981039346656037ull;
            for(UINT i=0;i<size;++i){hash^=((unsigned char*)code)[i];hash*=1099511628211ull;}
            match=hash==fingerprint;
        }
    }
    shader->AddRef();cache[used++]={shader,match};return match;
}
inline void BindVS(IDirect3DVertexShader9* s){vsMatch=Classify(s,296,0xB54F75E0F6242945ull);}
inline void BindPS(IDirect3DPixelShader9* s){psMatch=Classify(s,1768,0x2CED334CFD96AB0Aull);}
inline void Poll(const char* logPath){
    static DWORD last=0;DWORD now=GetTickCount();if(last&&now-last<1000)return;last=now;
    char path[MAX_PATH]{};strcpy_s(path,logPath);char* end=strrchr(path,'\\');if(!end)return;
    strcpy_s(end+1,sizeof(path)-(end+1-path),"shadow-world-receiver.flag");
    WIN32_FILE_ATTRIBUTE_DATA attr{};bool on=false;
    if(GetFileAttributesExA(path,GetFileExInfoStandard,&attr)&&!(attr.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){
        FILETIME time;GetSystemTimeAsFileTime(&time);ULARGE_INTEGER n{},w{};
        n.LowPart=time.dwLowDateTime;n.HighPart=time.dwHighDateTime;
        w.LowPart=attr.ftLastWriteTime.dwLowDateTime;w.HighPart=attr.ftLastWriteTime.dwHighDateTime;
        if(n.QuadPart>=w.QuadPart&&n.QuadPart-w.QuadPart<20ull*60*10000000){
            FILE* f=nullptr;char payload[64]{};
            if(!fopen_s(&f,path,"rb")&&f){size_t length=fread(payload,1,sizeof(payload),f);fclose(f);
                const char expected[]="SingularityVR world shadow reconstruction v1";
                on=length==sizeof(expected)-1 && memcmp(payload,expected,length)==0;
            }
        }
    }
    on=Requested(on);
    if(on!=enabled){enabled=on;vsMatch=psMatch=false;Log("WORLD_SHADOW_RECONSTRUCTION enabled=%d",enabled?1:0);}
    if(appliedLeft||appliedRight||nearSkipped||rejected||restoreFailures){
        Log("WORLD_SHADOW_RECONSTRUCTION left=%u right=%u nearSkipped=%u rejected=%u restoreFailures=%u fault=%d",
            appliedLeft,appliedRight,nearSkipped,rejected,restoreFailures,fault?1:0);
        appliedLeft=appliedRight=nearSkipped=rejected=restoreFailures=0;
    }
}
inline bool Resources(IDirect3DDevice9* dev,bool& right){
    IDirect3DBaseTexture9* tex[2]{};IDirect3DSurface9* target=nullptr;
    D3DSURFACE_DESC a{},b{},rt{};RECT sc{};bool valid=false;
    const HRESULT first=dev->GetTexture(0,&tex[0]),second=dev->GetTexture(1,&tex[1]);
    const HRESULT targetHr=dev->GetRenderTarget(0,&target);
    if(SUCCEEDED(first)&&SUCCEEDED(second)&&SUCCEEDED(targetHr)&&tex[0]&&tex[1]&&target&&
       tex[0]->GetType()==D3DRTYPE_TEXTURE&&tex[1]->GetType()==D3DRTYPE_TEXTURE&&
       SUCCEEDED(static_cast<IDirect3DTexture9*>(tex[0])->GetLevelDesc(0,&a))&&
       SUCCEEDED(static_cast<IDirect3DTexture9*>(tex[1])->GetLevelDesc(0,&b))&&
       SUCCEEDED(target->GetDesc(&rt))&&SUCCEEDED(dev->GetScissorRect(&sc))){
        const LONG half=(LONG)(rt.Width/2);
        const bool left=sc.left==0&&sc.right==half;
        right=sc.left==half&&sc.right==(LONG)rt.Width;
        valid=rt.Width>1&&!(rt.Width%2)&&a.Width==rt.Width&&a.Height==rt.Height&&
            rt.Format==D3DFMT_A16B16G16R16F&&a.Format==D3DFMT_A16B16G16R16F&&
            b.Format==D3DFMT_R32F&&b.Width==512&&b.Height==512&&
            sc.top==0&&sc.bottom==(LONG)rt.Height&&(left||right);
    }
    for(auto t:tex)if(t)t->Release();if(target)target->Release();return valid;
}
struct Scope {
    IDirect3DDevice9* dev;float saved[32]{};bool changed=false;
    Scope(IDirect3DDevice9* d,bool scene,UINT verts,UINT prims,unsigned moved):dev(d){
        if(!enabled||fault||shadowfix::enabled||!vsMatch||!psMatch||!scene||
           moved||verts!=8||prims!=12)return;
        float depth[4]{},vp[16]{},corrected[32]{};bool right=false;
        if(FAILED(dev->GetPixelShaderConstantF(2,depth,1))){++rejected;return;}
        if(!worldshadowmath::WorldDepth(depth)){++nearSkipped;return;}
        if(!Resources(dev,right)||FAILED(dev->GetVertexShaderConstantF(0,vp,4))||
           FAILED(dev->GetPixelShaderConstantF(3,saved,8))||
           !worldshadowmath::Correct(vp,saved,saved+16,corrected,corrected+16)){++rejected;return;}
        changed=true;
        if(FAILED(dev->SetPixelShaderConstantF(3,corrected,8))){
            ++rejected;
            if(FAILED(dev->SetPixelShaderConstantF(3,saved,8))){++restoreFailures;fault=true;}
            changed=false;return;
        }
        if(right)++appliedRight;else ++appliedLeft;
    }
    ~Scope(){if(changed&&FAILED(dev->SetPixelShaderConstantF(3,saved,8))){++restoreFailures;fault=true;}}
};
}
