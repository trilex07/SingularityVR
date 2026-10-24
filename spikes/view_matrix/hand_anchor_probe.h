#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
namespace handanchor {
struct Record {
    std::uint32_t tick=0, frame=0;
    int moved=0;
    std::uintptr_t weapon=0; unsigned epoch=0;
    float local[16]{}, controller[2][16]{}, ride[16]{}, before[2][12]{}, after[2][12]{};
    float target[2][3]{}, headOffset[3]{}, anchor[3]{};
    // TMD-only optional final-bound evidence. Defaults leave the noTMD probe
    // unchanged. World joints are reconstructed with ORIGINAL bind matrices.
    bool tmd=false;
    unsigned eye=0, hands=0, support=0, verts=0, first=0, triangles=0;
    std::uintptr_t indexBuffer=0;
    float wristWorld[2][3]{}, uuPerMetre=0;
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
        if(r.tmd && (r.eye>1 || !r.indexBuffer || !r.triangles ||
            !std::isfinite(r.uuPerMetre) || r.uuPerMetre<=0 ||
            !finite(r.wristWorld[0],3) || !finite(r.wristWorld[1],3)))return false;
        record_=r;last_=r.tick;++count_;pending_=true;return true;
    }
    bool Take(Record& out){if(!pending_)return false;out=record_;pending_=false;return true;}
    unsigned Count()const{return count_;}
private:
    Record record_{};unsigned count_=0;std::uint32_t last_=0;bool pending_=false;
};

// Eight free-hand records alone expire before the first pistol pickup. A second
// epoch reuses the already reflected Pawn.Weapon signal; max16 records/session,
// no object/name walk or reset loop per swap. Retain an unflushed record first.
class WeaponProbe {
public:
    bool Due(std::uint32_t tick, std::uintptr_t weapon) {
        if (!epochs_) { key_=weapon; epochs_=1; }
        if (key_ != weapon) {
            if (epochs_ >= 2 || pending_) return false;
            key_=weapon; ++epochs_; probe_=Probe{};
        }
        return probe_.Due(tick);
    }
    bool Capture(Record r) {
        r.weapon=key_; r.epoch=epochs_;
        if (!epochs_ || !probe_.Capture(r)) return false;
        pending_=true; ++total_; return true;
    }
    bool Take(Record& out) {
        if (!probe_.Take(out)) return false;
        pending_=false; return true;
    }
    unsigned Count() const { return total_; }
private:
    Probe probe_{}; std::uintptr_t key_=0; unsigned epochs_=0, total_=0; bool pending_=false;
};
}
