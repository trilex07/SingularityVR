#pragma once
#include "shadow_receiver_math.h"
// Experimental, default OFF. Exact privately captured shader identities; no
// game bytecode redistributed. Classification is bounded at successful binds.
namespace shadowfix {
struct Entry { IUnknown* shader; bool match; };
static Entry cache[1024]{};
static unsigned used=0;
static bool enabled=false,vsMatch=false,psMatch=false,fault=false;
static unsigned applied=0,rejected=0,restoreFailures=0;
inline void Release() {
    for(unsigned i=0;i<used;++i)cache[i].shader->Release();
    used=0;vsMatch=psMatch=false;
}
template<class T> bool Classify(T* shader,UINT expectedBytes,unsigned long long expectedHash) {
    if(!enabled || fault || !shader)return false;
    for(unsigned i=0;i<used;++i)if(cache[i].shader==shader)return cache[i].match;
    if(used==1024)return false;
    UINT size=0;DWORD code[442]{};bool match=false;
    if(SUCCEEDED(shader->GetFunction(nullptr,&size)) && size==expectedBytes) {
        UINT capacity=sizeof(code);
        if(SUCCEEDED(shader->GetFunction(code,&capacity)) && capacity==size) {
            unsigned long long hash=14695981039346656037ull;
            for(UINT i=0;i<size;++i){hash^=((unsigned char*)code)[i];hash*=1099511628211ull;}
            match=hash==expectedHash;
        }
    }
    shader->AddRef();cache[used++]={shader,match};return match;
}
inline void BindVS(IDirect3DVertexShader9* s){vsMatch=Classify(s,296,0xB54F75E0F6242945ull);}
inline void BindPS(IDirect3DPixelShader9* s){psMatch=Classify(s,1768,0x2CED334CFD96AB0Aull);}
inline void Poll(const char* logPath) {
    static DWORD last=0;const DWORD now=GetTickCount();if(last && now-last<1000)return;last=now;
    char path[MAX_PATH]{};strcpy_s(path,logPath);char* end=strrchr(path,'\\');if(!end)return;
    strcpy_s(end+1,sizeof(path)-(end+1-path),"shadow-receiver-fix.flag");
    // An abandoned opt-in expires after 20 minutes even without a guard.
    WIN32_FILE_ATTRIBUTE_DATA attr{};bool on=false;
    if(GetFileAttributesExA(path,GetFileExInfoStandard,&attr) && !(attr.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
        FILETIME time;GetSystemTimeAsFileTime(&time);ULARGE_INTEGER n{},w{};
        n.LowPart=time.dwLowDateTime;n.HighPart=time.dwHighDateTime;
        w.LowPart=attr.ftLastWriteTime.dwLowDateTime;w.HighPart=attr.ftLastWriteTime.dwHighDateTime;
        on=n.QuadPart>=w.QuadPart && n.QuadPart-w.QuadPart<20ull*60*10000000;
    }
    if(on!=enabled){enabled=on;vsMatch=psMatch=false;Log("SHADOW_RECEIVER_FIX enabled=%d (experimental)",enabled);}
    if(applied||rejected||restoreFailures){Log("SHADOW_RECEIVER_FIX applied=%u rejected=%u restoreFailures=%u fault=%d",applied,rejected,restoreFailures,fault);applied=rejected=restoreFailures=0;}
}
struct Scope {
    IDirect3DDevice9* dev;float saved[32];bool changed=false;
    Scope(IDirect3DDevice9* d,bool scene,UINT verts,UINT prims):dev(d) {
        if(!enabled||fault||!vsMatch||!psMatch||!scene||verts!=8||prims!=12)return;
        float vp[16],corrected[32];
        if(FAILED(dev->GetVertexShaderConstantF(0,vp,4))||FAILED(dev->GetPixelShaderConstantF(3,saved,8))||
           !shadowreceiver::Correct(vp,saved,saved+16,corrected,corrected+16)){++rejected;return;}
        // Restore even if a driver reports failure after a partial upload.
        changed=true;
        if(FAILED(dev->SetPixelShaderConstantF(3,corrected,8))){
            ++rejected;
            if(FAILED(dev->SetPixelShaderConstantF(3,saved,8))){++restoreFailures;fault=true;}
            changed=false;return;
        }
        ++applied;
    }
    ~Scope(){if(changed && FAILED(dev->SetPixelShaderConstantF(3,saved,8))){++restoreFailures;fault=true;}}
};
}
