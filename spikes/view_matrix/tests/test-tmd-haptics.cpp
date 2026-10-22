// Offline behaviour tests for tmd_haptics.h (pure policy; no OpenXR, no game).
#include <cassert>
#include <cstdio>
#include "../tmd_haptics.h"
using namespace tmdhaptics;

static Config On() { Config c; c.enabled = true; c.tmd = true; c.level = Level::Normal; return c; }
static bool Near(float a, float b) { return a > b - 1e-5f && a < b + 1e-5f; }

int main() {
    // 1. A start the native path did not accept (blocked / no energy) never pulses.
    { Engine e; assert(!e.OnStart(Power::Impulse, false, On(), 1000)); assert(Near(e.Amplitude(On(), true, 1001), 0)); }
    // 2. Impulse: strong, short, then silence; no sustain.
    { Engine e; assert(e.OnStart(Power::Impulse, true, On(), 1000));
      assert(Near(e.Amplitude(On(), true, 1050), 0.80f));
      assert(Near(e.Amplitude(On(), true, 1090), 0.0f)); assert(e.Active() == Power::None); }
    // 3. Age: medium pulse, subtle sustain only while active, brief finish, then silence.
    { Engine e; assert(e.OnStart(Power::Age, true, On(), 2000));
      assert(Near(e.Amplitude(On(), true, 2100), 0.50f));
      assert(Near(e.Amplitude(On(), true, 2200), 0.08f));
      assert(e.OnStop(Power::Age, On(), 3000));
      assert(Near(e.Amplitude(On(), true, 3010), 0.25f));
      assert(Near(e.Amplitude(On(), true, 3050), 0.0f)); }
    // 4. One native event never pulses twice (duplicate within 30 ms).
    { Engine e; assert(e.OnStart(Power::Impulse, true, On(), 5000)); assert(!e.OnStart(Power::Impulse, true, On(), 5010));
      assert(e.OnStart(Power::Impulse, true, On(), 5100)); }
    // 5. Rapid successive powers: stronger pulse still playing is not truncated by a weaker start.
    { Engine e; e.OnStart(Power::Impulse, true, On(), 6000); e.OnStart(Power::AntiGrav, true, On(), 6020);
      assert(Near(e.Amplitude(On(), true, 6030), 0.80f));
      assert(Near(e.Amplitude(On(), true, 6100), 0.10f)); } // pulse over, AntiGrav sustain remains
    // 6. Stop without a prior accepted start is not a completion pulse.
    { Engine e; assert(!e.OnStop(Power::Age, On(), 7000)); assert(Near(e.Amplitude(On(), true, 7001), 0)); }
    { Engine e; e.OnStart(Power::Age, true, On(), 7000); assert(!e.OnStop(Power::Deadlock, On(), 7500)); }
    // 7. Any failed gate (pause/cinematic/menu/focus/tracking) clears everything permanently.
    { Engine e; e.OnStart(Power::Deadlock, true, On(), 8000);
      assert(e.Amplitude(On(), true, 8010) > 0); assert(Near(e.Amplitude(On(), false, 8020), 0));
      assert(Near(e.Amplitude(On(), true, 8030), 0)); assert(e.Active() == Power::None); }
    // 8. Disabled master/TMD switch: no events, no amplitude.
    { Config off = On(); off.tmd = false; Engine e; assert(!e.OnStart(Power::Impulse, true, off, 1)); assert(Near(e.Amplitude(off, true, 2), 0));
      off = On(); off.enabled = false; assert(!e.OnStart(Power::Impulse, true, off, 1)); }
    // 9. Sustain watchdog: a missed Stop can never stick.
    { Engine e; e.OnStart(Power::Shield, true, On(), 0);
      assert(e.Amplitude(On(), true, 5000) > 0); assert(Near(e.Amplitude(On(), true, kSustainCapMs + 1), 0)); }
    // 10. Hand targeting and identity: native rumble untouched at NORMAL; overlay only on TMD hand.
    { Config c = On();
      assert(Near(Merge(0.37f, 0.0f, true, c), 0.37f)); assert(Near(Merge(0.37f, 0.0f, false, c), 0.37f));
      assert(Near(Merge(0.10f, 0.80f, true, c), 0.80f)); assert(Near(Merge(0.10f, 0.80f, false, c), 0.10f));
      assert(Near(Merge(0.90f, 0.80f, true, c), 0.90f));       // never below native
      assert(Near(Merge(1.0f, 0.0f, false, c), 1.0f)); assert(Near(Merge(0.0f, 0.0f, true, c), 0.0f)); }
    // 11. Intensity levels and master switch (explicit user settings only).
    { Config c = On(); c.level = Level::Low; assert(Near(Merge(0.8f, 0.0f, false, c), 0.4f));
      c.level = Level::Strong; assert(Near(Merge(0.5f, 0.0f, false, c), 0.65f)); assert(Near(Merge(0.9f, 0.0f, false, c), 1.0f));
      c.enabled = false; assert(Near(Merge(0.9f, 0.9f, true, c), 0.0f)); }
    // 12. Conservative bounds on every profile; None is silent.
    for (int i = 1; i <= 7; ++i) { const Profile p = ProfileFor((Power)i);
      assert(p.startAmp <= 0.85f && p.startMs <= 120 && p.sustainAmp <= 0.12f && p.endAmp <= 0.30f && p.endMs <= 60);
      assert(p.startAmp > 0 && p.startMs > 0); }
    { const Profile n = ProfileFor(Power::None); assert(n.startAmp == 0 && n.sustainAmp == 0); }
    // 13. Pulse timing budget: a 90 ms pulse expires at the very next frame at 90 Hz (11 ms).
    { Engine e; e.OnStart(Power::Impulse, true, On(), 100); int hi = 0;
      for (std::uint64_t t = 100; t < 400; t += 11) if (e.Amplitude(On(), true, t) > 0) ++hi;
      assert(hi >= 8 && hi <= 9); assert(Near(e.Amplitude(On(), true, 400), 0)); }
    std::puts("tmd_haptics: all behaviour tests passed");
    return 0;
}
