#pragma once
#include <cstdint>
#include <cstddef>
namespace tmdgravitydecal {
inline std::uint32_t ShaderFingerprint(const unsigned char* bytes,std::size_t size) {
    std::uint32_t hash=2166136261u;
    for(std::size_t i=0;i<size;++i)hash=(hash^bytes[i])*16777619u;
    return hash;
}
inline bool Shader(const unsigned char* bytes,std::size_t size) {
    return bytes&&size==2132&&ShaderFingerprint(bytes,size)==0x9ce7ca1du;
}

inline bool WorldDepthQuad(unsigned primitive,unsigned vertices,unsigned triangles) {
    return primitive==4&&vertices==4&&triangles==2;
}
constexpr unsigned kWorldDepthVertexBytes=372, kWorldDepthPixelBytes=136;
constexpr std::uint32_t kWorldDepthVertexCode=0x24717b60u, kWorldDepthPixelCode=0x37de15b0u;
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
