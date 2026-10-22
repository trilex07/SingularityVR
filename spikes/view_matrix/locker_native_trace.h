#pragma once
// Bounded read-only witness for RvUIGFxMovie_WeaponLocker native exec + pickup.
// Default OFF ([Diagnostic] LockerNativeTrace=0). No TArray writes, grants, or scans.
#include "MinHook.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"

extern uintptr_t g_textLo, g_textHi;
extern volatile LONG g_lockerNativeTrace;
void Log(_Printf_format_string_ const char* fmt, ...);

namespace LockerNativeTrace {

constexpr uintptr_t kVtableClass = 0x019A0948u;
constexpr uintptr_t kSlotStart = 0x11Cu;
constexpr uintptr_t kSlotPopMenu = 0x190u;
constexpr uintptr_t kSlotLoad = 0x1F4u;
constexpr uintptr_t kSlotEquip = 0x1F8u;
constexpr uintptr_t kFnStart = 0x00F11430u;
constexpr uintptr_t kFnPopMenu = 0x00F17F60u;
constexpr uintptr_t kFnLoad = 0x00F23840u;
constexpr uintptr_t kFnEquip = 0x00F00F50u;

constexpr unsigned kMaxEvents = 64u;
constexpr unsigned kMaxPerMethod = 20u;

using ExecFn = void(__thiscall*)(void* self, void* frame, void* result);

inline volatile LONG g_session = 0;
inline volatile LONG g_seq = 0;
inline volatile LONG g_events = 0;
inline uintptr_t g_movie = 0;
inline uintptr_t g_cls = 0;
inline unsigned g_methodCounts[4]{};

inline ExecFn g_origStart = nullptr;
inline ExecFn g_origEquip = nullptr;
inline ExecFn g_origLoad = nullptr;
inline ExecFn g_origPop = nullptr;
inline bool g_hooksInstalled = false;

inline bool Enabled() {
    return InterlockedCompareExchange(&g_lockerNativeTrace, 0, 0) != 0;
}

inline bool Armed() {
    return InterlockedCompareExchange(&g_session, 0, 0) != 0;
}

inline bool Budget(unsigned methodIdx) {
    if (g_events >= kMaxEvents || methodIdx >= 4 || g_methodCounts[methodIdx] >= kMaxPerMethod)
        return false;
    ++g_methodCounts[methodIdx];
    ++g_events;
    return true;
}

inline uintptr_t VtSlot(uintptr_t vt, unsigned off) {
    if (!vt || !PauseRttProbe::SafeRange(vt + off, 4)) return 0;
    return PauseRttProbe::Pointer(vt + off);
}

inline bool IdentityOk(uintptr_t vt) {
    return vt && VtSlot(vt, kSlotStart) == kFnStart && VtSlot(vt, kSlotPopMenu) == kFnPopMenu &&
           VtSlot(vt, kSlotLoad) == kFnLoad && VtSlot(vt, kSlotEquip) == kFnEquip;
}

inline void LogFNameList(const char* tag, uintptr_t movie, uintptr_t cls, const char* field) {
    const int off = PauseRttProbe::Property(cls, field, "ArrayProperty");
    if (off < 0 || !movie || !PauseRttProbe::SafeRange(movie + off, 12)) {
        Log("locker native trace: %s %s unresolved", tag, field);
        return;
    }
    const uintptr_t data = PauseRttProbe::Pointer(movie + off);
    const int count = *reinterpret_cast<const int*>(movie + off + 4);
    const int cap = *reinterpret_cast<const int*>(movie + off + 8);
    char names[256]{};
    int pos = 0;
    const int n = count < 0 ? 0 : (count > 6 ? 6 : count);
    for (int i = 0; i < n && pos < (int)sizeof(names) - 2; ++i) {
        if (!data || !PauseRttProbe::SafeRange(data + (uintptr_t)i * 8, 8)) break;
        const NativeOwnedInventory::Name nm = *reinterpret_cast<const NativeOwnedInventory::Name*>(data + (uintptr_t)i * 8);
        char buf[48]{};
        NameFromIndex(nm.index, buf, sizeof(buf));
        if (i) names[pos++] = ',';
        for (int c = 0; buf[c] && pos < (int)sizeof(names) - 1; ++c) names[pos++] = buf[c];
    }
    names[pos] = 0;
    Log("locker native trace: %s %s count=%d cap=%d fnames=[%s]", tag, field, count, cap, names);
}

inline void LogInventory(const char* tag) {
    unsigned mask = 0;
    unsigned cnt = 0;
    __try {
        NativeOwnedInventory::Refresh();
        if (NativeOwnedInventory::SnapshotNow()) {
            mask = NativeOwnedInventory::Mask();
            cnt = NativeOwnedInventory::PossessionCount();
        } else {
            mask = NativeOwnedInventory::Mask();
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        mask = NativeOwnedInventory::Mask();
    }
    Log("locker native trace: %s owned mask=0x%X count=%u pollTid=%u hookTid=%u",
        tag, mask, cnt, NativeOwnedInventory::PollThreadId(), GetCurrentThreadId());
}

inline void LogEnter(const char* method, unsigned methodIdx, void* self, void* frame) {
    if (!Enabled() || !Armed() || !Budget(methodIdx)) return;
    const unsigned n = (unsigned)InterlockedIncrement(&g_seq);
    char obj[96]{}, clsName[96]{};
    const uintptr_t movie = (uintptr_t)self;
    const uintptr_t cls = movie ? PauseRttProbe::Pointer(movie + OBJ_CLASS) : 0;
    if (movie) NameOf(movie, obj, sizeof(obj));
    if (cls) NameOf(cls, clsName, sizeof(clsName));
    Log("locker native trace: #%u %s ENTER tid=%u movie=%p exp=%p frame=%p obj=%s cls=%s",
        n, method, GetCurrentThreadId(), (void*)movie, (void*)g_movie, frame, obj, clsName);
    if (movie == g_movie && cls == g_cls) {
        LogFNameList("ENTER", movie, cls, "mEquippedWeapons");
        LogFNameList("ENTER", movie, cls, "mOriginalWeapons");
    }
    LogInventory("ENTER");
    (void)frame;
}

inline void LogExit(const char* method, unsigned methodIdx, void* self) {
    if (!Enabled() || !Armed()) return;
    const uintptr_t movie = (uintptr_t)self;
    Log("locker native trace: %s EXIT tid=%u movie=%p", method, GetCurrentThreadId(), (void*)movie);
    if (movie == g_movie && g_cls) {
        LogFNameList("EXIT", movie, g_cls, "mEquippedWeapons");
        LogFNameList("EXIT", movie, g_cls, "mOriginalWeapons");
    }
    LogInventory("EXIT");
    (void)methodIdx;
}

struct ExecHooks {
    static void __thiscall Start(void* self, void* frame, void* result) {
        LogEnter("Start", 0, self, frame);
        g_origStart(self, frame, result);
        LogExit("Start", 0, self);
    }
    static void __thiscall Equip(void* self, void* frame, void* result) {
        LogEnter("equip", 1, self, frame);
        g_origEquip(self, frame, result);
        LogExit("equip", 1, self);
    }
    static void __thiscall Load(void* self, void* frame, void* result) {
        LogEnter("LoadEquippedWeapons", 2, self, frame);
        g_origLoad(self, frame, result);
        LogExit("LoadEquippedWeapons", 2, self);
    }
    static void __thiscall Pop(void* self, void* frame, void* result) {
        LogEnter("PopMenu", 3, self, frame);
        g_origPop(self, frame, result);
        LogExit("PopMenu", 3, self);
    }
};

inline bool InstallHook(uintptr_t target, void* detour, ExecFn* orig, const char* label) {
    if (target < g_textLo || target >= g_textHi || !PauseRttProbe::SafeRange(target, 8)) {
        Log("locker native trace: refuse %s target=%p bounds", label, (void*)target);
        return false;
    }
    if (*orig) return true;
    const MH_STATUS c = MH_CreateHook((void*)target, detour, (void**)orig);
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
    Log("locker native trace: hooked %s at %p orig=%p (read-only witness)", label, (void*)target, (void*)*orig);
    return true;
}

inline bool InstallExecHooks(uintptr_t vt) {
    if (g_hooksInstalled) return true;
    if (!IdentityOk(vt)) {
        Log("locker native trace: vtable identity refused vt=%p (expected Start/Pop/Load/equip RVAs)", (void*)vt);
        return false;
    }
    bool ok = InstallHook(kFnStart, (void*)&ExecHooks::Start, &g_origStart, "Start") &&
              InstallHook(kFnEquip, (void*)&ExecHooks::Equip, &g_origEquip, "equip") &&
              InstallHook(kFnLoad, (void*)&ExecHooks::Load, &g_origLoad, "LoadEquippedWeapons") &&
              InstallHook(kFnPopMenu, (void*)&ExecHooks::Pop, &g_origPop, "PopMenu");
    if (ok) {
        g_hooksInstalled = true;
        Log("locker native trace: exec hooks armed (max %u events, %u/method)", kMaxEvents, kMaxPerMethod);
    }
    return ok;
}

inline void OnFocus(uintptr_t movie, uintptr_t cls) {
    if (!Enabled() || !movie || !cls) return;
    const uintptr_t vt = PauseRttProbe::Pointer(movie);
    if (!IdentityOk(vt)) {
        static unsigned refuse = 0;
        if (refuse++ < 2)
            Log("locker native trace: focus ignored — vtable mismatch movie=%p vt=%p", (void*)movie, (void*)vt);
        return;
    }
    g_movie = movie;
    g_cls = cls;
    memset(g_methodCounts, 0, sizeof(g_methodCounts));
    g_events = 0;
    InterlockedExchange(&g_seq, 0);
    InterlockedExchange(&g_session, 1);
    char obj[96]{};
    NameOf(movie, obj, sizeof(obj));
    Log("locker native trace: SESSION OPEN movie=%p cls=%p obj=%s tid=%u",
        (void*)movie, (void*)cls, obj, GetCurrentThreadId());
    LogInventory("SESSION");
    InstallExecHooks(vt);
}

inline void OnDefocus() {
    if (!Enabled() || !Armed()) return;
    Log("locker native trace: SESSION CLOSE events=%u seq=%u tid=%u",
        g_events, (unsigned)InterlockedCompareExchange(&g_seq, 0, 0), GetCurrentThreadId());
    LogInventory("SESSION");
    InterlockedExchange(&g_session, 0);
    g_movie = 0;
    g_cls = 0;
}

inline void ObservePickup(void* self, void* archetype, int keepIn, int keepOut, int result) {
    if (!Enabled() || g_events >= kMaxEvents) return;
    // Log pickup always when trace INI on (station may grant outside armed session).
    static unsigned pickupNotes = 0;
    if (pickupNotes >= 24) return;
    ++pickupNotes;
    ++g_events;
    char recv[96]{}, arch[96]{}, archClass[96]{};
    const uintptr_t a = (uintptr_t)archetype;
    NameOf((uintptr_t)self, recv, sizeof(recv));
    if (a) {
        NameOf(a, arch, sizeof(arch));
        const uintptr_t ac = PauseRttProbe::Pointer(a + OBJ_CLASS);
        if (ac) NameOf(ac, archClass, sizeof(archClass));
    }
    const unsigned n = (unsigned)InterlockedIncrement(&g_seq);
    Log("locker native trace: #%u pickup tid=%u armed=%d self=%p archetype=%p archName=%s archClass=%s keep=%d->%d rc=%d",
        n, GetCurrentThreadId(), Armed() ? 1 : 0, self, archetype, arch, archClass, keepIn, keepOut, result);
    LogInventory("pickup");
}

} // namespace LockerNativeTrace
