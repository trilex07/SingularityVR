#pragma once
#include <cstdint>
#include "game_build_native.h"

// The authored gun_scale control is a presentation switch on cog_gun, NOT
// recoil or reload. UE3 blends its scale as 1 + strength * (scale - 1).
// Returning 1 for this one local control leaves all current animation intact.
namespace dualgunscale {
inline constexpr unsigned GetterSlot=0x140, PawnControlOffset=0x1024;
inline constexpr unsigned ScaleOffset=0x7c;
inline const std::uintptr_t& Getter=gamebuild::NativeCurrent().scaleGetter;
inline const std::uintptr_t& Consumer=gamebuild::NativeCurrent().scaleConsumer;
inline const std::uintptr_t& Return=gamebuild::NativeCurrent().scaleReturn;
inline constexpr unsigned GetterBytes=6, ConsumerBytes=26;
inline const std::uint64_t& GetterHash=gamebuild::NativeCurrent().scaleGetterHash;
inline const std::uint64_t& ConsumerHash=gamebuild::NativeCurrent().scaleConsumerHash;
struct Context {
    bool enabled=false, dual=false, nativeHands=false, holstered=false, equipped=false;
};
inline bool ContextAllowed(const Context& c) {
    return c.enabled && c.dual && !c.nativeHands && !c.holstered && c.equipped;
}
inline bool Eligible(const Context& c, bool sameThread, bool ownedControl,
                     bool ownedMesh, std::uintptr_t caller, int bone, float nativeScale) {
    return ContextAllowed(c) && sameThread && ownedControl && ownedMesh &&
        caller==Return && bone>=0 && bone<256 && nativeScale==0.f;
}
}
