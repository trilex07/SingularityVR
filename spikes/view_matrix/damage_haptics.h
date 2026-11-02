#pragma once
#include <algorithm>
#include <cstdint>

// Actual local Pawn.Health loss across the native TakeDamage call, not a button,
// camera shake, enemy hit, or requested damage amount. Allocation-free policy.
namespace damagehaptics {
class Engine {
    std::uint64_t end_=0;
    float amplitude_=0;
public:
    bool OnNativeReturn(bool sameLocalPawn,int before,int after,std::uint64_t now) {
        if(!sameLocalPawn || before<=0 || before>1000000 || after>before || after< -1000000)return false;
        const int clamped=after<0?0:after;
        const int loss=before-clamped;
        if(loss<=0)return false;
        const float strength=.4f+.4f*(std::min)(loss,40)/40.f;
        // Nested damage receipts cannot add amplitudes or prolong the same hit.
        if(now<end_){amplitude_=(std::max)(amplitude_,strength);return true;}
        amplitude_=strength;end_=now+85;return true;
    }
    float Sample(bool allowed,std::uint64_t now){
        if(!allowed||now>=end_){Clear();return 0;}
        return amplitude_;
    }
    void Clear(){end_=0;amplitude_=0;}
};
}
