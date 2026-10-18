#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../holster_marker.h"
#include <cstdio>
using namespace uipointer;
struct Q{float x,y,z,w;};
struct Eye{V position;Q q;};
static Eye eyes[2];
static V Rotate(Q q,V v){
    const V axis{q.x,q.y,q.z};
    const auto cross=[](V a,V b){return V{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
    const auto t=cross(axis,v)*2.f;return v+t*q.w+cross(axis,t);
}
// Same non-inlined eye adapter as the measured production map symbol.
__declspec(noinline) static bool Legacy(int e,V p,ScreenPoint& s){
    return ProjectPoint(p,eyes[e].position,Rotate(eyes[e].q,{0,0,-1}),
        Rotate(eyes[e].q,{1,0,0}),Rotate(eyes[e].q,{0,1,0}),1.3f,1.3f,s);
}
static volatile double sink=0;
int main(){
    holstermarker::Mesh meshes[2];holstermarker::Build(holstermarker::Item::Gun,.3f,meshes[0]);
    holstermarker::Build(holstermarker::Item::Knife,.7f,meshes[1]);
    holstermarker::ScreenVertex out[holstermarker::Mesh::capacity];
    LARGE_INTEGER freq;QueryPerformanceFrequency(&freq);
    double sums[2]{};unsigned rounds[2]{};constexpr unsigned frames=1000;
    for(bool fast:{false,true,true,false,false,true}){
        LARGE_INTEGER start,end;ULONG64 c0=0,c1=0;QueryThreadCycleTime(GetCurrentThread(),&c0);QueryPerformanceCounter(&start);
        double checksum=0;
        for(unsigned frame=0;frame<frames;++frame){
            const float angle=.1f*std::sin(frame*.019f);const Q q{0,std::sin(angle),0,std::cos(angle)};
            eyes[0]={{-.032f,.01f,0},q};eyes[1]={{.032f,-.01f,0},q};
            for(unsigned item=0;item<2;++item){
                PreparedProjection prepared[2];
                if(fast)for(int e=0;e<2;++e)prepared[e].Prepare(eyes[e].position,Rotate(q,{0,0,-1}),Rotate(q,{1,0,0}),Rotate(q,{0,1,0}),1.3f,1.3f);
                const auto project=[&](int e,V p,ScreenPoint& s){return prepared[e].Project(p,s);};
                for(int e=0;e<2;++e){
                    const V centre{item?.24f:-.24f,-.6f,-.5f};
                    const unsigned count=fast?holstermarker::ProjectMesh(meshes[item],centre,angle,e,5904,3012,project,out,holstermarker::Mesh::capacity):
                        holstermarker::ProjectMesh(meshes[item],centre,angle,e,5904,3012,Legacy,out,holstermarker::Mesh::capacity);
                    for(unsigned i=0;i<count;++i)checksum+=out[i].x*.001+out[i].y*.002;
                }
            }
        }
        QueryPerformanceCounter(&end);QueryThreadCycleTime(GetCurrentThread(),&c1);sink=checksum;
        const double ms=double(end.QuadPart-start.QuadPart)*1000/freq.QuadPart/frames;
        sums[fast]+=ms;++rounds[fast];
        std::printf("%s ms/frame=%.6f cycles/frame=%.0f checksum=%.9f\n",fast?"prepared":"legacy",ms,double(c1-c0)/frames,checksum);
    }
    std::printf("ABBAAB averages legacy=%.6f prepared=%.6f saved=%.6f ms/frame; %u+%u vertices, two eyes, CPU only NOT game FPS\n",
        sums[0]/rounds[0],sums[1]/rounds[1],sums[0]/rounds[0]-sums[1]/rounds[1],meshes[0].count,meshes[1].count);
}
