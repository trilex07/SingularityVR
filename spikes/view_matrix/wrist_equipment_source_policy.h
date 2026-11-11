#pragma once
#include <cstdint>

namespace wristhud {
inline bool SourceCurrent(std::uintptr_t cachedController,std::uintptr_t cachedPawn,
                          std::uintptr_t cachedHud,std::uintptr_t cachedClass,
                          std::uint64_t sampled,std::uintptr_t liveController,
                          std::uintptr_t livePawn,std::uintptr_t liveClass,
                          std::uint64_t now){
    return cachedController&&cachedPawn&&cachedHud&&cachedClass&&
           cachedController==liveController&&cachedPawn==livePawn&&
           cachedClass==liveClass&&sampled&&now>=sampled&&now-sampled<=250;
}
}
