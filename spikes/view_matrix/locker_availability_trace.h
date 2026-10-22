#pragma once
// Minimal read-only WeaponLocker availability witness (+0x3E4 vs live catalog vs equip cycle).
// Default OFF ([Diagnostic] LockerAvailabilityTrace=0). Separate from LockerNativeTrace.
// Hooks: equip @ 0xF00F50 only. No Poll(), no progression/catalog writes, no page hooks.
#include "MinHook.h"
#include "inventory_retention_policy.h"
#include "pause_rtt_probe.h"
#include <cstring>
#include <intrin.h>

extern uintptr_t g_textLo, g_textHi;
extern uintptr_t g_playerPawn;
extern volatile LONG g_lockerAvailabilityTrace;
void Log(_Printf_format_string_ const char* fmt, ...);

namespace LockerAvailabilityTrace {

constexpr uintptr_t kWeaponLockerVtable = 0x019A0948u;
constexpr uintptr_t kSlotEquip = 0x1F8u;
constexpr uintptr_t kFnEquip = 0x00F00F50u;
constexpr uintptr_t kGGameSystemPtr = 0x01CF394Cu;
constexpr uintptr_t kCatalogDataOff = 0x160u;
constexpr uintptr_t kCatalogCountOff = 0x164u;
constexpr uintptr_t kCatalogStride = 0x50u;
constexpr uintptr_t kPawnInvManager = 0x3D0u;
constexpr uintptr_t kMgrNativeMaskOff = 0x3E4u;

constexpr unsigned kMaxCatalogScan = 32u;
constexpr unsigned kMaxEquipLogs = 96u;

// equip: ret 0x0C — ECX=self, then FFrame*, slotNum*, stackArg10 (inspect_locker_abi.py).
using EquipFn = void(__thiscall*)(void* self, void* frame, void* slotNum, void* stackArg10);

inline volatile LONG g_session = 0;
inline volatile LONG g_equipLogs = 0;
inline volatile LONG g_hookLive = 0;
inline uintptr_t g_movie = 0;
inline uintptr_t g_cls = 0;
inline int g_offEquipped = -1;
inline uint32_t g_sessionNativeMask = 0;
inline uint32_t g_observedVrMask = 0;
inline EquipFn g_origEquip = nullptr;

struct OrdinaryRow {
    const char* path;
    int runtimeIndex;
    uint32_t nativeMask;
    int row24;
    int row4C;
    bool known;
};
inline OrdinaryRow g_rows[4]{};

inline thread_local int t_reenter = 0;

inline bool Enabled() {
    return InterlockedCompareExchange(&g_lockerAvailabilityTrace, 0, 0) != 0;
}

inline bool Armed() {
    return InterlockedCompareExchange(&g_session, 0, 0) != 0;
}

inline bool VtSlot(uintptr_t vt, unsigned off, uintptr_t expect) {
    if (!vt || !PauseRttProbe::SafeRange(vt + off, 4)) return false;
    return PauseRttProbe::Pointer(vt + off) == expect;
}

inline bool IsWeaponLockerMovie(uintptr_t movie) {
    if (!movie || !PauseRttProbe::SafeRange(movie + OBJ_CLASS, 4)) return false;
    const uintptr_t vt = PauseRttProbe::Pointer(movie);
    if (!VtSlot(vt, kSlotEquip, kFnEquip)) return false;
    const uintptr_t cls = PauseRttProbe::Pointer(movie + OBJ_CLASS);
    char clsName[96]{};
    return cls && NameOf(cls, clsName, sizeof(clsName)) && !strcmp(clsName, "RvUIGFxMovie_WeaponLocker");
}

inline bool ReadFStringAscii(uintptr_t fstr, char* out, size_t outLen) {
    if (!out || outLen < 2 || !PauseRttProbe::SafeRange(fstr, 8)) return false;
    out[0] = 0;
    const int num = *reinterpret_cast<const int*>(fstr + 4);
    const wchar_t* w = reinterpret_cast<const wchar_t*>(PauseRttProbe::Pointer(fstr));
    if (num <= 0 || num > 128 || !w) return false;
    if (!PauseRttProbe::SafeRange(reinterpret_cast<uintptr_t>(w), static_cast<size_t>(num) * 2))
        return false;
    size_t j = 0;
    for (int i = 0; i < num && j + 1 < outLen; ++i) {
        const wchar_t c = w[i];
        if (c < 32 || c > 126) continue;
        out[j++] = static_cast<char>(c);
    }
    out[j] = 0;
    return out[0] != 0;
}

inline const char* PathForMesh(int mesh) {
    switch (mesh) {
    case 5799: return "wp_SP_ARPistol";
    case 11387: return "wp_SP_ARMachineGun";
    case 8297: return "wp_SP_ARShotgun";
    case 11389: return "wp_SP_ARSniperRifle";
    default: return nullptr;
    }
}

inline unsigned VrBitForPath(const char* path) {
    if (!path) return 0;
    if (!strcmp(path, "wp_SP_ARPistol")) return 1u;
    if (!strcmp(path, "wp_SP_ARMachineGun")) return 2u;
    if (!strcmp(path, "wp_SP_ARShotgun")) return 4u;
    if (!strcmp(path, "wp_SP_ARSniperRifle")) return 8u;
    return 0;
}

inline OrdinaryRow* RowSlotForPath(const char* path) {
    if (!path) return nullptr;
    for (auto& r : g_rows) {
        if (r.path && !strcmp(r.path, path)) return &r;
    }
    return nullptr;
}

inline const OrdinaryRow* FindRowByPath(const char* path) {
    OrdinaryRow* r = RowSlotForPath(path);
    return (r && r->known) ? r : nullptr;
}

inline uintptr_t ResolveInventoryManager() {
    const uintptr_t pawn = g_playerPawn;
    if (!pawn || !PauseRttProbe::SafeRange(pawn + kPawnInvManager, 4)) return 0;
    return PauseRttProbe::Pointer(pawn + kPawnInvManager);
}

inline void ResetOrdinaryRows() {
    static const char* kPaths[4] = {"wp_SP_ARPistol", "wp_SP_ARMachineGun", "wp_SP_ARShotgun",
                                    "wp_SP_ARSniperRifle"};
    for (unsigned i = 0; i < 4; ++i) {
        g_rows[i].path = kPaths[i];
        g_rows[i].runtimeIndex = -1;
        g_rows[i].nativeMask = 0;
        g_rows[i].row24 = 0;
        g_rows[i].row4C = 0;
        g_rows[i].known = false;
    }
}

inline void ScanLiveCatalog(uint32_t managerMask) {
    const uintptr_t gs = PauseRttProbe::Pointer(kGGameSystemPtr);
    if (!gs || !PauseRttProbe::SafeRange(gs + kCatalogCountOff, 8)) {
        Log("locker avail: catalog scan refused GGameSystem=%p", (void*)gs);
        return;
    }
    const uintptr_t data = PauseRttProbe::Pointer(gs + kCatalogDataOff);
    const int count = *reinterpret_cast<const int*>(gs + kCatalogCountOff);
    const int n = count < 0 ? 0 : (count > (int)kMaxCatalogScan ? (int)kMaxCatalogScan : count);
    Log("locker avail: session catalog GGameSystem=%p data=%p count=%d scan=%d managerMask=0x%X",
        (void*)gs, (void*)data, count, n, managerMask);
    for (int i = 0; i < n; ++i) {
        const uintptr_t row = data + static_cast<uintptr_t>(i) * kCatalogStride;
        if (!PauseRttProbe::SafeRange(row, kCatalogStride)) break;
        char path[96]{};
        if (!ReadFStringAscii(row, path, sizeof(path))) continue;
        OrdinaryRow* slot = RowSlotForPath(path);
        if (!slot) continue;
        slot->runtimeIndex = i;
        slot->nativeMask = 1u << static_cast<unsigned>(i);
        slot->row24 = PauseRttProbe::SafeRange(row + 0x24, 4) ? *reinterpret_cast<const int*>(row + 0x24) : -1;
        slot->row4C = PauseRttProbe::SafeRange(row + 0x4C, 4) ? *reinterpret_cast<const int*>(row + 0x4C) : -1;
        slot->known = true;
        const int bitSet = (managerMask & slot->nativeMask) ? 1 : 0;
        Log("locker avail: path=%s runtimeIndex=%d nativeMask=0x%X bitSet=%d row24=%d row4C=%d", path, i,
            slot->nativeMask, bitSet, slot->row24, slot->row4C);
    }
    for (unsigned k = 0; k < 4; ++k) {
        if (!g_rows[k].known)
            Log("locker avail: path=%s runtimeIndex=MISSING (not in first %d live rows)", g_rows[k].path, n);
    }
}

inline bool ReadEquippedSlotFName(uintptr_t movie, int index, char* out, unsigned outLen) {
    if (!out || outLen < 2 || g_offEquipped < 0 || !movie) return false;
    out[0] = 0;
    if (!PauseRttProbe::SafeRange(movie + static_cast<uintptr_t>(g_offEquipped), 12)) return false;
    const uintptr_t arrData = PauseRttProbe::Pointer(movie + static_cast<uintptr_t>(g_offEquipped));
    const int count = *reinterpret_cast<const int*>(movie + static_cast<uintptr_t>(g_offEquipped) + 4);
    if (index < 0 || index >= count || !arrData) return false;
    if (!PauseRttProbe::SafeRange(arrData + static_cast<uintptr_t>(index) * 8, 8)) return false;
    const int nameIndex = *reinterpret_cast<const int*>(arrData + static_cast<uintptr_t>(index) * 8);
    NameFromIndex(nameIndex, out, static_cast<int>(outLen));
    return out[0] != 0;
}

inline uint32_t NativeProgressionOrdinaryVrMask(uint32_t managerMask) {
    uint32_t vr = 0;
    for (const auto& r : g_rows) {
        if (!r.known || !r.nativeMask) continue;
        if (managerMask & r.nativeMask) vr |= VrBitForPath(r.path);
    }
    return vr;
}

inline uint32_t ReadManagerMask(uintptr_t mgr) {
    if (mgr && PauseRttProbe::SafeRange(mgr + kMgrNativeMaskOff, 4))
        return *reinterpret_cast<const uint32_t*>(mgr + kMgrNativeMaskOff);
    return g_sessionNativeMask;
}

inline void LogSessionOpen(uintptr_t movie, uintptr_t cls) {
    ResetOrdinaryRows();
    g_offEquipped = PauseRttProbe::Property(cls, "mEquippedWeapons", "ArrayProperty");
    const uintptr_t mgr = ResolveInventoryManager();
    uint32_t managerMask = 0;
    if (mgr && PauseRttProbe::SafeRange(mgr + kMgrNativeMaskOff, 4))
        managerMask = *reinterpret_cast<const uint32_t*>(mgr + kMgrNativeMaskOff);
    g_sessionNativeMask = managerMask;
    g_observedVrMask = 0;
    InterlockedExchange(&g_equipLogs, 0);
    char obj[96]{};
    NameOf(movie, obj, sizeof(obj));
    Log("locker avail: SESSION OPEN movie=%p mgr=%p nativeProgressionMask=0x%X obj=%s tid=%u",
        (void*)movie, (void*)mgr, managerMask, obj, GetCurrentThreadId());
    ScanLiveCatalog(managerMask);
}

inline void LogSessionClose() {
    const uint32_t mgrMask = ReadManagerMask(ResolveInventoryManager());
    const uint32_t nativeVr = NativeProgressionOrdinaryVrMask(mgrMask);
    const uint32_t offered = g_observedVrMask;
    char observed[160]{};
    int pos = 0;
    observed[pos++] = '{';
    for (unsigned k = 0; k < 4; ++k) {
        if (!g_rows[k].known) continue;
        const unsigned bit = VrBitForPath(g_rows[k].path);
        if (!(offered & bit)) continue;
        if (pos > 1) observed[pos++] = ',';
        for (const char* p = g_rows[k].path; *p && pos < (int)sizeof(observed) - 2; ++p)
            observed[pos++] = *p;
    }
    if (pos < (int)sizeof(observed) - 1) observed[pos++] = '}';
    observed[pos] = 0;
    Log("locker avail summary: offeredOrdinary=0x%X nativeProgressionOrdinary=0x%X observed=%s "
        "(VR bits 1/2/4/8; native catalog bits separate; manager+0x3E4=0x%X)",
        offered, nativeVr, observed, mgrMask);
}

inline void MaybeLogEquip(void* self, void* slotNum) {
    if (!Armed()) return;
    const LONG n = InterlockedIncrement(&g_equipLogs);
    if ((unsigned)n > kMaxEquipLogs) {
        InterlockedDecrement(&g_equipLogs);
        return;
    }
    const uintptr_t movie = reinterpret_cast<uintptr_t>(self);
    if (movie != g_movie) return;
    int slot = static_cast<int>(reinterpret_cast<intptr_t>(slotNum));
    if (slot != 0 && slot != 1 && slotNum) {
        const int* slotPtr = reinterpret_cast<const int*>(slotNum);
        if (PauseRttProbe::SafeRange(reinterpret_cast<uintptr_t>(slotPtr), 4)) slot = *slotPtr;
    }
    char fname[64]{};
    ReadEquippedSlotFName(movie, slot, fname, sizeof(fname));
    const int mesh = inventoryretention::WeaponMesh(fname);
    const char* path = PathForMesh(mesh);
    const OrdinaryRow* row = path ? FindRowByPath(path) : nullptr;
    const uint32_t managerMask = ReadManagerMask(ResolveInventoryManager());
    if (path) g_observedVrMask |= VrBitForPath(path);
    if (!path || !row || !row->known) {
        Log("locker avail equip: slot=%d Weapon=%s path=- idx=- nativeBit=- managerMask=0x%X bitSet=-",
            slot, fname[0] ? fname : "-", managerMask);
        return;
    }
    const int bitSet = (managerMask & row->nativeMask) ? 1 : 0;
    Log("locker avail equip: slot=%d Weapon=%s path=%s idx=%d nativeBit=0x%X managerMask=0x%X bitSet=%d",
        slot, fname, path, row->runtimeIndex, row->nativeMask, managerMask, bitSet);
}

struct EquipHook {
    static void __thiscall Detour(void* self, void* frame, void* slotNum, void* stackArg10) {
        (void)frame;
        (void)stackArg10;
        ++t_reenter;
        g_origEquip(self, frame, slotNum, stackArg10);
        if (t_reenter == 1 && Enabled() && Armed()) MaybeLogEquip(self, slotNum);
        --t_reenter;
    }
};

inline bool InstallEquipHook() {
    if (InterlockedCompareExchange(&g_hookLive, 0, 0) != 0) return true;
    if (!Enabled()) return false;
    if (!g_textLo || !g_textHi || g_textLo >= g_textHi) return false;
    if (kFnEquip < g_textLo || kFnEquip >= g_textHi || !PauseRttProbe::SafeRange(kFnEquip, 8)) {
        Log("locker avail: refuse equip hook target=%p", (void*)kFnEquip);
        return false;
    }
    if (g_origEquip) return true;
    const MH_STATUS c = MH_CreateHook(reinterpret_cast<void*>(kFnEquip), reinterpret_cast<void*>(&EquipHook::Detour),
                                      reinterpret_cast<void**>(&g_origEquip));
    if (c != MH_OK) {
        Log("locker avail: MH_CreateHook equip status=%d", static_cast<int>(c));
        return false;
    }
    const MH_STATUS e = MH_EnableHook(reinterpret_cast<void*>(kFnEquip));
    if (e != MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(kFnEquip));
        g_origEquip = nullptr;
        Log("locker avail: MH_EnableHook equip status=%d", static_cast<int>(e));
        return false;
    }
    InterlockedExchange(&g_hookLive, 1);
    Log("locker avail: equip hook live @ %p orig=%p (EquipFn __thiscall ret 0x0C; read-only)",
        reinterpret_cast<void*>(kFnEquip), reinterpret_cast<void*>(g_origEquip));
    return true;
}

inline void TryInstallEarly() {
    if (!Enabled()) return;
    InstallEquipHook();
}

inline void OnFocus(uintptr_t movie, uintptr_t cls) {
    if (!Enabled() || !movie || !cls) return;
    if (!IsWeaponLockerMovie(movie)) return;
    TryInstallEarly();
    if (!InterlockedCompareExchange(&g_hookLive, 0, 0)) {
        Log("locker avail: SESSION refused — equip hook not live");
        return;
    }
    g_movie = movie;
    g_cls = cls;
    InterlockedExchange(&g_session, 1);
    LogSessionOpen(movie, cls);
}

inline void OnDefocus() {
    if (!Enabled()) return;
    if (!Armed()) return;
    LogSessionClose();
    InterlockedExchange(&g_session, 0);
    g_movie = 0;
    g_cls = 0;
    g_offEquipped = -1;
}

} // namespace LockerAvailabilityTrace
