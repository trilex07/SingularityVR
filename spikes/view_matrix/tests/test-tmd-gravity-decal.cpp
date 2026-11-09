#include "../tmd_gravity_decal.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
    using namespace tmdgravitydecal;
    const Draw captured{true,true,4,12,0,5161,6,15,0,1,9,1,0,1,1,4,1,1,2,1};
    assert(Drop(captured));
    auto d=captured;d.triangles=64;assert(Drop(d));
    d.triangles=65;assert(!Drop(d));d.triangles=5;assert(!Drop(d));
    d=captured;d.valid=false;assert(!Drop(d));
    d=captured;d.context=false;assert(!Drop(d));
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
