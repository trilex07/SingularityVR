#pragma once
#include <cstdint>
namespace menucapture {
struct Owner {
    std::uint64_t state=0;
    std::uintptr_t source=0;
    bool valid=false;
    void Invalidate(std::uint64_t next) {state=next;source=0;valid=false;}
    void Capture(std::uint64_t current,std::uintptr_t id) {state=current;source=id;valid=id!=0;}
    bool CanProvide(std::uint64_t current) const {
        return valid && source && state==current && (current&0xffffffffu)!=0;
    }
};
}
