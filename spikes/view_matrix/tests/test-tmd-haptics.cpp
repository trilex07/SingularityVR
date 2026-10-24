// Offline tests for TMD haptics policy, concurrency, gating, and hook safety.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <thread>
#include <vector>
#include <atomic>
#include "../tmd_haptics.h"

using namespace tmdhaptics;

static Config On() { Config c; c.enabled = true; c.tmd = true; c.level = Level::Normal; return c; }
static bool Near(float a, float b) { return a > b - 1e-5f && a < b + 1e-5f; }

// --- Hook safety mock ---
namespace MockHook {
    static int createCalls = 0;
    static int enableCalls = 0;
    static int removeCalls = 0;
    static int disableCalls = 0;
    static int failCreateOn = 0; // 1 = fail first, 2 = fail second
    static int failEnableOn = 0; // 1 = fail first, 2 = fail second

    enum Status { OK = 0, ERR = 1 };
    static Status CreateHook(void*, void*, void** outOrig, int id) {
        ++createCalls;
        if (failCreateOn == id) return ERR;
        *outOrig = (void*)0x1234;
        return OK;
    }
    static Status EnableHook(void*, int id) {
        ++enableCalls;
        if (failEnableOn == id) return ERR;
        return OK;
    }
    static Status DisableHook(void*) { ++disableCalls; return OK; }
    static Status RemoveHook(void*) { ++removeCalls; return OK; }

    static void Reset() {
        createCalls = enableCalls = removeCalls = disableCalls = 0;
        failCreateOn = failEnableOn = 0;
    }
}

// Simulated installer matching NativeTmdHaptics::Poll() error handling logic
struct MockInstaller {
    void* origStart = nullptr;
    void* origStop = nullptr;
    int hooksReady = 0;
    int installAttempts = 0;

    void UnhookBoth() {
        if (origStart) {
            MockHook::DisableHook((void*)1);
            MockHook::RemoveHook((void*)1);
            origStart = nullptr;
        }
        if (origStop) {
            MockHook::DisableHook((void*)2);
            MockHook::RemoveHook((void*)2);
            origStop = nullptr;
        }
        hooksReady = 0;
    }

    bool Poll() {
        if (hooksReady || installAttempts >= 3) return false;
        ++installAttempts;

        if (MockHook::CreateHook((void*)1, nullptr, &origStart, 1) != MockHook::OK) {
            UnhookBoth();
            installAttempts = 3;
            return false;
        }
        if (MockHook::CreateHook((void*)2, nullptr, &origStop, 2) != MockHook::OK) {
            UnhookBoth();
            installAttempts = 3;
            return false;
        }

        const auto enStart = MockHook::EnableHook((void*)1, 1);
        const auto enStop  = MockHook::EnableHook((void*)2, 2);
        if (enStart != MockHook::OK || enStop != MockHook::OK) {
            UnhookBoth();
            installAttempts = 3;
            return false;
        }
        hooksReady = 1;
        return true;
    }
};

int main() {
    // Session-B correction: direct class identity and local-only Generic path.
    assert(ClassifyExactName("RvTMDPower_Impulse") == Power::Impulse);
    assert(ClassifyExactName("RvTMDPower_Age") == Power::Age);
    assert(ClassifyExactName("RvTMDPower_AntiGrav") == Power::AntiGrav);
    assert(ClassifyExactName("RvTMDPower_Deadlock") == Power::Deadlock);
    assert(ClassifyExactName("RvTMDPower_Heal") == Power::Heal);
    assert(ClassifyExactName("RvTMDPower_Shield") == Power::Shield);
    assert(ClassifyExactName("RvTMDPower_Teleport") == Power::Teleport);
    assert(ClassifyExactName("UnknownPower") == Power::None);
    assert(ClassifyExactName(nullptr) == Power::None);
    assert(SelectLocalPower(Power::Impulse,true,true) == Power::Impulse);
    assert(SelectLocalPower(Power::None,true,true) == Power::Generic);
    assert(SelectLocalPower(Power::None,true,false) == Power::None); // foreign TMD
    assert(SelectLocalPower(Power::Impulse,true,false) == Power::None);
    assert(SelectLocalPower(Power::None,false,true) == Power::None); // null/invalid receiver
    assert(SelectLocalPower(Power::Impulse,false,true) == Power::None);
    {
        Engine e;Config c=On();
        assert(!e.OnStart(Power::Generic,false,c,900)); // native activation rejected
        assert(e.OnStart(SelectLocalPower(Power::None,true,true),true,c,1000));
        assert(Near(e.Amplitude(c,true,1001),0.40f));
        assert(!e.OnStop(Power::Generic,c,1002));
        assert(Near(e.Amplitude(c,true,1070),0.0f));
        assert(e.OnStart(Power::Generic,true,c,1100));
        assert(Near(e.Amplitude(c,false,1101),0.0f)); // pause/menu/unfocused clears
        assert(Near(e.Amplitude(c,true,1102),0.0f));  // no delayed replay
        c.tmd=false;
        assert(!e.OnStart(Power::Generic,true,c,2000));
        assert(Near(e.Amplitude(c,true,2001),0.0f));
    }
    // 1. Rejected start (ret != 0: energy, cooldown, or invalid target) never pulses.
    {
        Engine e;
        assert(!e.OnStart(Power::Impulse, false, On(), 1000));
        assert(Near(e.Amplitude(On(), true, 1001), 0.0f));
        assert(!e.IsPulsing(1001));
    }

    // 2. Impulse: strong short pulse (90 ms), clean silence after expiry; no sustain.
    {
        Engine e;
        assert(e.OnStart(Power::Impulse, true, On(), 1000));
        assert(Near(e.Amplitude(On(), true, 1050), 0.80f));
        assert(e.IsPulsing(1050));
        assert(Near(e.Amplitude(On(), true, 1090), 0.0f));
        assert(!e.IsPulsing(1090));
    }

    // 3. Age: distinct medium pulse (90 ms), no sustain.
    {
        Engine e;
        assert(e.OnStart(Power::Age, true, On(), 2000));
        assert(Near(e.Amplitude(On(), true, 2040), 0.50f));
        assert(Near(e.Amplitude(On(), true, 2090), 0.0f));
        // StopFirePower has no effect since speculative sustain is removed
        assert(!e.OnStop(Power::Age, On(), 2100));
        assert(Near(e.Amplitude(On(), true, 2110), 0.0f));
    }

    // 4. De-duplication: duplicate native start within 30 ms ignored.
    {
        Engine e;
        assert(e.OnStart(Power::Impulse, true, On(), 5000));
        assert(!e.OnStart(Power::Impulse, true, On(), 5015)); // too fast, dropped
        assert(e.OnStart(Power::Impulse, true, On(), 5100));  // 100 ms later: accepted
    }

    // 5. Rapid successive powers: stronger pulse still playing is preserved.
    {
        Engine e;
        e.OnStart(Power::Impulse, true, On(), 6000);  // 0.80, 90 ms (ends 6090)
        e.OnStart(Power::AntiGrav, true, On(), 6020); // 0.40, 70 ms (ends 6090)
        assert(Near(e.Amplitude(On(), true, 6030), 0.80f));
        assert(Near(e.Amplitude(On(), true, 6090), 0.0f));
    }

    // 6. Failed gate (tracking loss, pause, cinematic, menu) guarantees immediate clear.
    {
        Engine e;
        e.OnStart(Power::Deadlock, true, On(), 8000);
        assert(e.Amplitude(On(), true, 8010) > 0.0f);
        // Tracking loss or pause occurs:
        assert(Near(e.Amplitude(On(), false, 8020), 0.0f));
        // When tracking returns later, the pulse is permanently gone, not leaked:
        assert(Near(e.Amplitude(On(), true, 8030), 0.0f));
    }

    // 7. Session restart / unfocus clear:
    {
        Engine e;
        e.OnStart(Power::Teleport, true, On(), 9000);
        assert(e.IsPulsing(9020));
        e.Clear();
        assert(!e.IsPulsing(9020));
        assert(Near(e.Amplitude(On(), true, 9020), 0.0f));
    }

    // 8. Level scaling: Normal is exact identity; Low is 0.5x; Strong is 1.3x.
    {
        Config c = On();
        assert(Near(Merge(0.35f, 0.0f, true, c), 0.35f));  // native rumble on TMD hand
        assert(Near(Merge(0.35f, 0.0f, false, c), 0.35f)); // native rumble on gun hand untouched
        assert(Near(Merge(0.10f, 0.80f, true, c), 0.80f)); // TMD overlay merged
        assert(Near(Merge(0.10f, 0.80f, false, c), 0.10f));// gun hand never takes TMD overlay
        assert(Near(Merge(0.90f, 0.80f, true, c), 0.90f)); // max(native, overlay)

        c.level = Level::Low;
        assert(Near(Merge(0.80f, 0.0f, false, c), 0.40f));
        c.level = Level::Strong;
        assert(Near(Merge(0.50f, 0.0f, false, c), 0.65f));
        c.enabled = false;
        assert(Near(Merge(0.80f, 0.80f, true, c), 0.0f));
    }

    // 9. All 8 profiles verified: all short (<= 100 ms), bounded (<= 0.80), no sustain.
    for (int i = 1; i <= 8; ++i) {
        const Profile p = ProfileFor((Power)i);
        assert(p.startAmp > 0.0f && p.startAmp <= 0.80f);
        assert(p.startMs > 0 && p.startMs <= 100);
    }
    {
        const Profile p = ProfileFor(Power::None);
        assert(p.startAmp == 0.0f && p.startMs == 0);
    }

    // 10. Multi-threaded concurrency: concurrent settings changes, starts, and clears.
    {
        Engine sharedEngine;
        Config sharedConfig = On();
        SRWLOCK lock = SRWLOCK_INIT;
        std::atomic<bool> stopFlag{false};

        // Thread 1: fires powers repeatedly
        std::thread tFire([&]() {
            uint64_t t = 1000;
            while (!stopFlag.load(std::memory_order_relaxed)) {
                AcquireSRWLockExclusive(&lock);
                sharedEngine.OnStart((Power)((t % 7) + 1), true, sharedConfig, t);
                ReleaseSRWLockExclusive(&lock);
                t += 10;
                Sleep(1);
            }
        });

        // Thread 2: modifies configuration (levels, enable/disable)
        std::thread tConfig([&]() {
            int step = 0;
            while (!stopFlag.load(std::memory_order_relaxed)) {
                AcquireSRWLockExclusive(&lock);
                sharedConfig.level = (Level)(step % 3);
                sharedConfig.enabled = (step % 5) != 0;
                sharedConfig.tmd = true;
                ReleaseSRWLockExclusive(&lock);
                ++step;
                Sleep(2);
            }
        });

        // Thread 3: samples overlay and clears on random "unfocus/pause" events
        std::thread tSample([&]() {
            uint64_t t = 1000;
            while (!stopFlag.load(std::memory_order_relaxed)) {
                AcquireSRWLockExclusive(&lock);
                const bool gatesOk = (t % 13) != 0;
                const float a = sharedEngine.Amplitude(sharedConfig, gatesOk, t);
                assert(a >= 0.0f && a <= 1.0f);
                if (!gatesOk) sharedEngine.Clear();
                ReleaseSRWLockExclusive(&lock);
                t += 5;
                Sleep(1);
            }
        });

        Sleep(50);
        stopFlag.store(true, std::memory_order_relaxed);
        tFire.join();
        tConfig.join();
        tSample.join();
    }

    // 11. Hook safety test A: first create fails -> clean, no leaks, permanently disabled.
    {
        MockHook::Reset();
        MockHook::failCreateOn = 1;
        MockInstaller inst;
        assert(!inst.Poll());
        assert(inst.installAttempts == 3);
        assert(inst.origStart == nullptr && inst.origStop == nullptr);
        assert(inst.hooksReady == 0);
        // Subsequent poll is an immediate no-op:
        assert(!inst.Poll());
        assert(MockHook::createCalls == 1);
    }

    // 12. Hook safety test B: second create fails -> first is removed, trampolines nulled.
    {
        MockHook::Reset();
        MockHook::failCreateOn = 2;
        MockInstaller inst;
        assert(!inst.Poll());
        assert(inst.installAttempts == 3);
        assert(inst.origStart == nullptr && inst.origStop == nullptr);
        assert(inst.hooksReady == 0);
        assert(MockHook::removeCalls == 1); // first hook was removed
    }

    // 13. Hook safety test C: enable fails on second hook -> both disabled & removed, no half-installed hook.
    {
        MockHook::Reset();
        MockHook::failEnableOn = 2;
        MockInstaller inst;
        assert(!inst.Poll());
        assert(inst.installAttempts == 3);
        assert(inst.origStart == nullptr && inst.origStop == nullptr);
        assert(inst.hooksReady == 0);
        assert(MockHook::disableCalls == 2);
        assert(MockHook::removeCalls == 2);
    }

    // 14. Hook safety test D: both create and enable succeed -> ready == 1.
    {
        MockHook::Reset();
        MockInstaller inst;
        assert(inst.Poll());
        assert(inst.hooksReady == 1);
        assert(inst.origStart != nullptr && inst.origStop != nullptr);
        // UnhookBoth cleans up completely:
        inst.UnhookBoth();
        assert(inst.hooksReady == 0);
        assert(inst.origStart == nullptr && inst.origStop == nullptr);
        assert(MockHook::disableCalls == 2);
        assert(MockHook::removeCalls == 2);
    }

    std::puts("test-tmd-haptics: classification plus 14 baseline groups PASS");
    return 0;
}
