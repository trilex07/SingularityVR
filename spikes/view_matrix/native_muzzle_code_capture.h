#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace muzzlecode {
// Private bounded pre-detour evidence; never callable/replacement code.
inline bool Write(const char* path,std::uint32_t wrapperAddress,const void* wrapper,
                  std::uint32_t methodAddress,const void* method){
    if(!path||!wrapper||!method||!wrapperAddress||!methodAddress)return false;
    const std::uint32_t header[]{0x31434d53,1,wrapperAddress,197,methodAddress,846};
    FILE* file=nullptr;
    if(fopen_s(&file,path,"wbx")||!file)return false;
    bool ok=std::fwrite(header,sizeof(header),1,file)==1;
    ok=ok&&std::fwrite(wrapper,197,1,file)==1;
    ok=ok&&std::fwrite(method,846,1,file)==1;
    if(std::fclose(file))ok=false;
    if(!ok)std::remove(path);
    return ok;
}
}
