// PSVR2 adaptive-trigger bridge probe (standalone, x86-safe).
//
// Goal (research brief section 26): verify the integration path WITHOUT the
// game running and WITHOUT ever actuating hardware.
//
// What this probe does:
//   1. Exercises psvr2_trigger.h profile selection for every Singularity
//      weapon and prints the exact command bytes that WOULD be sent.
//   2. Passively checks whether an x64 bridge pipe is present via
//      WaitNamedPipeW (existence check only -- never opened, never written).
//
// What it NEVER does:
//   - LoadLibrary any PSVR2Toolkit DLL (x64-only; this probe may itself be
//     built x86 to prove the game-side seam stays bitness-agnostic).
//   - Send any TriggerState message, apply any effect, hold any resistance.
//
// Build (x64 or x86):
//   cl /nologo /EHsc /W4 /std:c++17 psvr2_trigger_probe.cpp
//
// Exit code 0 = logic self-check passed. Pipe presence is informational only.

#include "../../spikes/view_matrix/psvr2_trigger.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>

namespace {
const char* kEffectName(psvr2trigger::Effect e) {
    switch (e) {
    case psvr2trigger::Effect::Off: return "Off";
    case psvr2trigger::Effect::Weapon: return "Weapon";
    case psvr2trigger::Effect::Vibration: return "Vibration";
    case psvr2trigger::Effect::Feedback: return "Feedback";
    case psvr2trigger::Effect::SlopeFeedback: return "SlopeFeedback";
    }
    return "?";
}

psvr2trigger::PolicyInput LiveGame(psvr2trigger::Weapon w) {
    psvr2trigger::PolicyInput in;
    in.backendAvailable = true;
    in.runtimeCompatible = true;
    in.sessionFocused = true;
    in.gameplayActive = true;
    in.weapon = w;
    in.ammoOk = true;
    return in;
}

bool CheckProfiles() {
    using namespace psvr2trigger;
    struct Expectation {
        Weapon weapon;
        Effect effect;
    };
    const Expectation kCases[] = {
        {Weapon::Pistol, Effect::Weapon}, {Weapon::Rifle, Effect::Weapon},
        {Weapon::Shotgun, Effect::Weapon}, {Weapon::Sniper, Effect::Weapon},
        {Weapon::Tmd, Effect::Off}, {Weapon::Other, Effect::Off},
    };
    bool ok = true;
    for (const auto& c : kCases) {
        Command cmd = Select(LiveGame(c.weapon));
        const std::uint64_t packed = Pack(cmd);
        std::printf("weapon=%d effect=%s start=%u end=%u str=%u pos=%u amp=%u freq=%u packed=0x%llx\n",
                    static_cast<int>(c.weapon), kEffectName(cmd.effect), cmd.start, cmd.end, cmd.strength,
                    cmd.position, cmd.amplitude, cmd.frequency, (unsigned long long)packed);
        if (cmd.effect != c.effect) {
            std::printf("MISMATCH weapon=%d\n", static_cast<int>(c.weapon));
            ok = false;
        }
        if (cmd.hand != Hand::Right) {
            std::puts("MISMATCH: expected RIGHT Sense trigger");
            ok = false;
        }
    }
    return ok;
}

// Fully passive: WaitNamedPipe with zero timeout only tests existence.
// No handle is opened, no byte is written, no effect is applied.
void ReportBridgePresence() {
    const wchar_t* pipe = L"\\\\.\\pipe\\SingularityPsvr2Bridge";
    if (WaitNamedPipeW(pipe, 0))
        std::puts("bridge: pipe present (no connection opened, no effect sent)");
    else if (GetLastError() == ERROR_FILE_NOT_FOUND)
        std::puts("bridge: absent (expected without the x64 helper running)");
    else
        std::puts("bridge: busy or unavailable (left untouched)");
}
} // namespace

int main() {
    std::puts("psvr2_trigger_probe: logic-only, no hardware actuation");
    if (!CheckProfiles()) {
        std::puts("FAIL");
        return 1;
    }
    ReportBridgePresence();
    std::puts("PASS (no trigger effect was sent)");
    return 0;
}
