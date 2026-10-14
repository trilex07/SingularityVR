#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
namespace handanchor {
struct Record {
    std::uint32_t tick=0, frame=0;
    int moved=0;
    float local[16]{}, controller[2][16]{}, ride[16]{}, before[2][12]{}, after[2][12]{};
    float target[2][3]{}, headOffset[3]{}, anchor[3]{};
};
class Probe {
public:
    static constexpr unsigned limit=8, intervalMs=2000;
    bool Due(std::uint32_t tick) const {return count_<limit && !pending_ && (!count_ || std::uint32_t(tick-last_)>=intervalMs);}
    bool Capture(const Record& r) {
        if(!Due(r.tick))return false;
        const auto finite=[](const float* f,unsigned n){for(unsigned i=0;i<n;++i)if(!std::isfinite(f[i]))return false;return true;};
        if(!finite(r.local,16)||!finite(r.controller[0],16)||!finite(r.controller[1],16)||!finite(r.ride,16)||
           !finite(r.before[0],12)||!finite(r.before[1],12)||!finite(r.after[0],12)||!finite(r.after[1],12)||
           !finite(r.target[0],3)||!finite(r.target[1],3)||!finite(r.headOffset,3)||!finite(r.anchor,3))return false;
        record_=r;last_=r.tick;++count_;pending_=true;return true;
    }
    bool Take(Record& out){if(!pending_)return false;out=record_;pending_=false;return true;}
    unsigned Count()const{return count_;}
private:
    Record record_{};unsigned count_=0;std::uint32_t last_=0;bool pending_=false;
};
}
