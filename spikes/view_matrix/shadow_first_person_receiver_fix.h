#pragma once
#include "shadow_world_receiver_fix.h"
#include "shadow_clear_gate.h"
// Default-OFF near1 receiver experiment. Requires the physically tested
// world/ClearGate combination; never edits a mesh palette or gun transform.
namespace fpshadowfix {
static bool enabled=false,fault=false;
static unsigned appliedLeft=0,appliedRight=0,depthSkipped=0,rejected=0,restoreFailures=0;
inline bool FirstDepth(const float* depth){
    for(unsigned i=0;i<4;++i)if(!std::isfinite(depth[i]))return false;
    return depth[0]>0 && depth[1]>=0 && depth[1]<1 &&
        std::fabs(depth[0]/(1-depth[1])-1.f)<0.005f &&
        std::fabs(depth[2]*depth[0]-1.f)<1e-4f &&
        std::fabs(depth[3]*depth[0]-depth[1])<1e-5f;
}
inline void Poll(const char* logPath){
    static DWORD last=0;DWORD now=GetTickCount();if(last&&now-last<1000)return;last=now;
    char path[MAX_PATH]{};strcpy_s(path,logPath);char* end=strrchr(path,'\\');if(!end)return;
    strcpy_s(end+1,sizeof(path)-(end+1-path),"shadow-first-person-receiver.flag");
    WIN32_FILE_ATTRIBUTE_DATA attr{};bool on=false;
    if(GetFileAttributesExA(path,GetFileExInfoStandard,&attr)&&!(attr.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){
        FILETIME time;GetSystemTimeAsFileTime(&time);ULARGE_INTEGER n{},w{};
        n.LowPart=time.dwLowDateTime;n.HighPart=time.dwHighDateTime;
        w.LowPart=attr.ftLastWriteTime.dwLowDateTime;w.HighPart=attr.ftLastWriteTime.dwHighDateTime;
        if(n.QuadPart>=w.QuadPart&&n.QuadPart-w.QuadPart<20ull*60*10000000){
            FILE* f=nullptr;char payload[64]{};
            if(!fopen_s(&f,path,"rb")&&f){size_t length=fread(payload,1,sizeof(payload),f);fclose(f);
                const char expected[]="SingularityVR first person shadow reconstruction v1";
                on=length==sizeof(expected)-1 && memcmp(payload,expected,length)==0;
            }
        }
    }
    if(on!=enabled){enabled=on;Log("FP_SHADOW_RECONSTRUCTION enabled=%d",enabled?1:0);}
    if(appliedLeft||appliedRight||depthSkipped||rejected||restoreFailures){
        Log("FP_SHADOW_RECONSTRUCTION left=%u right=%u depthSkipped=%u rejected=%u restoreFailures=%u fault=%d",
            appliedLeft,appliedRight,depthSkipped,rejected,restoreFailures,fault?1:0);
        appliedLeft=appliedRight=depthSkipped=rejected=restoreFailures=0;
    }
}
struct Scope {
    IDirect3DDevice9* dev;float saved[32]{};bool changed=false;
    Scope(IDirect3DDevice9* d,bool scene,UINT verts,UINT prims,unsigned moved):dev(d){
        if(!enabled||fault||!worldshadowfix::enabled||worldshadowfix::fault||
           !shadowcleargate::Enabled()||shadowfix::enabled||!worldshadowfix::vsMatch||
           !worldshadowfix::psMatch||!scene||moved||verts!=8||prims!=12)return;
        float depth[4]{},vp[16]{},corrected[32]{};bool right=false;
        if(FAILED(dev->GetPixelShaderConstantF(2,depth,1))){++rejected;return;}
        if(!FirstDepth(depth)){++depthSkipped;return;}
        if(!worldshadowfix::Resources(dev,right)||FAILED(dev->GetVertexShaderConstantF(0,vp,4))||
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
    Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
    ~Scope(){if(changed&&FAILED(dev->SetPixelShaderConstantF(3,saved,8))){++restoreFailures;fault=true;}}
};
}
