#pragma once
#include <cstdint>

namespace wristhud {
// Native resolver already verifies camera/controller and pawn/controller
// reciprocal links. A second generic Pawn-property walk must not veto it.
inline bool SourceOwner(std::uintptr_t controller,std::uintptr_t pawn,
                        std::uintptr_t publishedController,std::uintptr_t publishedPawn){
    return controller&&pawn&&controller==publishedController&&pawn==publishedPawn;
}
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
