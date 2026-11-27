#pragma once
#include <cstdint>
#include <cstddef>
namespace tmdgravitydecal {
inline bool Shader(const unsigned char* bytes,std::size_t n){
    if(!bytes||n!=2132)return false;
    std::uint32_t h=2166136261u;for(std::size_t i=0;i<n;++i)h=(h^bytes[i])*16777619u;
    return h==0x9ce7ca1du;
}
struct Draw {bool valid,context;unsigned primitive,stride,offset,vertices,triangles;
    unsigned colour,zWrite,blend,src,dst,stencil,op,zEnable,zFunc,separateAlpha,srcAlpha,dstAlpha,alphaOp;};
inline bool CapturedGeometry(unsigned primitive, unsigned vertices, unsigned triangles) {
    return primitive == 4 && vertices == 5161 && triangles >= 6 && triangles <= 64;
}
inline bool Drop(const Draw& d){return d.valid&&d.context&&CapturedGeometry(d.primitive,d.vertices,d.triangles)&&
    d.stride==12&&d.offset==0&&d.colour==15&&!d.zWrite&&
    d.blend==1&&d.src==9&&d.dst==1&&!d.stencil&&d.op==1&&d.zEnable==1&&d.zFunc==4&&
    d.separateAlpha==1&&d.srcAlpha==1&&d.dstAlpha==2&&d.alphaOp==1;}
}
