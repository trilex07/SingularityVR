#pragma once
#include <cstdint>

// The authored gun_scale control is a presentation switch on cog_gun, NOT
// recoil or reload. UE3 blends its scale as 1 + strength * (scale - 1).
// Returning 1 for this one local control leaves all current animation intact.
namespace dualgunscale {
inline constexpr unsigned GetterSlot=0x140, PawnControlOffset=0x1024;
inline constexpr unsigned ScaleOffset=0x7c;
inline constexpr std::uintptr_t Getter=0xb3e400, Consumer=0xaec25d, Return=0xaec26c;
inline constexpr unsigned GetterBytes=6, ConsumerBytes=26;
inline constexpr std::uint64_t GetterHash=0x63ba66a41ada6541ull;
inline constexpr std::uint64_t ConsumerHash=0x401d7706085b7ae1ull;
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
