#include "../weapon_reload_animation.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <limits>
#include <string>
using namespace weaponreloadanim;
static armbind::Mat Identity(){armbind::Mat m;for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
static void Close(const armbind::Mat& a,const armbind::Mat& b,float tolerance=.00025f){
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)assert(std::fabs(a.m[r][c]-b.m[r][c])<=tolerance);
}
static std::vector<unsigned char> Synthetic(const Spec& s){
    const unsigned fields[]{1,unsigned(s.kind),s.vertices,0,s.triangles,s.indexHash,s.slots,s.root,s.tracks,s.frames};
    const float fps=float(s.fps);const auto identity=Identity();
    std::vector<unsigned char> bytes(112+72*s.tracks+28*s.tracks*s.frames);
    std::memcpy(bytes.data(),"SVWA",4);std::memcpy(bytes.data()+4,fields,sizeof(fields));std::memcpy(bytes.data()+44,&fps,4);
    std::memcpy(bytes.data()+48,identity.m,64);std::size_t cursor=112;
    for(unsigned i=0;i<s.tracks;++i){
        std::memcpy(bytes.data()+cursor,&s.target[i],4);std::memcpy(bytes.data()+cursor+4,&s.parent[i],4);
        std::memcpy(bytes.data()+cursor+8,identity.m,64);cursor+=72;
    }
    for(unsigned f=0;f<s.frames;++f)for(unsigned i=0;i<s.tracks;++i){
        float key[]{float(f)*.01f*(i+1),float(i)*.02f,0,0,0,0,(f%2)?-1.f:1.f};
        std::memcpy(bytes.data()+cursor,key,sizeof(key));cursor+=sizeof(key);
    }
    return bytes;
}
static void TestPalette(const Data& d,float frame){
    float native[16*12],out[16*12],expected[16*12];
    for(unsigned i=0;i<16*12;++i)native[i]=out[i]=expected[i]=-1234.f;
    auto parent=Identity();parent.m[0][0]=parent.m[1][1]=parent.m[2][2]=.75f;
    parent.m[3][0]=14.f;parent.m[3][1]=-9.f;parent.m[3][2]=33.f;
    armbind::ToGpu(parent,reinterpret_cast<float (*)[4]>(native+d.root*12));
    std::memcpy(expected,native,sizeof(expected));
    assert(!Palette(d,false,frame,native,d.slots*3,out));for(float v:out)assert(v==-1234.f);
    assert(!Palette(d,true,frame,native,d.slots*3-1,out));
    armbind::Mat delta[8];assert(Sample(d,frame,delta));
    for(unsigned i=0;i<d.tracks;++i){
        armbind::Mat target;skeletal::Multiply(target.m,delta[i].m,parent.m);
        armbind::ToGpu(target,reinterpret_cast<float (*)[4]>(expected+d.target[i]*12));
    }
    assert(Palette(d,true,frame,native,d.slots*3,out));
    assert(!std::memcmp(out,expected,d.slots*12*sizeof(float)));
    assert(!std::memcmp(native+d.root*12,out+d.root*12,12*sizeof(float))); // never animate root/aim
    for(unsigned i=d.slots*12;i<16*12;++i)assert(out[i]==-1234.f); // no other registers touched
    assert(Palette(d,true,frame,native,d.slots*3,native)); // aliased in-place output safe
    assert(!std::memcmp(native,expected,d.slots*12*sizeof(float)));
}
static void TestSynthetic(const Spec& s){
    const auto bytes=Synthetic(s);auto d=std::make_unique<Data>();
    assert(Decode(bytes.data(),bytes.size(),s.kind,*d));
    for(unsigned q=0;q<=(s.frames-1)*4;++q){
        armbind::Mat delta[8];const float frame=q*.25f;assert(Sample(*d,frame,delta));
        for(unsigned i=0;i<s.tracks;++i){
            auto expected=Identity();int ancestor=int(i);
            while(ancestor>=0){expected.m[3][0]+=frame*.01f*(ancestor+1);expected.m[3][1]+=ancestor*.02f;ancestor=s.parent[ancestor];}
            Close(delta[i],expected,.00002f);
        }
        if(q%17==0)TestPalette(*d,frame);
    }
    for(unsigned field=0;field<10;++field){auto bad=bytes;
        const auto original=U32(bad.data()+4+field*4);const unsigned altered=original+1;
        std::memcpy(bad.data()+4+field*4,&altered,4);
        assert(!Decode(bad.data(),bad.size(),s.kind,*d)&&d->ready); // failed decode does not clobber prior
    }
    for(const auto& other:specs)if(other.kind!=s.kind)assert(!Decode(bytes.data(),bytes.size(),other.kind,*d));
    assert(!Decode(bytes.data(),bytes.size()-1,s.kind,*d));
    assert(!Decode(nullptr,bytes.size(),s.kind,*d));
    for(unsigned pos:{112u,116u,120u,112u+72u*s.tracks,112u+72u*s.tracks+24u}){
        auto bad=bytes;const unsigned nan=0x7fc00000u;std::memcpy(bad.data()+pos,&nan,4);
        assert(!Decode(bad.data(),bad.size(),s.kind,*d));
    }
    armbind::Mat out[8];for(auto& m:out)for(auto& r:m.m)for(float& v:r)v=321.f;
    assert(!Sample(*d,-1,out)&&out[0].m[0][0]==321.f);
    assert(!Sample(*d,float(s.frames),out));
    assert(!Sample(*d,std::numeric_limits<float>::quiet_NaN(),out));
}
static std::vector<unsigned char> Read(const std::string& path){
    FILE* f=nullptr;assert(fopen_s(&f,path.c_str(),"rb")==0&&f);
    assert(!std::fseek(f,0,SEEK_END));long size=std::ftell(f);assert(size>0&&size<1024*1024);std::rewind(f);
    std::vector<unsigned char> b((std::size_t)size);assert(std::fread(b.data(),1,b.size(),f)==b.size());std::fclose(f);return b;
}
static unsigned TestPrivate(const Spec& s,const std::string& root,const char* name){
    const auto bytes=Read(root+"/"+name+"/"+name+"_reload.svwa");auto d=std::make_unique<Data>();
    assert(Decode(bytes.data(),bytes.size(),s.kind,*d));
    const auto golden=Read(root+"/"+name+"/"+name+"_oracle.svwo");
    assert(golden.size()>=20&&!std::memcmp(golden.data(),"SVWO",4));
    assert(U32(golden.data()+4)==1&&U32(golden.data()+8)==unsigned(s.kind)&&U32(golden.data()+12)==s.tracks);
    const unsigned samples=U32(golden.data()+16);assert(samples==(s.frames-1)*4+1);
    assert(golden.size()==20+samples*(4+s.tracks*64));
    std::size_t cursor=20;
    for(unsigned k=0;k<samples;++k){
        float frame;std::memcpy(&frame,golden.data()+cursor,4);cursor+=4;assert(frame==float(k)*.25f);
        armbind::Mat delta[8];assert(Sample(*d,frame,delta));
        for(unsigned i=0;i<s.tracks;++i){armbind::Mat expected;std::memcpy(expected.m,golden.data()+cursor,64);cursor+=64;
            assert(armbind::Rigid(delta[i]));Close(delta[i],expected);}
        TestPalette(*d,frame);
    }
    std::printf("private %s original Reload: %u fractional track poses + preserved root/parent scale PASS\n",name,samples*s.tracks);
    return samples*s.tracks;
}
int main(int argc,char** argv){
    for(const auto& s:specs)TestSynthetic(s);
    unsigned poses=0;
    if(argc>1){assert(argc==2);const char* names[]{"valkyrie","volk","kasimov"};
        for(unsigned i=0;i<3;++i)poses+=TestPrivate(specs[i],argv[1],names[i]);}
    std::printf("L1 three-weapon animation PASS: hierarchy/shortest-quaternion/root/parent/aliasing/transactional rejection; original poses=%u; pure consumer fixture, NOT native draws\n",poses);
}
