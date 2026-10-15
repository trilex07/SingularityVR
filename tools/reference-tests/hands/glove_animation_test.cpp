#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <chrono>
#include <new>
#include "../../../spikes/view_matrix/glove_animation.h"

static unsigned failures=0,checks=0;
static bool watch=false;
static std::size_t allocations=0;
void* operator new(std::size_t n){if(watch)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete[](void* p) noexcept{::operator delete(p);}
void operator delete[](void* p,std::size_t) noexcept{::operator delete(p);}
static void Check(bool ok,const char* what){++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",what);}}
static bool Near(float a,float b,float eps=1e-5f){return std::fabs(a-b)<eps;}
static void Push32(std::vector<unsigned char>& b,std::uint32_t v){const auto* p=reinterpret_cast<const unsigned char*>(&v);b.insert(b.end(),p,p+4);}
static std::vector<unsigned char> Fixture(){
    std::vector<unsigned char> b={'S','V','R','G'};Push32(b,1);Push32(b,3);Push32(b,3);
    for(unsigned i=0;i<3;++i){
        gloves::SourceVertex v;v.open={float(i)*.01f,.01f,-.1f};v.delta={0,-.02f,.03f};v.normal={0,1,0};
        const auto* p=reinterpret_cast<const unsigned char*>(&v);b.insert(b.end(),p,p+sizeof(v));
    }
    Push32(b,0);Push32(b,1);Push32(b,2);return b;
}
static bool Read(const char* path,std::vector<unsigned char>& bytes){
    FILE* f=nullptr;if(fopen_s(&f,path,"rb")||!f)return false;
    std::fseek(f,0,SEEK_END);const long n=std::ftell(f);std::rewind(f);
    if(n<=0||n>2*1024*1024){std::fclose(f);return false;}
    bytes.resize(n);const bool ok=std::fread(bytes.data(),1,n,f)==std::size_t(n);std::fclose(f);return ok;
}
int main(int argc,char** argv){
    using namespace gloves;
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    Check(Blend(1,true)==1,"full grip endpoint");Check(Blend(0,true)==0,"release endpoint");
    Check(Blend(.08f,true)==0,"deadzone edge");Check(Blend(.04f,true)==0,"deadzone noise");
    Check(Near(Blend(.54f,true),.5f),"linear analog rescale");
    for(float x:{nan,inf,-inf})Check(Blend(x,true)==0,"nonfinite opens");
    Check(Blend(1,false)==0,"inactive opens");Check(Blend(10,true)==1,"high clamps");
    Check(Blend(-1,true)==0,"low clamps");Check(Blend(1,true,1)==0,"invalid deadzone");
    struct Binding {int action,binding;};
    Binding bindings[]={{10,1},{90,2},{10,3},{91,4},{20,5},{90,6},{91,7}};
    Check(WithoutOptional(bindings,7,90,91)==3,"fallback strips only four new sources");
    Check(bindings[0].action==10&&bindings[0].binding==1&&bindings[1].action==10&&
        bindings[1].binding==3&&bindings[2].action==20&&bindings[2].binding==5,"original binding identity/order preserved");
    Q q;Check(!Normalize({0,0,0,0},q),"zero quaternion rejects");
    Check(!Normalize({nan,0,0,1},q),"NaN quaternion rejects");
    Check(Normalize({0,0,0,2},q)&&q.w==1,"quaternion normalized");
    const float s=std::sqrt(.5f);
    V v=Rotate({0,0,s,s},{1,0,0});Check(Near(v.x,0)&&Near(v.y,1),"roll rotates, no fixed wrist");
    v=Rotate({s,0,0,s},{0,0,-1});Check(Near(v.z,0)&&Near(v.y,1),"pitch rotates");
    v=Rotate({0,s,0,s},{0,0,-1});Check(Near(v.x,-1)&&Near(v.z,0),"yaw rotates");
    auto bytes=Fixture();Mesh mesh;
    Check(Decode(bytes.data(),bytes.size(),mesh),"valid cache");
    for(std::size_t n=0;n<bytes.size();++n)Check(!Decode(bytes.data(),n,mesh),"every truncation rejects");
    auto bad=bytes;bad.push_back(0);Check(!Decode(bad.data(),bad.size(),mesh),"trailing bytes reject");
    bad=bytes;bad[4]=2;Check(!Decode(bad.data(),bad.size(),mesh),"version rejects");
    bad=bytes;bad[8]=0xff;bad[9]=0xff;Check(!Decode(bad.data(),bad.size(),mesh),"bounded vertex count");
    bad=bytes;bad[bad.size()-4]=3;Check(!Decode(bad.data(),bad.size(),mesh),"out of range index");
    bad=bytes;std::memcpy(bad.data()+16,&nan,4);Check(!Decode(bad.data(),bad.size(),mesh),"NaN asset rejects");
    Check(mesh.source.size()==3,"failed decode preserves previous asset");
    Pose hand{},eye{};hand.position={.1f,.2f,-.5f};
    Check(Transform(mesh,0,hand,eye),"open transform");
    const Vertex open=mesh.scratch[0];
    Check(Near(open.p.x,.1f)&&Near(open.p.y,.21f)&&Near(open.p.z,.6f),"metres, XR to D3D depth");
    Check(Transform(mesh,1,hand,eye),"fist transform");
    Check(Near(mesh.scratch[0].p.y,.19f)&&Near(mesh.scratch[0].p.z,.57f),"fist delta actually consumed");
    Check(Transform(mesh,0,hand,eye)&&!std::memcmp(&open,&mesh.scratch[0],sizeof(Vertex)),"release restores bytes");
    hand.orientation={0,0,s,s};
    Check(Transform(mesh,0,hand,eye)&&Near(mesh.scratch[0].p.x,.09f),"controller roll reaches vertex");
    eye=hand;Check(Transform(mesh,0,hand,eye)&&Near(mesh.scratch[0].p.y,.01f),"inverse eye full quaternion");
    hand={};hand.position={0,0,-.5f};eye={};eye.position={-.032f,0,0};
    Transform(mesh,0,hand,eye);const float lx=mesh.scratch[0].p.x;
    eye.position={.032f,0,0};Transform(mesh,0,hand,eye);
    Check(Near(lx-mesh.scratch[0].p.x,.064f),"per-eye separation once, no double IPD");
    Check(!Transform(mesh,nan,hand,eye),"bad blend rejects");
    const auto eligible=[](int off){bool v[13];for(auto& b:v)b=true;bool p=false,m=false,c=false,w=false,g=false;
        if(off==0)v[0]=false;if(off==1)v[1]=false;if(off==2)v[2]=false;if(off==3)p=true;if(off==4)m=true;
        if(off==5)c=true;if(off==6)v[6]=false;if(off==7)v[7]=false;if(off==8)v[8]=false;
        if(off==9)v[9]=false;if(off==10)v[10]=false;if(off==11)w=true;if(off==12)g=true;
        return Eligible(v[0],v[1],v[2],p,m,c,v[6],v[7],v[8],v[9],v[10],w,g);};
    Check(eligible(-1),"explicit free-hand eligibility");
    for(int i=0;i<13;++i)Check(!eligible(i),"each safety boundary excludes replacement");
    Check(ReplaceSection(true,3314,0,4830,true,false,5,225,230,true),"known native noTMD draw");
    Check(!ReplaceSection(true,5799,0,4830,true,false,5,225,230,true),"gun never suppressed");
    Check(!ReplaceSection(true,7444,0,4402,true,false,5,225,230,true),"TMD not changed");
    Check(!ReplaceSection(true,3314,0,4830,true,true,5,225,230,true),"shadow not changed");
    float proj[4][4];Check(Projection(1,1,.01f,20,proj),"projection valid");
    const auto depth=[&](float z){return (z*proj[2][2]+proj[3][2])/z;};
    Check(Near(depth(.01f),0)&&Near(depth(20),1),"D3D near/far 0..1");
    Check(!Projection(nan,1,.01f,20,proj),"bad projection rejects");
    for(int a=1;a<argc;++a){
        std::vector<unsigned char> b;Check(Read(argv[a],b),"read local prepared cache");Mesh real;
        Check(Decode(b.data(),b.size(),real),"real asset validates");
        if(real.source.empty())continue;
        const auto* storage=real.scratch.data();allocations=0;watch=true;
        const auto start=std::chrono::steady_clock::now();
        bool ok=true;
        for(int frame=0;frame<3600;++frame){
            const float blend=Blend(float(frame%101)/100,true);
            for(int e=0;e<2;++e){eye.position={e?.032f:-.032f,0,0};ok=Transform(real,blend,hand,eye)&&ok;}
        }
        const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();watch=false;
        Check(ok,"3600-frame analog/eye sweep");Check(!allocations&&real.scratch.data()==storage,"no blend heap work or swaps");
        std::printf("CPU cache %s: vertices=%zu tris=%zu, 7200 transforms %.3f ms, %.4f ms/pair, allocations=%zu\n",
            argv[a],real.source.size(),real.indices.size()/3,elapsed,elapsed/3600,allocations);
    }
    std::printf("glove animation: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
