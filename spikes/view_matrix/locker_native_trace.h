#pragma once
// Bounded read-only witness for RvUIGFxMovie_WeaponLocker native exec + pickup.
// Default OFF ([Diagnostic] LockerNativeTrace=0). No TArray writes, grants, or scans.
#include "MinHook.h"
#include "locker_availability_witness.h"
#include "locker_native_trace_witness.h"
#include "inventory_retention_policy.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"
#include <cstring>
#include <intrin.h>

extern uintptr_t g_textLo, g_textHi;
extern volatile LONG g_lockerNativeTrace;
void Log(_Printf_format_string_ const char* fmt, ...);

namespace LockerNativeTrace {

constexpr uintptr_t kWeaponLockerVtable = 0x019A0948u;
constexpr uintptr_t kSlotStart = 0x11Cu;
constexpr uintptr_t kSlotPopMenu = 0x190u;
constexpr uintptr_t kSlotLoad = 0x1F4u;
constexpr uintptr_t kSlotEquip = 0x1F8u;
constexpr uintptr_t kFnStart = 0x00F11430u;
constexpr uintptr_t kFnPopMenu = 0x00F17F60u;
constexpr uintptr_t kFnLoad = 0x00F23840u;
constexpr uintptr_t kFnEquip = 0x00F00F50u;
// Page exec bodies: inspect_locker_page_methods.py (registration, NOT vtable slots).
constexpr uintptr_t kFnLoadUpgrades = 0x00F87C80u;
constexpr uintptr_t kFnUpgrade = 0x00F87E60u; // witness-only; do not hook (purchase/action exec)
constexpr const char* kPrefixLoadUpgrades16 =
    "7d4a11aa21143fd090b972fa6cd4d88bec7c600407e1e350f5d0df71c754784f";

constexpr unsigned kMethodCount = 5u;
constexpr unsigned kMaxEvents = 96u;
constexpr unsigned kMaxPerMethod = 24u;
constexpr unsigned kMaxPickupNotes = 24u;
constexpr unsigned kMaxFNameLines = 32u;
constexpr unsigned kMaxInventoryLines = 32u;
constexpr unsigned kMaxPreFocusStart = 4u;
constexpr unsigned kMaxLoadCallerWitness = 16u;

// ABI witnesses (inspect_locker_abi.py on capture-20261006-205508):
// Start / LoadEquippedWeapons / PopMenu: ret 4 -> one stack arg after ECX=self.
// equip: ret 0x0C -> three stack args after ECX=self ([ebp+0xC] slot witness).
using StartFn = void(__thiscall*)(void* self, void* frame);
using FrameFn = void(__thiscall*)(void* self, void* frame);
using EquipFn = void(__thiscall*)(void* self, void* frame, void* slotNum, void* stackArg10);
// ret 8: FFrame* + RESULT_DECL (inspect_locker_page_methods.py ABI witness).
using ScriptExecFn = void(__thiscall*)(void* self, void* frame, void* result);

inline volatile LONG g_session = 0;
inline volatile LONG g_seq = 0;
inline volatile LONG g_events = 0;
inline volatile LONG g_pickupNotes = 0;
inline volatile LONG g_fnameLines = 0;
inline volatile LONG g_inventoryLines = 0;
inline volatile LONG g_hooksLive = 0;
inline volatile LONG g_preFocusStartOpens = 0;
inline volatile LONG g_loadCallerWitness = 0;

inline uintptr_t g_movie = 0;
inline uintptr_t g_cls = 0;
inline int g_offEquipped = -1;
inline int g_offOriginal = -1;
inline int g_offActiveSlot = -1;
inline int g_offRequestedPackages = -1;
inline volatile LONG g_methodCounts[5]{};
inline volatile LONG g_lastPageLoadSignal = 0; // 0 unknown, 1 LoadEquipped, 2 LoadUpgrades
inline volatile LONG g_pageHooksLive = 0;
inline volatile LONG g_availWitnessLines = 0;
constexpr unsigned kMaxAvailWitnessLines = 24u;

inline StartFn g_origStart = nullptr;
inline EquipFn g_origEquip = nullptr;
inline FrameFn g_origLoad = nullptr;
inline FrameFn g_origPop = nullptr;
inline ScriptExecFn g_origLoadUpgrades = nullptr;

inline bool Enabled() {
    return InterlockedCompareExchange(&g_lockerNativeTrace, 0, 0) != 0;
}

inline bool Armed() {
    return InterlockedCompareExchange(&g_session, 0, 0) != 0;
}

inline bool HooksLive() {
    return InterlockedCompareExchange(&g_hooksLive, 0, 0) != 0;
}

inline thread_local int t_reenter = 0;
inline thread_local bool t_logExec[6]{};

inline bool TryBudget(unsigned methodIdx, unsigned* outSeq) {
    if (methodIdx >= kMethodCount || !outSeq) return false;
    const LONG total = InterlockedIncrement(&g_events);
    if ((unsigned)total > kMaxEvents) {
        InterlockedDecrement(&g_events);
        return false;
    }
    const LONG per = InterlockedIncrement(&g_methodCounts[methodIdx]);
    if ((unsigned)per > kMaxPerMethod) {
        InterlockedDecrement(&g_methodCounts[methodIdx]);
        InterlockedDecrement(&g_events);
        return false;
    }
    *outSeq = (unsigned)InterlockedIncrement(&g_seq);
    return true;
}

inline bool TryLineCap(volatile LONG* counter, unsigned maxLines) {
    for (;;) {
        const LONG n = InterlockedCompareExchange(counter, 0, 0);
        if ((unsigned)n >= maxLines) return false;
        if (InterlockedCompareExchange(counter, n + 1, n) == n) return true;
    }
}

inline uintptr_t VtSlot(uintptr_t vt, unsigned off) {
    if (!vt || !PauseRttProbe::SafeRange(vt + off, 4)) return 0;
    return PauseRttProbe::Pointer(vt + off);
}

inline bool IdentityOk(uintptr_t vt) {
    return vt && VtSlot(vt, kSlotStart) == kFnStart && VtSlot(vt, kSlotPopMenu) == kFnPopMenu &&
           VtSlot(vt, kSlotLoad) == kFnLoad && VtSlot(vt, kSlotEquip) == kFnEquip;
}

inline bool IsWeaponLockerMovie(uintptr_t movie) {
    if (!movie || !PauseRttProbe::SafeRange(movie + OBJ_CLASS, 4)) return false;
    const uintptr_t vt = PauseRttProbe::Pointer(movie);
    if (!IdentityOk(vt)) return false;
    const uintptr_t cls = PauseRttProbe::Pointer(movie + OBJ_CLASS);
    char clsName[96]{};
    return cls && NameOf(cls, clsName, sizeof(clsName)) && !strcmp(clsName, "RvUIGFxMovie_WeaponLocker");
}

inline void CacheMovieLayout(uintptr_t movie, uintptr_t cls) {
    g_movie = movie;
    g_cls = cls;
    g_offEquipped = PauseRttProbe::Property(cls, "mEquippedWeapons", "ArrayProperty");
    g_offOriginal = PauseRttProbe::Property(cls, "mOriginalWeapons", "ArrayProperty");
    g_offActiveSlot = PauseRttProbe::Property(cls, "mActiveWeaponSlot", "IntProperty");
    g_offRequestedPackages = PauseRttProbe::Property(cls, "mRequestedPackages", "IntProperty");
}

inline const char* PageLoadLabel(LONG signal) {
    switch (signal) {
    case 1: return "LoadEquippedWeapons";
    case 2: return "LoadUpgrades";
    default: return "Unknown";
    }
}

inline bool ReadEquippedSlotFName(uintptr_t movie, int index, char* out, unsigned outLen) {
    if (!out || outLen < 2 || g_offEquipped < 0 || !movie) return false;
    out[0] = 0;
    if (!PauseRttProbe::SafeRange(movie + (uintptr_t)g_offEquipped, 12)) return false;
    const uintptr_t data = PauseRttProbe::Pointer(movie + (uintptr_t)g_offEquipped);
    const int count = *reinterpret_cast<const int*>(movie + (uintptr_t)g_offEquipped + 4);
    if (index < 0 || index >= count || !data) return false;
    if (!PauseRttProbe::SafeRange(data + (uintptr_t)index * 8, 8)) return false;
    const NativeOwnedInventory::Name nm =
        *reinterpret_cast<const NativeOwnedInventory::Name*>(data + (uintptr_t)index * 8);
    NameFromIndex(nm.index, out, (int)outLen);
    return out[0] != 0;
}

inline void LogEventContext(unsigned seq, const char* method, void* self) {
    const uintptr_t movie = (uintptr_t)self;
    int activeSlot = -999, reqPkg = -999;
    if (movie == g_movie && g_cls) {
        if (g_offActiveSlot >= 0 && PauseRttProbe::SafeRange(movie + (uintptr_t)g_offActiveSlot, 4))
            activeSlot = *reinterpret_cast<const int*>(movie + (uintptr_t)g_offActiveSlot);
        if (g_offRequestedPackages >= 0 && PauseRttProbe::SafeRange(movie + (uintptr_t)g_offRequestedPackages, 4))
            reqPkg = *reinterpret_cast<const int*>(movie + (uintptr_t)g_offRequestedPackages);
    }
    char eq0[64]{}, eq1[64]{};
    if (movie == g_movie) {
        ReadEquippedSlotFName(movie, 0, eq0, sizeof(eq0));
        ReadEquippedSlotFName(movie, 1, eq1, sizeof(eq1));
    }
    const unsigned owned = NativeOwnedInventory::Mask();
    const int m0 = inventoryretention::WeaponMesh(eq0);
    const int m1 = inventoryretention::WeaponMesh(eq1);
    Log("locker native trace: ctx #%u %s tid=%u movie=%p activeSlot=%d reqPkg=%d eq0=%s(%d) eq1=%s(%d) "
        "owned=0x%X lastPageLoad=%s",
        seq, method, GetCurrentThreadId(), (void*)movie, activeSlot, reqPkg, eq0[0] ? eq0 : "-", m0,
        eq1[0] ? eq1 : "-", m1, owned, PageLoadLabel(InterlockedCompareExchange(&g_lastPageLoadSignal, 0, 0)));
}

inline void MaybeLogAvailability(const char* method) {
    if (InterlockedIncrement(&g_availWitnessLines) > (LONG)kMaxAvailWitnessLines) {
        InterlockedDecrement(&g_availWitnessLines);
        return;
    }
    LockerAvailabilityWitness::LogInventoryAvailability(method);
    LockerAvailabilityWitness::LogSessionWeaponsTable(method);
}

inline void LogCachedOwned(const char* tag) {
    if (!TryLineCap(&g_inventoryLines, kMaxInventoryLines)) return;
    const unsigned mask = NativeOwnedInventory::Mask();
    const unsigned cnt = NativeOwnedInventory::PossessionCount();
    Log("locker native trace: %s owned-cache mask=0x%X count=%u pollTid=%u hookTid=%u (Poll() refresh only)",
        tag, mask, cnt, NativeOwnedInventory::PollThreadId(), GetCurrentThreadId());
}

inline void LogEquippedTArray(const char* tag, uintptr_t movie, int off, const char* label) {
    if (off < 0 || !movie || !PauseRttProbe::SafeRange(movie + (uintptr_t)off, 12)) {
        Log("locker native trace: %s %s unresolved", tag, label);
        return;
    }
    if (!TryLineCap(&g_fnameLines, kMaxFNameLines)) return;
    const uintptr_t data = PauseRttProbe::Pointer(movie + (uintptr_t)off);
    const int count = *reinterpret_cast<const int*>(movie + (uintptr_t)off + 4);
    const int cap = *reinterpret_cast<const int*>(movie + (uintptr_t)off + 8);
    char names[256]{};
    int pos = 0;
    const int n = count < 0 ? 0 : (count > 6 ? 6 : count);
    for (int i = 0; i < n && pos < (int)sizeof(names) - 2; ++i) {
        if (!data || !PauseRttProbe::SafeRange(data + (uintptr_t)i * 8, 8)) break;
        const NativeOwnedInventory::Name nm =
            *reinterpret_cast<const NativeOwnedInventory::Name*>(data + (uintptr_t)i * 8);
        char buf[48]{};
        NameFromIndex(nm.index, buf, sizeof(buf));
        if (i) names[pos++] = ',';
        for (int c = 0; buf[c] && pos < (int)sizeof(names) - 1; ++c) names[pos++] = buf[c];
    }
    names[pos] = 0;
    Log("locker native trace: %s %s count=%d cap=%d fnames=[%s] (equipped/original GFx TArray; not station catalog)",
        tag, label, count, cap, names);
}

inline void LogEnter(const char* method, unsigned methodIdx, void* self, const char* argNote) {
    if (!Enabled() || !Armed() || !LockerNativeTraceWitness::LogEnterPermitted(t_reenter)) return;
    unsigned seq = 0;
    if (!TryBudget(methodIdx, &seq)) return;
    t_logExec[methodIdx] = true;
    char obj[96]{}, clsName[96]{};
    const uintptr_t movie = (uintptr_t)self;
    const uintptr_t cls = movie ? PauseRttProbe::Pointer(movie + OBJ_CLASS) : 0;
    if (movie) NameOf(movie, obj, sizeof(obj));
    if (cls) NameOf(cls, clsName, sizeof(clsName));
    Log("locker native trace: #%u %s ENTER tid=%u movie=%p sessionMovie=%p frameNote=%s obj=%s cls=%s",
        seq, method, GetCurrentThreadId(), (void*)movie, (void*)g_movie, argNote ? argNote : "-", obj, clsName);
    if (movie == g_movie && cls == g_cls) LogEventContext(seq, method, self);
    if (movie == g_movie && cls == g_cls) {
        LogEquippedTArray("ENTER", movie, g_offEquipped, "mEquippedWeapons");
        LogEquippedTArray("ENTER", movie, g_offOriginal, "mOriginalWeapons");
    }
    LogCachedOwned("ENTER");
    if (!strcmp(method, "equip") || !strcmp(method, "LoadEquippedWeapons"))
        MaybeLogAvailability(method);
}

inline void LogExit(const char* method, unsigned methodIdx, void* self) {
    if (!Enabled() || !Armed() || !t_logExec[methodIdx]) return;
    t_logExec[methodIdx] = false;
    const uintptr_t movie = (uintptr_t)self;
    Log("locker native trace: %s EXIT tid=%u movie=%p", method, GetCurrentThreadId(), (void*)movie);
    if (movie == g_movie && g_cls) {
        LogEquippedTArray("EXIT", movie, g_offEquipped, "mEquippedWeapons");
        LogEquippedTArray("EXIT", movie, g_offOriginal, "mOriginalWeapons");
    }
    LogCachedOwned("EXIT");
}

inline void ResetSessionCounters() {
    for (unsigned i = 0; i < kMethodCount; ++i) InterlockedExchange(&g_methodCounts[i], 0);
    InterlockedExchange(&g_events, 0);
    InterlockedExchange(&g_seq, 0);
    InterlockedExchange(&g_pickupNotes, 0);
    InterlockedExchange(&g_fnameLines, 0);
    InterlockedExchange(&g_inventoryLines, 0);
    InterlockedExchange(&g_availWitnessLines, 0);
    InterlockedExchange(&g_lastPageLoadSignal, 0);
}

inline bool TryArmSessionFromStart(void* self) {
    const uintptr_t movie = (uintptr_t)self;
    if (!IsWeaponLockerMovie(movie)) return Armed();
    if (Armed() && g_movie == movie) return true;
    const LONG n = InterlockedIncrement(&g_preFocusStartOpens);
    if ((unsigned)n > kMaxPreFocusStart) {
        InterlockedDecrement(&g_preFocusStartOpens);
        return false;
    }
    const uintptr_t cls = PauseRttProbe::Pointer(movie + OBJ_CLASS);
    CacheMovieLayout(movie, cls);
    ResetSessionCounters();
    InterlockedExchange(&g_session, 1);
    char obj[96]{};
    NameOf(movie, obj, sizeof(obj));
    Log("locker native trace: SESSION OPEN (pre-focus Start) movie=%p cls=%p obj=%s tid=%u",
        (void*)movie, (void*)cls, obj, GetCurrentThreadId());
    LogCachedOwned("SESSION");
    return true;
}

struct ExecHooks {
    static void __thiscall Start(void* self, void* frame) {
        ++t_reenter;
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) {
            TryArmSessionFromStart(self);
            LogEnter("Start", 0, self, "frame");
        }
        g_origStart(self, frame);
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) LogExit("Start", 0, self);
        --t_reenter;
    }
    static void __thiscall Equip(void* self, void* frame, void* slotNum, void* stackArg10) {
        ++t_reenter;
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) {
            char note[64]{};
            _snprintf_s(note, _TRUNCATE, "slot=%d stackArg10=%p", (int)(intptr_t)slotNum, stackArg10);
            LogEnter("equip", 1, self, note);
        }
        g_origEquip(self, frame, slotNum, stackArg10);
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) LogExit("equip", 1, self);
        --t_reenter;
    }
    static void __thiscall Load(void* self, void* frame) {
        ++t_reenter;
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) {
            InterlockedExchange(&g_lastPageLoadSignal, 1);
            char note[96]{};
            if (TryLineCap(&g_loadCallerWitness, kMaxLoadCallerWitness)) {
                const uintptr_t callerRet = (uintptr_t)_ReturnAddress();
                _snprintf_s(note, _TRUNCATE, "frame callerRet=%p", (void*)callerRet);
            } else {
                _snprintf_s(note, _TRUNCATE, "frame callerRet=cap");
            }
            LogEnter("LoadEquippedWeapons", 2, self, note);
        }
        g_origLoad(self, frame);
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) LogExit("LoadEquippedWeapons", 2, self);
        --t_reenter;
    }
    static void __thiscall Pop(void* self, void* frame) {
        ++t_reenter;
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) LogEnter("PopMenu", 3, self, "frame");
        g_origPop(self, frame);
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) LogExit("PopMenu", 3, self);
        --t_reenter;
    }
    static void __thiscall LoadUpgrades(void* self, void* frame, void* result) {
        ++t_reenter;
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) {
            InterlockedExchange(&g_lastPageLoadSignal, 2);
            LogEnter("LoadUpgrades", 4, self, "frame");
        }
        g_origLoadUpgrades(self, frame, result);
        if (LockerNativeTraceWitness::AllowOuterExecWitness(t_reenter)) LogExit("LoadUpgrades", 4, self);
        --t_reenter;
    }
};

inline void DisableOne(uintptr_t target, void** origSlot) {
    if (!*origSlot) return;
    MH_DisableHook((void*)target);
    MH_RemoveHook((void*)target);
    *origSlot = nullptr;
}

inline void RollbackAllHooks() {
    DisableOne(kFnStart, (void**)&g_origStart);
    DisableOne(kFnEquip, (void**)&g_origEquip);
    DisableOne(kFnLoad, (void**)&g_origLoad);
    DisableOne(kFnPopMenu, (void**)&g_origPop);
    DisableOne(kFnLoadUpgrades, (void**)&g_origLoadUpgrades);
    InterlockedExchange(&g_hooksLive, 0);
    InterlockedExchange(&g_pageHooksLive, 0);
}

inline bool Prefix16Matches(uintptr_t body) {
    if (!body || !PauseRttProbe::SafeRange(body, 16)) return false;
    static const unsigned char kLoadUpgradesPrefix[16] = {0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68, 0x68, 0x67,
                                                          0x6E, 0x01, 0x64, 0xA1, 0x00, 0x00, 0x00, 0x00};
    if (body == kFnLoadUpgrades) return memcmp((void*)body, kLoadUpgradesPrefix, 16) == 0;
    return false;
}

inline bool InstallOne(uintptr_t target, void* detour, void** orig, const char* label) {
    if (target < g_textLo || target >= g_textHi || !PauseRttProbe::SafeRange(target, 8)) {
        Log("locker native trace: refuse %s target=%p bounds", label, (void*)target);
        return false;
    }
    if (*orig) return true;
    const MH_STATUS c = MH_CreateHook((void*)target, detour, orig);
    if (c != MH_OK) {
        Log("locker native trace: MH_CreateHook %s status=%d", label, (int)c);
        return false;
    }
    const MH_STATUS e = MH_EnableHook((void*)target);
    if (e != MH_OK) {
        MH_RemoveHook((void*)target);
        *orig = nullptr;
        Log("locker native trace: MH_EnableHook %s status=%d", label, (int)e);
        return false;
    }
    Log("locker native trace: hooked %s at %p orig=%p (read-only witness)", label, (void*)target, *orig);
    return true;
}

inline bool InstallPageHooks() {
    if (InterlockedCompareExchange(&g_pageHooksLive, 0, 0) != 0) return true;
    if (!Prefix16Matches(kFnLoadUpgrades)) {
        Log("locker native trace: LoadUpgrades prefix refused (re-run inspect_locker_page_methods.py)");
        return false;
    }
    const bool ok =
        InstallOne(kFnLoadUpgrades, (void*)&ExecHooks::LoadUpgrades, (void**)&g_origLoadUpgrades, "LoadUpgrades");
    if (!ok) {
        DisableOne(kFnLoadUpgrades, (void**)&g_origLoadUpgrades);
        Log("locker native trace: page hook install aborted");
        return false;
    }
    InterlockedExchange(&g_pageHooksLive, 1);
    Log("locker native trace: page hook live LoadUpgrades=%p (ScriptExecFn ret8; Upgrade @ %p unhooked)",
        (void*)kFnLoadUpgrades, (void*)kFnUpgrade);
    return true;
}

inline bool InstallExecHooks(uintptr_t vt) {
    if (!Enabled()) return false;
    if (HooksLive()) return true;
    if (!IdentityOk(vt)) {
        Log("locker native trace: vtable identity refused vt=%p", (void*)vt);
        return false;
    }
    const bool ok = InstallOne(kFnStart, (void*)&ExecHooks::Start, (void**)&g_origStart, "Start") &&
                    InstallOne(kFnEquip, (void*)&ExecHooks::Equip, (void**)&g_origEquip, "equip") &&
                    InstallOne(kFnLoad, (void*)&ExecHooks::Load, (void**)&g_origLoad, "LoadEquippedWeapons") &&
                    InstallOne(kFnPopMenu, (void*)&ExecHooks::Pop, (void**)&g_origPop, "PopMenu");
    if (!ok) {
        RollbackAllHooks();
        Log("locker native trace: install aborted — all hooks removed");
        return false;
    }
    if (!InstallPageHooks()) {
        RollbackAllHooks();
        Log("locker native trace: install aborted — page hooks failed");
        return false;
    }
    InterlockedExchange(&g_hooksLive, 1);
    Log("locker native trace: exec hooks live (max %u events, %u/method x%u; ctx logs activeSlot/reqPkg/eq0/eq1)",
        kMaxEvents, kMaxPerMethod, kMethodCount);
    return true;
}

inline void TryInstallEarly() {
    if (!Enabled() || HooksLive()) return;
    // InstallOne compares hook RVAs against g_textLo/g_textHi; attempting before the
    // executable .text range is logged causes thousands of refuse/aborted lines per session.
    if (!g_textLo || !g_textHi || g_textLo >= g_textHi) return;
    if (!PauseRttProbe::SafeRange(kWeaponLockerVtable, 16)) return;
    InstallExecHooks(kWeaponLockerVtable);
}

inline void OnFocus(uintptr_t movie, uintptr_t cls) {
    if (!Enabled() || !movie || !cls) return;
    const uintptr_t vt = PauseRttProbe::Pointer(movie);
    if (!IdentityOk(vt) && !IdentityOk(kWeaponLockerVtable)) {
        static unsigned refuse = 0;
        if (refuse++ < 2)
            Log("locker native trace: focus ignored — vtable mismatch movie=%p vt=%p", (void*)movie, (void*)vt);
        return;
    }
    TryInstallEarly();
    if (!HooksLive()) {
        Log("locker native trace: SESSION refused — hooks not live");
        return;
    }
    if (Armed() && g_movie == movie) {
        CacheMovieLayout(movie, cls);
        Log("locker native trace: SESSION reaffirm (focus after pre-focus Start) movie=%p tid=%u",
            (void*)movie, GetCurrentThreadId());
        return;
    }
    CacheMovieLayout(movie, cls);
    ResetSessionCounters();
    InterlockedExchange(&g_session, 1);
    char obj[96]{};
    NameOf(movie, obj, sizeof(obj));
    Log("locker native trace: SESSION OPEN movie=%p cls=%p obj=%s tid=%u equippedOff=%d originalOff=%d "
        "activeSlotOff=%d reqPkgOff=%d",
        (void*)movie, (void*)cls, obj, GetCurrentThreadId(), g_offEquipped, g_offOriginal, g_offActiveSlot,
        g_offRequestedPackages);
    LogCachedOwned("SESSION");
}

inline void OnDefocus() {
    if (!Enabled()) return;
    if (!Armed()) return;
    Log("locker native trace: SESSION CLOSE events=%ld seq=%ld tid=%u",
        InterlockedCompareExchange(&g_events, 0, 0), InterlockedCompareExchange(&g_seq, 0, 0),
        GetCurrentThreadId());
    LogCachedOwned("SESSION");
    InterlockedExchange(&g_session, 0);
    g_movie = 0;
    g_cls = 0;
    g_offEquipped = -1;
    g_offOriginal = -1;
    g_offActiveSlot = -1;
    g_offRequestedPackages = -1;
    InterlockedExchange(&g_lastPageLoadSignal, 0);
}

inline void ObservePickup(void* self, void* archetype, int keepIn, int keepOut, int result) {
    if (!Enabled()) return;
    if (!TryLineCap(&g_pickupNotes, kMaxPickupNotes)) return;
    const unsigned seq = (unsigned)InterlockedIncrement(&g_seq);
    char recv[96]{}, arch[96]{}, archClass[96]{};
    const uintptr_t a = (uintptr_t)archetype;
    NameOf((uintptr_t)self, recv, sizeof(recv));
    if (a) {
        NameOf(a, arch, sizeof(arch));
        const uintptr_t ac = PauseRttProbe::Pointer(a + OBJ_CLASS);
        if (ac) NameOf(ac, archClass, sizeof(archClass));
    }
    Log("locker native trace: #%u pickup tid=%u armed=%d self=%p archetype=%p archName=%s archClass=%s keep=%d->%d rc=%d",
        seq, GetCurrentThreadId(), Armed() ? 1 : 0, self, archetype, arch, archClass, keepIn, keepOut, result);
    LogCachedOwned("pickup");
}

} // namespace LockerNativeTrace
