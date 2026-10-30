#pragma once
#include <bcrypt.h>
#pragma comment(lib,"bcrypt.lib")
#include "seeker_targets_generated.h"
// Called exclusively by production CreateDevice/Reset on its owning thread.
// No worker publishes mutable data to the bind callback.
namespace seekerpreload {
inline DDSImage cpu[2];
inline IDirect3DTexture9* gpu[2]{};
inline IDirect3DDevice9* owner=nullptr; // borrowed; GPU references own their device
inline DWORD thread=0;
inline int mode=0;
inline bool loaded=false;
inline unsigned reads=0,uploads=0;
inline volatile LONG binds=0,substitutions=0;
inline void Release(){for(auto& p:gpu){if(p)p->Release();p=nullptr;}owner=nullptr;}
inline bool Pinned(const char* path,const char* expected){
    FILE* f=nullptr;if(fopen_s(&f,path,"rb")||!f)return false;
    ++reads;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
    if(n<128||n>12*1024*1024){fclose(f);return false;}
    std::vector<unsigned char> b(n);bool ok=fread(b.data(),1,n,f)==size_t(n);fclose(f);
    unsigned char digest[32]{};char hex[65]{};
    if(!ok||BCryptHash(BCRYPT_SHA256_ALG_HANDLE,nullptr,0,b.data(),ULONG(b.size()),digest,32)<0)return false;
    for(unsigned i=0;i<32;++i)sprintf_s(hex+i*2,3,"%02x",digest[i]);
    return !strcmp(hex,expected);
}
inline bool Validate(const DDSImage& d,unsigned index){
    if(d.w!=4096||d.h!=2048||d.levels!=13||d.mip.size()!=13||
       d.fmt!=(index?D3DFMT_DXT1:D3DFMT_DXT5))return false;
    size_t total=0;for(unsigned i=0;i<13;++i){
        size_t bytes=size_t(((std::max)(1u,d.w>>i)+3)/4)*(((std::max)(1u,d.h>>i)+3)/4)*(index?8:16);
        if(d.mip[i].size()!=bytes)return false;total+=bytes;
    }return total<=12*1024*1024;
}
inline void Lifecycle(IDirect3DDevice9* dev,unsigned event){
    if(event==1){Release();return;}
    if(!dev||!g_bridgeOwned||!g_enabled)return;
    if(event==0){
        Release();thread=GetCurrentThreadId();loaded=false;cpu[0]={};cpu[1]={};
        char ini[MAX_PATH];BuildPath("texture_override.ini",ini,sizeof(ini));
        mode=iniInt("general","seeker_material_mode",0,ini);if(mode<0||mode>2)mode=0;
        if(!mode)return;
        bool valid=true;
        for(unsigned i=0;i<2;++i){char path[MAX_PATH];BuildPath(seekerFiles[i],path,sizeof(path));
            valid=valid&&Pinned(path,seekerSha[i])&&LoadDDS(path,cpu[i])&&Validate(cpu[i],i);}
        loaded=valid;if(!valid){cpu[0]={};cpu[1]={};}
        Log("SEEKER preload mode=%d SHA/full-chain pair=%s; normal=DXT1 diffuse=DXT5; no PBR shader changes",mode,loaded?"READY":"INVALID-original-retained");
    }
    if(GetCurrentThreadId()!=thread||mode!=2||!loaded)return;
    Release();IDirect3DTexture9* staged[2]{};
    for(unsigned i=0;i<2;++i){++uploads;if(!CreateReplacement(dev,cpu[i],&staged[i])){
        for(auto p:staged)if(p)p->Release();return;}}
    gpu[0]=staged[0];gpu[1]=staged[1];owner=dev;
    Log("SEEKER device pair READY thread=%lu; upload outside bind",thread);
}
inline int Index(const char* name){return !strcmp(name,"ar_steerable_d")?0:!strcmp(name,"ar_steerable_n")?1:-1;}
inline IDirect3DBaseTexture9* Ready(IDirect3DDevice9* dev,int index,IDirect3DBaseTexture9* original){
    InterlockedIncrement(&binds);if(mode==2&&owner==dev&&thread==GetCurrentThreadId()&&gpu[index]){InterlockedIncrement(&substitutions);return gpu[index];}
    return original;
}
}
extern "C" void WINAPI SingularitySeekerLifecycleV1(IDirect3DDevice9* dev,unsigned event){seekerpreload::Lifecycle(dev,event);}
