#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace cinemaexitprofile {
inline constexpr char request[]="SingularityVR cinema exit hotpath v1";
inline constexpr std::uint64_t max_ms=45000;
inline bool IsRequest(const char* data,std::size_t size) {
    return data && size==sizeof(request)-1 && std::memcmp(data,request,size)==0;
}
class Gate {
    bool seen_=false;
public:
    void Observe(bool cinematic,bool cutsceneFlat) {if(cinematic && cutsceneFlat)seen_=true;}
    bool Ready(bool focused,bool gameplay,bool cinematic,bool menu,bool paused) const {
        return seen_ && focused && gameplay && !cinematic && !menu && !paused;
    }
};
inline bool Expired(std::uint64_t start,std::uint64_t now) {
    return start && now>=start && now-start>=max_ms;
}
}
