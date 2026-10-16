#pragma once
// Original procedural artwork. XR metres in a horizontal plane, no textures,
// input changes, heap work, time pulses or engine dependencies.
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "ui_pointer.h"

namespace holstermarker {
enum class Item { Gun, Knife };
inline bool Eligible(bool gameplay,bool paused,bool menu,bool panel,bool cinematic,bool focused){
    return gameplay&&!paused&&!menu&&!panel&&!cinematic&&focused;
}
struct Vertex { float x,z; std::uint32_t colour; };
struct Mesh {
    static constexpr unsigned capacity=1536;
    Vertex vertices[capacity]{};unsigned count=0;
    bool Triangle(Vertex a,Vertex b,Vertex c){
        if(count+3>capacity)return false;
        vertices[count++]=a;vertices[count++]=b;vertices[count++]=c;return true;
    }
};
inline float Clamp(float x){return std::isfinite(x)?(x<0?0:(x>1?1:x)):0;}
inline float Proximity(uipointer::V centre,uipointer::V hand,bool valid){
    if(!valid||!uipointer::Finite(centre)||!uipointer::Finite(hand))return 0;
    const auto d=centre-hand;const float distance=std::sqrt(uipointer::Dot(d,d));
    const float t=Clamp((.45f-distance)/.27f);return t*t*(3-2*t);
}
inline std::uint32_t Colour(unsigned rgb,float alpha){
    return ((std::uint32_t)std::lround(255*Clamp(alpha))<<24)|(rgb&0xffffffu);
}
inline void Quad(Mesh& m,Vertex a,Vertex b,Vertex c,Vertex d){m.Triangle(a,b,c);m.Triangle(c,b,d);}
inline void Arc(Mesh& m,float inner,float outer,float begin,float end,unsigned segments,std::uint32_t colour){
    for(unsigned i=0;i<segments;++i){
        const float a=begin+(end-begin)*i/segments,b=begin+(end-begin)*(i+1)/segments;
        Quad(m,{std::cos(a)*inner,std::sin(a)*inner,colour},{std::cos(a)*outer,std::sin(a)*outer,colour},
               {std::cos(b)*inner,std::sin(b)*inner,colour},{std::cos(b)*outer,std::sin(b)*outer,colour});
    }
}
inline void Stroke(Mesh& m,float x,float z,float xx,float zz,float width,std::uint32_t colour){
    const float dx=xx-x,dz=zz-z,length=std::sqrt(dx*dx+dz*dz);if(length<1e-6f)return;
    const float nx=-dz/length*width*.5f,nz=dx/length*width*.5f;
    Quad(m,{x+nx,z+nz,colour},{x-nx,z-nz,colour},{xx+nx,zz+nz,colour},{xx-nx,zz-nz,colour});
}
inline void Build(Item item,float proximity,Mesh& mesh){
    mesh.count=0;const float proximityWeight=Clamp(proximity),pi=3.14159265358979323846f;
    const auto edge=Colour(0xe8ded0u,.32f+.48f*proximityWeight);
    const auto accent=Colour(proximityWeight>.5f?0xf0c982u:0x9baeb6u,.30f+.50f*proximityWeight);
    // Same 22.54 cm outer footprint as the accepted ring. Thin dark keyline,
    // four broken arcs, inner guide and orientation ticks. Empty centre stays.
    Arc(mesh,.1035f,.1127f,0,2*pi,48,Colour(0x121b20u,.24f+.12f*proximityWeight));
    for(unsigned arc=0;arc<4;++arc){
        const float a=arc*pi*.5f+pi/18,b=(arc+1)*pi*.5f-pi/18;
        Arc(mesh,.1075f,.1115f,a,b,10,edge);
        Arc(mesh,.1015f,.104f,a,b,10,Colour(0xc6b080u,.12f+.22f*proximityWeight));
    }
    Arc(mesh,.0877f,.0892f,0,2*pi,48,Colour(0x8ea1a9u,.12f+.20f*proximityWeight));
    for(unsigned tick=0;tick<4;++tick){
        const float a=tick*pi*.5f;
        Stroke(mesh,std::cos(a)*.095f,std::sin(a)*.095f,std::cos(a)*.1115f,std::sin(a)*.1115f,.003f,accent);
    }
    const float w=.0025f;
    if(item==Item::Gun){
        Stroke(mesh,-.025f,-.018f,.024f,-.018f,w,edge);
        Stroke(mesh,.024f,-.018f,.024f,-.002f,w,edge);
        Stroke(mesh,.024f,-.002f,-.004f,-.002f,w,edge);
        Stroke(mesh,-.004f,-.002f,-.010f,.027f,w,edge);
        Stroke(mesh,-.010f,.027f,-.025f,.021f,w,edge);
        Stroke(mesh,-.025f,.021f,-.018f,-.002f,w,edge);
        Stroke(mesh,-.018f,-.002f,-.025f,-.018f,w,edge);
    }else{
        Stroke(mesh,0,-.035f,.007f,-.019f,w,edge);Stroke(mesh,.007f,-.019f,.006f,.010f,w,edge);
        Stroke(mesh,.006f,.010f,-.006f,.010f,w,edge);Stroke(mesh,-.006f,.010f,0,-.035f,w,edge);
        Stroke(mesh,-.015f,.013f,.015f,.013f,.003f,accent);
        Stroke(mesh,0,.016f,0,.035f,.008f,edge);
    }
}
// Shared per-eye projection. A triangle that crosses the near plane is
// omitted, never collapsed to the centre or mirrored from the other eye.
struct ScreenVertex {float x,y,z,rhw;std::uint32_t colour;};
template<class Project> unsigned ProjectMesh(const Mesh& mesh,uipointer::V centre,
        float yaw,int eye,unsigned width,unsigned height,Project&& project,ScreenVertex* out,unsigned capacity){
    if(!out||capacity<mesh.count||eye<0||eye>1||width<8||height<8||!uipointer::Finite(centre)||!std::isfinite(yaw))return 0;
    const float cs=std::cos(yaw),sn=std::sin(yaw);unsigned count=0;
    for(unsigned i=0;i+2<mesh.count;i+=3){
        ScreenVertex triangle[3];bool valid=true;
        for(unsigned j=0;j<3;++j){
            const auto& v=mesh.vertices[i+j];
            const uipointer::V p{centre.x+cs*v.x-sn*v.z,centre.y,centre.z+sn*v.x+cs*v.z};
            uipointer::ScreenPoint s;
            if(!project(eye,p,s)||s.depth<.01f||!std::isfinite(s.u)||!std::isfinite(s.v)||std::fabs(s.u)>100||std::fabs(s.v)>100){valid=false;break;}
            triangle[j]={(eye+s.u)*width*.5f,s.v*height,0,1,v.colour};
        }
        if(valid)for(const auto& v:triangle)out[count++]=v;
    }
    return count;
}
// Caller adapter captures/restores ALL D3D state. Successful Begin is ALWAYS
// paired with End, including missing geometry and failed draw calls.
template<class Pass,class Project> bool Render(Pass& pass,const Mesh& mesh,uipointer::V centre,
        float yaw,unsigned width,unsigned height,Project&& project){
    if(!pass.Begin(width,height))return false;
    bool drew=false;ScreenVertex vertices[Mesh::capacity];
    for(int eye=0;eye<2;++eye){
        const unsigned count=ProjectMesh(mesh,centre,yaw,eye,width,height,project,vertices,Mesh::capacity);
        if(count&&pass.Eye(eye,width,height)&&pass.Draw(count,vertices))drew=true;
    }
    return pass.End()&&drew;
}
} // namespace holstermarker
