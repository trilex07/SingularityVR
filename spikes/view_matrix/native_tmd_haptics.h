#pragma once
#include "tmd_haptics.h"
// Native TMD power event source for tmd_haptics.h. Observation only: both detours call the
// original FIRST, forward its result unchanged, and only then record an event. No UObject,
// energy, inventory, input or camera writes.
//
// Evidence (owned original Singularity.exe + archived .text, offline disassembly):
//  * Script exec wrappers URvTMDPower::execStartFirePower (0xF5D370, 96 B, FNV 7B8C05DBDEEC95C5)
//    and execStopFirePower (0xF5D3D0, 96 B, FNV D5110E0A85330314) each read one object argument
//    and make a direct thiscall: Start -> 0x100D790(this=power, RvTMD*), Stop -> 0x100AEC0.
//  * 0x100D790 (248 B, ret 4, FNV 894F69CF191C20C7): ebx = power->vtable[+0x11C](tmd,0).
//    If ebx != 0 the whole body is skipped (jne 0x100D87F) and `al = bl` is returned. Only when
//    ebx == 0 does it run the activation body (state call 0x10105C0, vtable+0x4BC, 0x100D530,
//    vtable+0x124/+0x138 FX/hooks, owner notifications). So "native activation ran" <=> ret == 0.
//    (INFERRED polarity from control flow; the bounded receipts below record the raw return so
//    MAIN can confirm or flip kStartAcceptedWhenZero from a real run.)
//  * 0x100AEC0 (194 B, ret 4, FNV 1563D1E2018602DE) is the matching stop/cleanup path.
// Both bodies are fingerprint-verified in the loaded image before any detour is created; any
// mismatch, non-0x400000 image or hook failure leaves the game untouched.
namespace NativeTmdHaptics {
using StartFn = unsigned char (__fastcall*)(void*, void*, void*);
using StopFn = int (__fastcall*)(void*, void*, void*);
static StartFn originalStart = nullptr;
static StopFn originalStop = nullptr;
constexpr uintptr_t kImageBase = 0x400000;
constexpr uintptr_t kStartAddr = 0x100D790, kStopAddr = 0x100AEC0;
constexpr size_t kStartSize = 248, kStopSize = 194;
constexpr unsigned long long kStartHash = 0x894F69CF191C20C7ull, kStopHash = 0x1563D1E2018602DEull;
constexpr bool kStartAcceptedWhenZero = true;

static tmdhaptics::Config config;
static tmdhaptics::Engine engine;
static SRWLOCK lock = SRWLOCK_INIT;
static volatile LONG hooksReady = 0;
static float cachedAmp = 0.0f;

static tmdhaptics::Power Classify(uintptr_t power) {
    using namespace NativeOwnedInventory;
    using tmdhaptics::Power;
    static const struct { const char* name; Power p; } kMap[] = {
        { "RvTMDPower_Impulse", Power::Impulse }, { "RvTMDPower_Age", Power::Age },
        { "RvTMDPower_AntiGrav", Power::AntiGrav }, { "RvTMDPower_Deadlock", Power::Deadlock },
        { "RvTMDPower_Heal", Power::Heal }, { "RvTMDPower_Shield", Power::Shield },
        { "RvTMDPower_Teleport", Power::Teleport } };
    if (!power || !IsLive(power)) return Power::None;
    const auto cls = P(power + OBJ_CLASS);
    for (const auto& e : kMap) if (NativeTmdAim::ExactClass(cls, e.name)) return e.p;
    return Power::None;
}
static const char* Name(tmdhaptics::Power p) {
    static const char* n[] = { "None", "Impulse", "Age", "AntiGrav", "Deadlock", "Heal", "Shield", "Teleport" };
    return n[(unsigned)p < 8 ? (unsigned)p : 0];
}
static bool IsLocal(uintptr_t tmd) {
    uintptr_t pawn = 0;
    return tmd && NativeTmdAim::LocalTmd(pawn) == tmd;
}
static void Note(void* self, void* tmd, unsigned raw, bool start) {
    if (!InterlockedCompareExchange(&hooksReady, 0, 0)) return;
    __try {
        const auto power = Classify((uintptr_t)self);
        if (power == tmdhaptics::Power::None || !IsLocal((uintptr_t)tmd)) return;  // AI / unknown: ignore
        const bool accepted = start && ((raw & 0xff) == 0) == kStartAcceptedWhenZero;
        bool armed = false;
        const auto now = GetTickCount64();
        if (TryAcquireSRWLockExclusive(&lock)) {   // never block the engine thread; drop on contention
            armed = start ? engine.OnStart(power, accepted, config, now) : engine.OnStop(power, config, now);
            ReleaseSRWLockExclusive(&lock);
        }
        static unsigned receipts = 0;
        if (receipts < 24) {
            ++receipts;
            Log("TMD haptics: %s power=%s ret=%u accepted=%d armed=%d [receipt %u/24]",
                start ? "start" : "stop", Name(power), raw & 0xff, accepted ? 1 : 0, armed ? 1 : 0, receipts);
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
static void Configure(bool enabled, tmdhaptics::Level level, bool tmdPulses) {
    config.enabled = enabled; config.level = level; config.tmd = tmdPulses;
}
// Once per frame from the single rumble site. Non-blocking; stale value on contention.
static float Overlay(bool gatesOk) {
    if (!config.tmd) return 0.0f;
    if (TryAcquireSRWLockExclusive(&lock)) {
        cachedAmp = engine.Amplitude(config, gatesOk, GetTickCount64());
        ReleaseSRWLockExclusive(&lock);
    } else if (!gatesOk) cachedAmp = 0.0f;
    return cachedAmp;
}
static void ClearAll() {
    if (TryAcquireSRWLockExclusive(&lock)) { engine.Clear(); cachedAmp = 0.0f; ReleaseSRWLockExclusive(&lock); }
}
// Engine-thread poll (native_ui_input.h). Installs once; every failure is silent no-op.
static void Poll() {
    if (!config.tmd || InterlockedCompareExchange(&hooksReady, 0, 0)) return;
    static ULONGLONG retry = 0; static unsigned attempts = 0;
    const auto now = GetTickCount64();
    if (now < retry || attempts >= 3) return;
    retry = now + 3000; ++attempts;
    if ((uintptr_t)GetModuleHandleW(nullptr) != kImageBase) { attempts = 3; Log("TMD haptics: image base moved; refused"); return; }
    const bool sOk = NativeContextCursor::Method(kStartAddr, kStartSize, kStartHash);
    const bool tOk = NativeContextCursor::Method(kStopAddr, kStopSize, kStopHash);
    if (!sOk || !tOk) { attempts = 3; Log("TMD haptics: contract refused start=%d stop=%d; no hook", sOk ? 1 : 0, tOk ? 1 : 0); return; }
    if (MH_CreateHook((void*)kStartAddr, (void*)&Start, (void**)&originalStart) != MH_OK) return;
    if (MH_CreateHook((void*)kStopAddr, (void*)&Stop, (void**)&originalStop) != MH_OK) {
        MH_RemoveHook((void*)kStartAddr); originalStart = nullptr; return;
    }
    if (MH_EnableHook((void*)kStartAddr) != MH_OK || MH_EnableHook((void*)kStopAddr) != MH_OK) {
        Log("TMD haptics: enable failed; trampolines retained, events inactive"); return;
    }
    InterlockedExchange(&hooksReady, 1);
    Log("TMD haptics: verified native Start/StopFirePower observers installed [runtime acceptance pending]");
}
}  // namespace NativeTmdHaptics
