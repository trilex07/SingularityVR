#include "../holster_marker.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>

using namespace holstermarker;
static bool Project(int eye,uipointer::V p,uipointer::ScreenPoint& s){
    return uipointer::ProjectPoint(p,{eye?.032f:-.032f,0,0},{0,0,-1},{1,0,0},{0,1,0},1,1,s);
}
struct FakePass {
    int captures=0,restores=0,draws=0,eyes=0;bool capture=true,draw=true,restore=true;
    bool Begin(unsigned,unsigned){++captures;return capture;}
    bool Eye(int eye,unsigned,unsigned){assert(eye==eyes++);return true;}
    bool Draw(unsigned count,const ScreenVertex*){assert(count%3==0);++draws;return draw;}
    bool End(){++restores;return restore;}
};
int main(int argc,char** argv){
    for(unsigned mask=0;mask<64;++mask)assert(Eligible(mask&1,mask&2,mask&4,mask&8,mask&16,mask&32)==(mask==33));
    for(auto item:{Item::Gun,Item::Knife})for(int n=0;n<=100;++n){
        Mesh m;Build(item,n*.01f,m);assert(m.count>1000&&m.count<Mesh::capacity&&m.count%3==0);
        for(unsigned i=0;i<m.count;++i){
            const auto& v=m.vertices[i];assert(std::isfinite(v.x)&&std::isfinite(v.z));
            assert(v.x*v.x+v.z*v.z<.112701f*.112701f);assert((v.colour>>24)>0&&(v.colour>>24)<=204);
        }
        for(unsigned i=0;i<m.count;i+=3){
            const auto a=m.vertices[i],b=m.vertices[i+1],c=m.vertices[i+2];
            assert(std::fabs((b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x))>1e-10f);
        }
        Mesh repeat;Build(item,n*.01f,repeat);assert(m.count==repeat.count);
        assert(!std::memcmp(m.vertices,repeat.vertices,m.count*sizeof(Vertex)));
    }
    const uipointer::V centre{.25f,-.6f,-.5f};
    assert(Proximity(centre,centre,true)==1);assert(Proximity(centre,centre,false)==0);
    assert(Proximity(centre,{1,1,1},true)==0);
    assert(Proximity(centre,{NAN,1,1},true)==0);
    float last=1;for(unsigned i=0;i<100;++i){const float p=Proximity(centre,{centre.x+i*.01f,centre.y,centre.z},true);assert(p<=last);last=p;}
    Mesh m;Build(Item::Gun,.5f,m);ScreenVertex l[Mesh::capacity],r[Mesh::capacity];
    for(float yaw:{0.f,.8f,3.14f}){
        const auto ln=ProjectMesh(m,centre,yaw,0,4000,2000,Project,l,Mesh::capacity);
        const auto rn=ProjectMesh(m,centre,yaw,1,4000,2000,Project,r,Mesh::capacity);
        assert(ln==m.count&&rn==m.count);
        for(unsigned i=0;i<ln;++i){assert(l[i].x>r[i].x-2000);assert(l[i].y==r[i].y);assert(l[i].colour==r[i].colour);}
    }
    assert(ProjectMesh(m,centre,0,2,4000,2000,Project,l,Mesh::capacity)==0);
    assert(ProjectMesh(m,centre,0,0,4000,2000,Project,l,1)==0);
    assert(ProjectMesh(m,{0,0,.3f},0,0,4000,2000,Project,l,Mesh::capacity)==0);
    assert(ProjectMesh(m,{NAN,0,0},0,0,4000,2000,Project,l,Mesh::capacity)==0);
    FakePass pass;assert(Render(pass,m,centre,0,4000,2000,Project));assert(pass.captures==1&&pass.restores==1&&pass.draws==2);
    pass={};pass.capture=false;assert(!Render(pass,m,centre,0,4000,2000,Project));assert(pass.restores==0&&pass.draws==0);
    pass={};pass.draw=false;assert(!Render(pass,m,centre,0,4000,2000,Project));assert(pass.restores==1&&pass.draws==2);
    pass={};assert(!Render(pass,m,{0,0,.3f},0,4000,2000,Project));assert(pass.restores==1&&pass.draws==0);
    pass={};pass.restore=false;assert(!Render(pass,m,centre,0,4000,2000,Project));assert(pass.restores==1);
    if(argc==2){
        FILE* out=nullptr;if(fopen_s(&out,argv[1],"wb")||!out)return 2;
        std::fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1000\" height=\"400\" viewBox=\"0 0 1000 400\"><rect width=\"1000\" height=\"400\" fill=\"#242b30\"/>",out);
        for(int panel=0;panel<4;++panel){
            Build(panel<2?Item::Gun:Item::Knife,panel%2?1.f:0.f,m);
            const float cx=125.f+250*panel,cy=210;
            for(unsigned i=0;i<m.count;i+=3){const auto col=m.vertices[i].colour;
                std::fprintf(out,"<polygon fill=\"#%06x\" fill-opacity=\"%.4f\" points=\"",col&0xffffffu,(col>>24)/255.f);
                for(unsigned j=0;j<3;++j)std::fprintf(out,"%.2f,%.2f ",cx+m.vertices[i+j].x*850,cy+m.vertices[i+j].z*650);
                std::fputs("\"/>",out);}
            std::fprintf(out,"<text x=\"%.0f\" y=\"65\" text-anchor=\"middle\" fill=\"#ded8cd\" font-family=\"sans-serif\" font-size=\"18\">%s / %s</text>",cx,panel<2?"BRON":"NOZ",panel%2?"DLON BLISKO":"SPOCZYNEK");
        }
        std::fputs("<text x=\"500\" y=\"355\" text-anchor=\"middle\" fill=\"#9baeb6\" font-family=\"sans-serif\" font-size=\"14\">PODGLAD GEOMETRII CPU - NIE KADR Z GRY</text></svg>",out);std::fclose(out);
    }
    std::printf("PASS segmented markers: geometry, footprint, proximity, stereo, lifecycle adapter; %u vertices\n",m.count);
}
