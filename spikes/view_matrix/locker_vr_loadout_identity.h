#pragma once
// Offline-pinned executable identity for locker VR loadout mutation helpers.
// Text capture: research/etw/capture-20261006-205508/Singularity.text-memory.bin
// (sha256 858f2fc33b1c99b3a4a27a817a11205da66414ee1b030a03df821f1ad6a0c78a).
#include "pause_rtt_probe.h"
#include <cstring>

extern uintptr_t g_textLo, g_textHi;

namespace LockerVrLoadoutIdentity {

constexpr uintptr_t kCaptureTextBase = 0x402000u;
constexpr uintptr_t kPinnedAppendFNameVa = 0x4AFFD0u;
constexpr uintptr_t kAppendFNameTextRva = kPinnedAppendFNameVa - kCaptureTextBase;
constexpr unsigned kPrologueBytes = 16u;

inline const unsigned char kAppendFNamePrologue[kPrologueBytes] = {
    0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x0C, 0x56, 0x6A, 0x01, 0x50, 0x8B, 0xF1, 0x8B, 0x4D, 0x08, 0x6A};

inline volatile LONG g_factoryOffsetPinned = 0;
inline volatile LONG g_factoryOffsetRva = 0;
inline volatile LONG g_factoryPrologueHashLo = 0;
inline volatile LONG g_factoryPrologueHashHi = 0;

inline bool FnInText(uintptr_t fn) { return fn >= g_textLo && fn < g_textHi; }

inline unsigned long long HashPrologue(uintptr_t fn) {
    if (!fn || !PauseRttProbe::SafeRange(fn, kPrologueBytes)) return 0;
    unsigned long long h = 1469598103934665603ull;
    const auto* p = reinterpret_cast<const unsigned char*>(fn);
    for (unsigned i = 0; i < kPrologueBytes; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

inline bool AppendFNameIdentityOk() {
    if (!g_textLo) return false;
    const uintptr_t fn = g_textLo + kAppendFNameTextRva;
    if (!FnInText(fn)) return false;
    return memcmp(reinterpret_cast<const void*>(fn), kAppendFNamePrologue, kPrologueBytes) == 0;
}

inline bool PinFactoryOffsetFromLiveSlot(uintptr_t fn) {
    if (!FnInText(fn) || !AppendFNameIdentityOk()) return false;
    const unsigned long long h = HashPrologue(fn);
    if (!h) return false;
    const LONG rva = static_cast<LONG>(fn - g_textLo);
    if (InterlockedCompareExchange(&g_factoryOffsetPinned, 0, 0)) {
        if (InterlockedCompareExchange(&g_factoryOffsetRva, 0, 0) != rva) return false;
        const unsigned long long expect =
            (static_cast<unsigned long long>(static_cast<ULONG>(InterlockedCompareExchange(
                 &g_factoryPrologueHashHi, 0, 0)))
             << 32) |
            static_cast<unsigned>(InterlockedCompareExchange(&g_factoryPrologueHashLo, 0, 0));
        return h == expect;
    }
    InterlockedExchange(&g_factoryOffsetRva, rva);
    InterlockedExchange(&g_factoryPrologueHashLo, static_cast<LONG>(h & 0xFFFFFFFFu));
    InterlockedExchange(&g_factoryPrologueHashHi, static_cast<LONG>(h >> 32));
    InterlockedExchange(&g_factoryOffsetPinned, 1);
    return true;
}

inline bool FactoryIdentityOk(uintptr_t fn) {
    if (!FnInText(fn) || !AppendFNameIdentityOk()) return false;
    if (!InterlockedCompareExchange(&g_factoryOffsetPinned, 0, 0)) return false;
    const LONG rva = InterlockedCompareExchange(&g_factoryOffsetRva, 0, 0);
    if (fn != g_textLo + static_cast<uintptr_t>(rva)) return false;
    const unsigned long long expect =
        (static_cast<unsigned long long>(static_cast<ULONG>(InterlockedCompareExchange(
             &g_factoryPrologueHashHi, 0, 0)))
         << 32) |
        static_cast<unsigned>(InterlockedCompareExchange(&g_factoryPrologueHashLo, 0, 0));
    return HashPrologue(fn) == expect;
}

} // namespace LockerVrLoadoutIdentity
