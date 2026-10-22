#pragma once
// Offline-pinned executable identity for locker VR loadout mutation helpers.
// Text capture: research/etw/capture-20261006-205508/Singularity.text-memory.bin
// (sha256 858f2fc33b1c99b3a4a27a817a11205da66414ee1b030a03df821f1ad6a0c78a).
//
// GGameSystem (RvGameSystem) vtable +0x20C factory — recovered offline from:
//   mission unlock 0xF03450 (add vtable+0x20C, 0x4AFFD0, indirect call)
//   duplicate pickup 0x11B4B6E (same sequence)
//   vtable anchor: GetFeaturesUnlocked @ +0x1F0 -> 0xFAA240 (C++ virtual method in RvGameSystem vtable 0x019CDBF8;
//                  0xF55B30 is its script exec wrapper in GNatives table 0x01BCF9C4)
#include "pause_rtt_probe.h"
#include <Windows.h>
#include <cstring>

extern uintptr_t g_textLo, g_textHi;
bool NameOf(uintptr_t obj, char* buf, size_t cap);

namespace LockerVrLoadoutIdentity {

constexpr uintptr_t kCaptureImageBase = 0x400000u;

constexpr uintptr_t kPinnedAppendFNameVa = 0x4AFFD0u;
constexpr uintptr_t kAppendFNameRvaFromImage = kPinnedAppendFNameVa - kCaptureImageBase;

constexpr uintptr_t kPinnedFactoryVa = 0x00FD2320u;
constexpr uintptr_t kExpectedFactoryRvaFromImage = kPinnedFactoryVa - kCaptureImageBase;
constexpr uint64_t kExpectedFactoryFingerprint = 0xF2B5EBD13E8B220Full;

constexpr uintptr_t kPinnedRvGameSystemVtableVa = 0x019CDBF8u;
constexpr uintptr_t kExpectedRvGameSystemVtableRvaFromImage =
    kPinnedRvGameSystemVtableVa - kCaptureImageBase;
constexpr unsigned kGameSystemFactoryVtableOff = 0x20Cu;

constexpr unsigned kPrologueBytes = 16u;

inline const unsigned char kAppendFNamePrologue[kPrologueBytes] = {
    0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x0C, 0x56, 0x6A, 0x01, 0x50, 0x8B, 0xF1, 0x8B, 0x4D, 0x08, 0x6A};

inline const unsigned char kFactoryPrologue[kPrologueBytes] = {
    0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68, 0x78, 0xE9, 0x6E, 0x01, 0x64, 0xA1, 0x00, 0x00, 0x00, 0x00};

inline uintptr_t ImageBase() {
    return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
}

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
    const uintptr_t base = ImageBase();
    if (!base) return false;
    const uintptr_t fn = base + kAppendFNameRvaFromImage;
    if (!FnInText(fn)) return false;
    return memcmp(reinterpret_cast<const void*>(fn), kAppendFNamePrologue, kPrologueBytes) == 0;
}

inline bool FactoryIdentityOk(uintptr_t fn) {
    const uintptr_t base = ImageBase();
    if (!base) return false;
    const uintptr_t expected = base + kExpectedFactoryRvaFromImage;
    if (fn != expected || !FnInText(fn)) return false;
    if (HashPrologue(fn) != kExpectedFactoryFingerprint) return false;
    return memcmp(reinterpret_cast<const void*>(fn), kFactoryPrologue, kPrologueBytes) == 0;
}

// Live GGameSystem instance: vtable must resolve to RvGameSystem C++ vtable (0x019CDBF8),
// and vtable+0x20C must resolve to native archetype factory method (0x00FD2320).
inline bool GameSystemObjectIdentityOk(uintptr_t gs) {
    if (!gs || !PauseRttProbe::SafeRange(gs, 4)) return false;
    const uintptr_t vt = PauseRttProbe::Pointer(gs);
    const uintptr_t base = ImageBase();
    if (!base || !vt) return false;
    const uintptr_t expectedVt = base + kExpectedRvGameSystemVtableRvaFromImage;
    if (vt != expectedVt) return false;
    if (!PauseRttProbe::SafeRange(vt + kGameSystemFactoryVtableOff, 4)) return false;
    const uintptr_t factoryFn = PauseRttProbe::Pointer(vt + kGameSystemFactoryVtableOff);
    return FactoryIdentityOk(factoryFn);
}

// Catalog / availability: validated RvGameSystem object, class name check, and readable catalog layout.
inline bool GameSystemCatalogSourceOk(uintptr_t gs) {
    constexpr int kObjClass = 0x34;
    constexpr uintptr_t kCatalogDataOff = 0x160u;
    constexpr uintptr_t kCatalogCountOff = 0x164u;
    constexpr uintptr_t kCatalogStride = 0x50u;
    if (!GameSystemObjectIdentityOk(gs)) return false;
    if (!PauseRttProbe::SafeRange(gs + kCatalogCountOff, 8)) return false;
    const int count = *reinterpret_cast<const int*>(gs + kCatalogCountOff);
    if (count <= 0 || count > 32) return false;
    const uintptr_t data = PauseRttProbe::Pointer(gs + kCatalogDataOff);
    if (!data || !PauseRttProbe::SafeRange(data, kCatalogStride)) return false;
    if (PauseRttProbe::SafeRange(gs + kObjClass, 4)) {
        const uintptr_t cls = PauseRttProbe::Pointer(gs + kObjClass);
        if (cls) {
            char className[96]{};
            if (NameOf(cls, className, sizeof(className)) && strcmp(className, "RvGameSystem") != 0) {
                return false;
            }
        }
    }
    return true;
}

} // namespace LockerVrLoadoutIdentity
