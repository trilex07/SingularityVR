#include "../tmd_gravity_decal.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
    using namespace tmdgravitydecal;
    // The wreck uses the same depth shaders as the misrouted TMD quads.
    // Its hull and rotor draws must fail the geometry gate even with those shaders bound.
    assert(WorldDepthQuad(4, 4, 2));
    assert(!WorldDepthQuad(4, 3440, 4093)); // Huey hull, trace 35252, draws 27–28.
    assert(!WorldDepthQuad(4, 1345, 1422)); // Rotor blades, draws 755–756.
    assert(!WorldDepthQuad(5, 4, 2));
    assert(!WorldDepthQuad(4, 4, 3));
    assert(!WorldDepthQuad(4, 5, 2));
    puts("World depth quad and captured wreck exclusions PASS");

    const Draw captured{true,true,4,12,0,5161,6,15,0,1,9,1,0,1,1,4,1,1,2,1};
    assert(Drop(captured));
    auto d=captured;d.triangles=64;assert(Drop(d));
    d.triangles=65;assert(!Drop(d));d.triangles=5;assert(!Drop(d));
    d=captured;d.valid=false;assert(!Drop(d));
    d=captured;d.context=false;assert(!Drop(d));
    // Actual sky7243 groups462/463 are a different class: nine vertices,
    // eight primitives, stride32 and RGB-only opaque material. Never hide
    // that geometry by treating this gravity-decal experiment as a sky fix.
    d=captured;d.vertices=9;d.triangles=8;d.stride=32;d.colour=7;d.blend=0;
    assert(!Drop(d));
    // All captured range endpoints are admitted, but no widened range.
    for(unsigned n=6;n<=64;++n){d=captured;d.triangles=n;assert(Drop(d));}
    unsigned Draw::*fields[]={&Draw::primitive,&Draw::stride,&Draw::offset,&Draw::vertices,
        &Draw::colour,&Draw::zWrite,&Draw::blend,&Draw::src,&Draw::dst,&Draw::stencil,
        &Draw::op,&Draw::zEnable,&Draw::zFunc,&Draw::separateAlpha,&Draw::srcAlpha,
        &Draw::dstAlpha,&Draw::alphaOp};
    for(auto field:fields){d=captured;++(d.*field);assert(!Drop(d));}
    unsigned char bytes[2132]{};
    assert(!Shader(nullptr,sizeof bytes));assert(!Shader(bytes,sizeof bytes));
    if(argc>1){
        FILE* f=nullptr;assert(fopen_s(&f,argv[1],"rb")==0);
        assert(fread(bytes,1,sizeof bytes,f)==sizeof bytes);assert(fgetc(f)==EOF);fclose(f);
        assert(Shader(bytes,sizeof bytes));assert(!Shader(bytes,sizeof bytes-1));
        for(unsigned i=0;i<sizeof bytes;++i){bytes[i]^=1;assert(!Shader(bytes,sizeof bytes));bytes[i]^=1;}
        puts("Captured shader positive + 2132 single-byte mutations PASS");
    }
    puts("Captured draw and all individual gate rejections PASS");
}
