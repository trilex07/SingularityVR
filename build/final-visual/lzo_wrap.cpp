#define main ignored_package_main
#include "C:/Users/Trilex/vr-mods/SingularityVR-final-visual/tools/uedecompress/uedecompress.cpp"
#undef main
extern "C" __declspec(dllexport) int decode_lzo(const unsigned char* src, unsigned long long n,unsigned char* out,unsigned long long cap){size_t used=0;const char*err=nullptr; if(!Lzo1xDecompress(src,size_t(n),out,size_t(cap),&used,&err))return -1;return int(used);}
