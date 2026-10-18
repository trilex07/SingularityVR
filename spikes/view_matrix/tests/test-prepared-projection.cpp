#include "../holster_marker.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
using namespace uipointer;
static unsigned randomState=0x741ac23u;
static float Random(){randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;return (randomState&65535)/65535.f;}
struct Q{float x,y,z,w;};
static V Rotate(Q q,V v){
    const V axis{q.x,q.y,q.z};
    const auto cross=[](V a,V b){return V{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
    const auto t=cross(axis,v)*2.f;return v+t*q.w+cross(axis,t);
}
int main(){
    unsigned comparisons=0;
    for(unsigned scene=0;scene<200;++scene){
        Q q{Random()-.5f,Random()-.5f,Random()-.5f,Random()-.5f};
        const float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);q={q.x/n,q.y/n,q.z/n,q.w/n};
        const V f=Rotate(q,{0,0,-1}),r=Rotate(q,{1,0,0}),u=Rotate(q,{0,1,0});
        const V eye{Random(),Random(),Random()};const float tx=.4f+Random(),ty=.4f+Random();
        PreparedProjection prepared;assert(prepared.Prepare(eye,f,r,u,tx,ty));
        for(unsigned i=0;i<1000;++i){
            const V point{Random()*4-2,Random()*4-2,Random()*4-2};
            ScreenPoint a{17,19,23},b=a;
            const bool baseline=ProjectPoint(point,eye,f,r,u,tx,ty,a);
            assert(prepared.Project(point,b)==baseline);
            assert(!std::memcmp(&a,&b,sizeof(a)));++comparisons;
        }
        // Compare FULL triangle clipping/output, not just visible points.
        holstermarker::Mesh mesh;holstermarker::Build(scene&1?holstermarker::Item::Gun:holstermarker::Item::Knife,Random(),mesh);
        holstermarker::ScreenVertex a[holstermarker::Mesh::capacity],b[holstermarker::Mesh::capacity];
        const auto legacy=[&](int,V p,ScreenPoint& s){return ProjectPoint(p,eye,f,r,u,tx,ty,s);};
        const auto fast=[&](int,V p,ScreenPoint& s){return prepared.Project(p,s);};
        const float yaw=Random()*6.28f;const V centre{Random()-.5f,Random()-.5f,Random()-.5f};
        for(int e=0;e<2;++e){
            const auto ac=holstermarker::ProjectMesh(mesh,centre,yaw,e,5904,3012,legacy,a,holstermarker::Mesh::capacity);
            const auto bc=holstermarker::ProjectMesh(mesh,centre,yaw,e,5904,3012,fast,b,holstermarker::Mesh::capacity);
            assert(ac==bc&&!std::memcmp(a,b,ac*sizeof(*a)));
        }
    }
    PreparedProjection p;ScreenPoint a{17,19,23},b=a;
    assert(!p.Project({0,0,-1},b)&&!std::memcmp(&a,&b,sizeof(a)));
    assert(p.Prepare({},{0,0,-1},{1,0,0},{0,1,0},1,1));
    for(V point: {V{0,0,0},V{0,0,-1e-5f},V{NAN,0,0},V{0,INFINITY,0},V{1,0,-1}}){
        a={17,19,23};b=a;const bool ok=ProjectPoint(point,{},{0,0,-1},{1,0,0},{0,1,0},1,1,a);
        assert(p.Project(point,b)==ok&&!std::memcmp(&a,&b,sizeof(a)));
    }
    for(unsigned invalid=0;invalid<9;++invalid){
        V eye{},f{0,0,-1},r{1,0,0},u{0,1,0};float tx=1,ty=1;
        switch(invalid){case 0:eye.x=NAN;break;case 1:f={};break;case 2:r={};break;case 3:u.x=INFINITY;break;
        case 4:r=f;break;case 5:tx=0;break;case 6:ty=-1;break;case 7:tx=NAN;break;case 8:ty=INFINITY;break;}
        assert(!p.Prepare(eye,f,r,u,tx,ty));a={17,19,23};b=a;
        assert(!p.Project({0,0,-1},b)&&!std::memcmp(&a,&b,sizeof(a)));
        assert(!ProjectPoint({0,0,-1},eye,f,r,u,tx,ty,a));
        assert(p.Prepare({},{0,0,-1},{1,0,0},{0,1,0},1,1));
    }
    std::printf("PASS prepared eye projection: %u bit-identical point results, 400 full triangle outputs, invalidation/near-plane/nonfinite guards\n",comparisons);
}
