#pragma once
#include "tmd_haptics.h"

// Native TMD power event source for tmd_haptics.h. Observation only: detours call the
// original method FIRST, forward its return value untouched, and only then record an event.
// Absolutely no UObject, energy, inventory, input, or camera writes.
//
// DISASSEMBLY & RUNTIME EVIDENCE (Owned original Singularity.exe + archived .text):
//  * Script exec wrappers URvTMDPower::execStartFirePower (0xF5D370, 96 B, FNV 7B8C05DBDEEC95C5)
//    and execStopFirePower (0xF5D3D0, 96 B, FNV D5110E0A85330314) read one object argument
//    and execute a direct __thiscall: Start -> 0x100D790(this=power, RvTMD*), Stop -> 0x100AEC0.
//  * 0x100D790 (248 B, ret 4, FNV 894F69CF191C20C7):
//    1. Calls power->vtable[+0x11C](tmd, 0), which points to 0x100CD00 (CheckPowerFail).
//    2. 0x100CD00 checks:
//       - 0x100CD7A: Cooldown timer comparison (comiss xmm0, [0x179E148]); returns 3 if active.
//       - 0x100CDB6: Energy balance check (call 0xFFCD50); returns 2 if insufficient energy.
//       - Target/readiness checks; returns 1 on invalid target / not ready.
//       - If all checks pass, returns 0 (SUCCESS).
//    3. At 0x100D7AB: `test ebx, ebx; jne 0x100D87F` -> if CheckPowerFail returns nonzero,
//       the entire activation body (state call 0x10105C0, FX hooks, notifications) is skipped!
//       At 0x100D881: `mov al, bl; ret 4` returns the nonzero failure code (1, 2, or 3).
//    4. If CheckPowerFail returns 0, the activation logic executes and 0 is returned.
//    PROVEN: ret == 0 strictly indicates confirmed native activation (energy and cooldown OK).
//            ret != 0 strictly indicates failure (1 = not ready/invalid target, 2 = no energy, 3 = cooldown).
//
// Thread safety:
//  - Lock-guaranteed configuration, notification, and clearing.
//  - Guaranteed clear on focus loss, tracking loss, pause, cinematic, menu, or shutdown.
//  - Fail-closed hook installation: any error during create or enable unhooks both cleanly.
namespace NativeTmdHaptics {
using StartFn = unsigned char (__fastcall*)(void*, void*, void*);
using StopFn  = int (__fastcall*)(void*, void*, void*);
static StartFn originalStart = nullptr;
static StopFn  originalStop  = nullptr;

constexpr uintptr_t kImageBase = 0x400000;
constexpr uintptr_t kStartAddr = 0x100D790, kStopAddr = 0x100AEC0;
constexpr size_t    kStartSize = 248,       kStopSize = 194;
constexpr unsigned long long kStartHash = 0x894F69CF191C20C7ull;
constexpr unsigned long long kStopHash  = 0x1563D1E2018602DEull;

static SRWLOCK lock = SRWLOCK_INIT;
static tmdhaptics::Config config;
static tmdhaptics::Engine engine;
static volatile LONG cachedAmpMilli = 0;
static volatile LONG hooksReady = 0;
static volatile LONG installAttempts = 0;

static tmdhaptics::Power Classify(uintptr_t power, char* outName, size_t outNameCap,
                                  bool& readable, bool& live, uintptr_t& cls) {
    using namespace NativeOwnedInventory;
    using tmdhaptics::Power;
    if (outName && outNameCap) outName[0] = '\0';
    readable = power && Readable((const void*)power, 0x40);
    live = readable && IsLive(power);
    cls = readable ? P(power + OBJ_CLASS) : 0;
    if (!readable || !cls || !Readable((const void*)cls, 0x40)) return Power::None;
    if (outName && outNameCap) NameOf(cls, outName, outNameCap);

    // A native power receiver can be a TMD-owned subobject whose Outer chain
    // does not satisfy IsLive's PersistentLevel heuristic. Its direct class
    // identity is still available even before superclass reflection resolves.
    const auto direct = tmdhaptics::ClassifyExactName(outName);
    if (direct != Power::None) return direct;

    static const struct { const char* name; Power p; } kMap[] = {
        { "RvTMDPower_Impulse",  Power::Impulse },
        { "RvTMDPower_Age",      Power::Age },
        { "RvTMDPower_AntiGrav", Power::AntiGrav },
        { "RvTMDPower_Deadlock", Power::Deadlock },
        { "RvTMDPower_Heal",     Power::Heal },
        { "RvTMDPower_Shield",   Power::Shield },
        { "RvTMDPower_Teleport", Power::Teleport }
    };
    for (const auto& e : kMap) {
        if (ClassHas(cls, e.name) || NativeTmdAim::ExactClass(cls, e.name)) {
            return e.p;
        }
    }
    return Power::None;
}

static const char* Name(tmdhaptics::Power p) {
    static const char* n[] = { "None", "Impulse", "Age", "AntiGrav", "Deadlock", "Heal", "Shield", "Teleport", "Generic" };
    return n[(unsigned)p < 9 ? (unsigned)p : 0];
}

static bool IsLocal(uintptr_t tmd) {
    if (!tmd || !IsLive(tmd)) return false;
    uintptr_t pawn = 0;
    const uintptr_t expectedTmd = NativeTmdAim::LocalTmd(pawn);
    if (expectedTmd && expectedTmd == tmd) return true;
    const uintptr_t owner = NativeOwnedInventory::P(tmd + 0x98);
    if (!owner || !IsLive(owner)) return false;
    if (g_playerPawn && owner == g_playerPawn) return true;
    pawn = NativeOwnedInventory::CameraOwnedPawn();
    if (!pawn) pawn = EngineNativeLocalPlayerPawn();
    if (pawn && owner == pawn) return true;
    return false;
}

static void ClearAll() {
    InterlockedExchange(&cachedAmpMilli, 0);
    AcquireSRWLockExclusive(&lock);
    engine.Clear();
    ReleaseSRWLockExclusive(&lock);
}

static void Configure(bool enabled, tmdhaptics::Level level, bool tmdPulses) {
    AcquireSRWLockExclusive(&lock);
    config.enabled = enabled;
    config.level = level;
    config.tmd = tmdPulses;
    if (!enabled || !tmdPulses) {
        engine.Clear();
        InterlockedExchange(&cachedAmpMilli, 0);
    }
    ReleaseSRWLockExclusive(&lock);
}

static void Note(void* self, void* tmd, unsigned raw, bool start) {
    if (!InterlockedCompareExchange(&hooksReady, 0, 0)) return;
    __try {
        char clsName[96]{};
        bool readable=false, live=false;
        uintptr_t cls=0;
        const auto classified=Classify((uintptr_t)self, clsName, sizeof(clsName),
                                        readable, live, cls);
        const bool local=IsLocal((uintptr_t)tmd);
        const auto power=tmdhaptics::SelectLocalPower(classified, readable, local);
        static unsigned identityReceipts=0;
        if(identityReceipts<24){
            ++identityReceipts;
            Log("[Haptics Trace] TMD CLASSIFY [%u/24]: event=%s object=%08X readable=%d live=%d classPtr=%08X class=%s tmd=%08X local=%d classified=%s decision=%s",
                identityReceipts,start?"start":"stop",(unsigned)(uintptr_t)self,
                readable?1:0,live?1:0,(unsigned)cls,clsName,(unsigned)(uintptr_t)tmd,
                local?1:0,Name(classified),Name(power));
        }
        if (power == tmdhaptics::Power::None) {
            return;
        }

        // Proven return semantics: ret == 0 is successful activation (energy & cooldown clear)
        const bool accepted = start && ((raw & 0xff) == 0);
        const char* reason = accepted ? "activated" :
            ((raw & 0xff) == 1 ? "not_ready_or_invalid_target" :
            ((raw & 0xff) == 2 ? "insufficient_energy" :
            ((raw & 0xff) == 3 ? "cooldown_active" : "unknown_fail")));

        bool armed = false;
        const auto now = GetTickCount64();
        AcquireSRWLockExclusive(&lock);
        if (start) {
            armed = engine.OnStart(power, accepted, config, now);
        } else {
            armed = engine.OnStop(power, config, now);
        }
        ReleaseSRWLockExclusive(&lock);

        static unsigned receipts = 0;
        if (receipts < 24) {
            ++receipts;
            Log("[Haptics Trace] TMD EVENT [%u/24]: %s power=%s (%s) ret=%u [%s] accepted=%d armed=%d",
                receipts, start ? "start" : "stop", Name(power), clsName, raw & 0xff, reason,
                accepted ? 1 : 0, armed ? 1 : 0);
        }
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        InterlockedExchange(&hooksReady, 0);
    }
}

static unsigned char __fastcall Start(void* self, void* edx, void* tmd) {
    const unsigned char r = originalStart(self, edx, tmd);
    Note(self, tmd, r, true);
    return r;
}

static int __fastcall Stop(void* self, void* edx, void* tmd) {
    const int r = originalStop(self, edx, tmd);
    Note(self, tmd, (unsigned)r, false);
    return r;
}

// Evaluated once per frame on the render thread from the single rumble site in d3d9.cpp.
static float Overlay(bool gatesOk) {
    if (!gatesOk) {
        ClearAll();
        return 0.0f;
    }
    AcquireSRWLockExclusive(&lock);
    const float amp = engine.Amplitude(config, gatesOk, GetTickCount64());
    InterlockedExchange(&cachedAmpMilli, (LONG)(amp * 1000.0f));
    ReleaseSRWLockExclusive(&lock);
    return amp;
}

static void UnhookBoth() {
    if (originalStart) {
        MH_DisableHook((void*)kStartAddr);
        MH_RemoveHook((void*)kStartAddr);
        originalStart = nullptr;
    }
    if (originalStop) {
        MH_DisableHook((void*)kStopAddr);
        MH_RemoveHook((void*)kStopAddr);
        originalStop = nullptr;
    }
    InterlockedExchange(&hooksReady, 0);
}

// Engine-thread lazy installer. Safe, fail-closed, no partial hook states.
static void Poll() {
    if (!config.tmd || InterlockedCompareExchange(&hooksReady, 0, 0)) return;
    if (InterlockedCompareExchange(&installAttempts, 0, 0) >= 3) return;

    static ULONGLONG retry = 0;
    const auto now = GetTickCount64();
    if (now < retry) return;
    retry = now + 3000;
    InterlockedIncrement(&installAttempts);

    if ((uintptr_t)GetModuleHandleW(nullptr) != kImageBase) {
        InterlockedExchange(&installAttempts, 3);
        Log("TMD haptics: image base moved; refused");
        return;
    }
    const bool sOk = NativeContextCursor::Method(kStartAddr, kStartSize, kStartHash);
    const bool tOk = NativeContextCursor::Method(kStopAddr, kStopSize, kStopHash);
    if (!sOk || !tOk) {
        InterlockedExchange(&installAttempts, 3);
        Log("TMD haptics: contract refused start=%d stop=%d; no hook", sOk ? 1 : 0, tOk ? 1 : 0);
        return;
    }

    if (MH_CreateHook((void*)kStartAddr, (void*)&Start, (void**)&originalStart) != MH_OK) {
        UnhookBoth();
        InterlockedExchange(&installAttempts, 3);
        Log("TMD haptics: MH_CreateHook failed for Start; permanently disabled");
        return;
    }
    if (MH_CreateHook((void*)kStopAddr, (void*)&Stop, (void**)&originalStop) != MH_OK) {
        UnhookBoth();
        InterlockedExchange(&installAttempts, 3);
        Log("TMD haptics: MH_CreateHook failed for Stop; unhooked both, permanently disabled");
        return;
    }

    const MH_STATUS enStart = MH_EnableHook((void*)kStartAddr);
    const MH_STATUS enStop  = MH_EnableHook((void*)kStopAddr);
    if (enStart != MH_OK || enStop != MH_OK) {
        UnhookBoth();
        InterlockedExchange(&installAttempts, 3);
        Log("TMD haptics: MH_EnableHook failed start=%d stop=%d; unhooked both, permanently disabled",
            (int)enStart, (int)enStop);
        return;
    }

    InterlockedExchange(&hooksReady, 1);
    Log("TMD haptics: verified native Start/StopFirePower observers installed");
}

} // namespace NativeTmdHaptics
