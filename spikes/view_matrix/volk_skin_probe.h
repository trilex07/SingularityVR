#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>

// Diagnostic only. Never supplies matrices to rendering. Producers may run on
// input/engine threads; window and snapshots are owned by the render thread.
namespace volkskin {
enum Event : unsigned { ImpulseDown=1, ImpulseUp=2, FireEnter=4, FireExit=8, Hands=16 };
constexpr unsigned bones[5]={55,56,53,54,57};
constexpr const char* names[5]={"clip","Slide","cog_shotgun","release","Trigger"};
struct Mailbox {
    std::atomic<unsigned> pending{0}, receipts{0};
    bool Publish(unsigned event) {
        if(receipts.fetch_add(1,std::memory_order_relaxed)>=64)return false;
        pending.fetch_or(event,std::memory_order_release);return true;
    }
    unsigned Take(){return pending.exchange(0,std::memory_order_acq_rel);}
};
struct Window {
    unsigned captures=0, reason=0, origin=0, last=~0u;
    int state=-1; bool active=false;
    bool Draw(unsigned frame,int nextState,unsigned events) {
        if(nextState!=state){events|=Hands;state=nextState;}
        if(events){origin=frame;reason=events;active=true;}
        const unsigned age=frame-origin;
        if(!active||captures>=64||last==frame)return false;
        if(age>24){active=false;return false;}
        if(age!=0&&age!=1&&age!=4&&age!=24)return false;
        last=frame;++captures;return true;
    }
};
inline bool Signature(unsigned vertices,unsigned first,unsigned triangles,unsigned minIndex,int base) {
    return vertices==8297&&first==0&&triangles==10835&&minIndex==0&&base==0;
}
inline float Determinant(const float* p){
    return p[0]*(p[5]*p[10]-p[6]*p[9])-p[1]*(p[4]*p[10]-p[6]*p[8])+p[2]*(p[4]*p[9]-p[5]*p[8]);
}
inline bool Finite(const float* p,unsigned n){for(unsigned i=0;i<n;++i)if(!std::isfinite(p[i]))return false;return true;}
struct Snapshot {
    bool valid=false;unsigned frame=0,weapon=0;int state=-1;
    float skin[15][4]{},local[4][4]{};
};
}
