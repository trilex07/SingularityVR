#include "../native_camera_comfort_math.h"
#include <cassert>
#include <limits>
#include <cstdio>
#include <initializer_list>
using namespace cameracomfort;
int main() {
    Gates g{};g.enabled=g.vr=g.gameplay=g.sameCamera=g.samePawn=true;g.style=1;
    float pawn[3]={100,200,30};
    Pov raw{{101,198,103},{120,400,0},95};
    Pov shaken{{110,190,110},{130,420,30},110};
    Pov out=shaken,anchor=raw;
    assert(Reduce(g,pawn,70,raw,out,anchor));
    assert(out.location[0]==100 && out.location[1]==200 && out.location[2]==100);
    assert(!std::memcmp(out.location,anchor.location,12));
    assert(!std::memcmp(out.rotation,raw.rotation,12));
    assert(out.fov==110 && anchor.fov==95); // distinct native FOV owners preserved
    assert(!std::memcmp(anchor.rotation,raw.rotation,12));
    auto rejected=[&](Gates bad,float height=70) {
        out=shaken;anchor=raw;
        assert(!Reduce(bad,pawn,height,raw,out,anchor));
        assert(!std::memcmp(&out,&shaken,sizeof(out)) && !std::memcmp(&anchor,&raw,sizeof(anchor)));
    };
    auto bad=g;bad.enabled=false;rejected(bad);
    bad=g;bad.vr=false;rejected(bad);
    bad=g;bad.gameplay=false;rejected(bad);
    bad=g;bad.cinematic=true;rejected(bad);
    bad=g;bad.menu=true;rejected(bad);
    bad=g;bad.sameCamera=false;rejected(bad);
    bad=g;bad.samePawn=false;rejected(bad);
    bad=g;bad.overridden=true;rejected(bad);
    for(unsigned style: {0u,2u,3u,4u,5u,6u}) {bad=g;bad.style=style;rejected(bad);}
    bad=g;bad.cameraBlend=.001f;rejected(bad);
    bad=g;bad.cameraBlend=std::numeric_limits<float>::quiet_NaN();rejected(bad);
    rejected(g,0);rejected(g,201);rejected(g,std::numeric_limits<float>::infinity());
    pawn[0]=std::numeric_limits<float>::quiet_NaN();rejected(g);pawn[0]=100;
    // Replay a changing body position, scripted bob/shake and crouched base height.
    // A tracked hand relative to the HMD must not inherit bob in either eye.
    for(int frame=0;frame<900;++frame) {
        pawn[0]=100+frame*.125f;pawn[2]=30+(frame/90);
        const float height=frame<450?70.f:45.f;
        raw.location[0]=pawn[0];raw.location[2]=pawn[2]+height+3*std::sin(frame*.3f);
        out=raw;out.location[0]+=10*std::sin(frame*.7f);anchor=raw;
        assert(Reduce(g,pawn,height,raw,out,anchor));
        for(float eye: {-1.6f,1.6f}) {
            const float physicalHead=.7f,physicalHand=12.f;
            const float relative=(anchor.location[0]+physicalHand)-(out.location[0]+physicalHead+eye);
            assert(std::fabs(relative-(physicalHand-physicalHead-eye))<.0001f);
        }
        assert(out.location[2]==pawn[2]+height);
    }
    std::puts("PASS native camera comfort: gates, transactional rejects, FOV, 900-frame world/hand origin replay");
}
