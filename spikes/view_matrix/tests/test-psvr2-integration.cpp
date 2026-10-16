#include "../psvr2_ipc.h"
#include "../psvr2_trigger.h"
#include "../psvr2_trigger_config.h"
#include "../psvr2_client.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

namespace {
using namespace psvr2trigger;

PolicyInput LiveGame(Weapon w) {
    PolicyInput in;
    in.backendAvailable = true;
    in.runtimeCompatible = true;
    in.sessionFocused = true;
    in.gameplayActive = true;
    in.weapon = w;
    in.ammoOk = true;
    return in;
}

void ipc_codec_tests() {
    using namespace psvr2ipc;
    const std::uint8_t token[] = {'a', 'b', 'c', '1', '2', '3'};
    std::uint8_t msg[kMaxMessageBytes];
    // Hello round-trip + session auth.
    std::size_t n = EncodeHello(msg, 7, 12345, token, sizeof(token));
    assert(n == kHeaderBytes + sizeof(token));
    Decoded d;
    assert(Decode(msg, n, d) == 0 && d.type == MsgType::Hello && d.sequence == 7 &&
           d.timeMs == 12345 && d.tokenLen == sizeof(token) &&
           std::memcmp(d.token, token, sizeof(token)) == 0);
    ServerSession session(token, sizeof(token));
    assert(session.Accept(msg, n, d) == 0 && session.Authed());
    assert(session.Accept(msg, n, d) == -105); // replay (same seq) rejected
    std::size_t h2 = EncodeHello(msg, 8, 12346, token, sizeof(token));
    assert(session.Accept(msg, h2, d) == -106); // re-hello rejected
    // Trigger round-trip with monotonic sequence.
    EffectDesc fx;
    fx.effect = 1;
    fx.hand = 0;
    fx.start = 4;
    fx.end = 6;
    fx.strength = 3;
    n = EncodeTrigger(msg, 8, 12400, fx);
    assert(n == kHeaderBytes + kTriggerBytes);
    assert(Decode(msg, n, d) == 0 && d.type == MsgType::TriggerState && d.sequence == 8);
    assert(d.fx == fx);
    assert(session.Accept(msg, n, d) == 0);
    assert(session.Accept(msg, n, d) == -105); // replay rejected
    // Shutdown accepted, then truncated/corrupt rejected.
    n = EncodeShutdown(msg, 9, 12500);
    assert(session.Accept(msg, n, d) == 0 && d.type == MsgType::Shutdown);
    assert(Decode(msg, n - 1, d) == -1); // truncated below header
    std::uint8_t longMsg[kMaxMessageBytes + 1];
    std::memcpy(longMsg, msg, n);
    longMsg[n] = 0;
    assert(Decode(longMsg, n + 1, d) == -6); // length/payload mismatch
    std::uint8_t bad[kMaxMessageBytes];
    std::memcpy(bad, msg, n);
    bad[0] ^= 0xFF;
    assert(Decode(bad, n, d) == -2); // bad magic
    bad[0] ^= 0xFF;
    bad[4] ^= 0xFF;
    assert(Decode(bad, n, d) == -3); // version mismatch (KHARVOX v1/v3 lesson)
    // Unauthed session: trigger-before-hello rejected; wrong token rejected.
    ServerSession fresh(token, sizeof(token));
    std::size_t m = EncodeTrigger(msg, 1, 1, fx);
    assert(fresh.Accept(msg, m, d) == -102);
    const std::uint8_t other[] = {'x', 'y', 'z'};
    m = EncodeHello(msg, 1, 1, other, sizeof(other));
    assert(fresh.Accept(msg, m, d) == -103);
    // Name/token validation.
    assert(PipeNameValid("\\\\.\\pipe\\SingularityPsvr2_1234_ab", 34));
    assert(!PipeNameValid("short", 5));
    assert(!PipeNameValid("\\\\.\\pipe\\evil;del", 17));
    assert(!TokenValid(nullptr, 0) && !TokenValid(token, 0));
    assert(TokenValid(token, sizeof(token)));
    std::puts("ipc: codec round-trip, auth-first, replay/version/magic rejection");
}

void config_tests() {
    using namespace psvr2config;
    Profiles p;
    const char* ini = "[PSVR2]\n"
                      "AdaptiveTriggers=1\n"
                      "TriggerPistol=5,7,4\n"
                      "TriggerRifleFire=2,6,50\n"
                      "TriggerShotgun=9,9,9\n" // invalid: rejected, default kept
                      "TriggerSniper=2,5,6\n";
    assert(ParseIni(ini, p) == 4);
    assert(p.enabled && p.pistol.a == 5 && p.pistol.b == 7 && p.pistol.c == 4);
    assert(p.rifleFire.a == 2 && p.rifleFire.b == 6 && p.rifleFire.c == 50);
    assert(p.shotgun.a == 2 && p.shotgun.b == 4 && p.shotgun.c == 7); // default kept
    // Unknown section ignored; disabled flag gates everything to Off.
    Profiles q;
    assert(ParseIni("[Other]\nTriggerPistol=2,3,1\n", q) == 0);
    assert(q.pistol.a == 4);
    assert(ParseIni("[PSVR2]\nTriggerPistol=8,8,4\n", q) == 1);
    assert(q.pistol.a == 8 && q.pistol.b == 8 && q.pistol.c == 4);
    assert(ParseIni("[PSVR2]\nTriggerPistol=2,8,0\n", q) == 1);
    assert(q.pistol.a == 2 && q.pistol.b == 8 && q.pistol.c == 0);
    assert(ParseIni("[PSVR2]\nTriggerPistol=1,2,8\n", q) == 1);
    assert(q.pistol.a == 1 && q.pistol.b == 2 && q.pistol.c == 8);
    assert(ParseIni("[PSVR2]\nAdaptiveTriggers=0\n", q) == 1 && !q.enabled);
    PolicyInput in = LiveGame(Weapon::Pistol);
    assert(SelectWithProfiles(in, q).effect == Effect::Off);
    // Config-driven selection matches built-in semantics on defaults.
    Profiles dflt;
    for (Weapon w : {Weapon::Pistol, Weapon::Rifle, Weapon::Shotgun, Weapon::Sniper,
                     Weapon::Tmd, Weapon::Other}) {
        for (bool fire : {false, true}) {
            PolicyInput i = LiveGame(w);
            i.fireDown = fire;
            Command a = Select(i), b = SelectWithProfiles(i, dflt);
            assert(Pack(a) == Pack(b));
        }
    }
    // Custom profile flows through: pistol triple + rifle fire override.
    Command c = ProfileFromConfig(Weapon::Pistol, false, p);
    assert(c.start == 5 && c.end == 7 && c.strength == 4);
    Command v = ProfileFromConfig(Weapon::Rifle, true, p);
    assert(v.effect == Effect::Vibration && v.position == 2 && v.amplitude == 6 &&
           v.frequency == 50);
    std::puts("config: parse, per-key fallback, defaults parity, custom flow");
}

struct RecordingBackend : IBackend {
    Command last;
    bool Send(const Command& c) override {
        last = c;
        return true;
    }
};

void pack_uniqueness_tests() {
    // Every distinct reference command (including Off reasons) must pack
    // distinctly, or generation-dedup would swallow real transitions
    // (e.g. Off(Menu) -> Off(SessionEnd) must forward).
    std::uint64_t seen[32];
    unsigned n = 0;
    auto note = [&](Command c) {
        const std::uint64_t p = Pack(c);
        for (unsigned i = 0; i < n; ++i) assert(seen[i] != p);
        assert(n < 32);
        seen[n++] = p;
    };
    note(ProfileFor(Weapon::Pistol, false));
    note(ProfileFor(Weapon::Rifle, false));
    note(ProfileFor(Weapon::Rifle, true));
    note(ProfileFor(Weapon::Shotgun, false));
    note(ProfileFor(Weapon::Sniper, false));
    note(OffCommand(OffReason::Disabled));
    note(OffCommand(OffReason::UnsupportedRuntime));
    note(OffCommand(OffReason::Unfocused));
    note(OffCommand(OffReason::Inactive));
    note(OffCommand(OffReason::SessionEnd));
    note(OffCommand(OffReason::UnsupportedWeapon));
    // Masking keeps the codec inside 43 bits for the full value domain.
    assert(Pack(ProfileFor(Weapon::Pistol, false)) < (1ull << 43));
    std::puts("pack: 11 reference commands unique, Off reasons distinct");
}

void runner_event_tests() {
    RecordingBackend backend;
    Runner runner(&backend);
    runner.SetGating(true, true, true, true);
    runner.OnWeaponChanged(Weapon::Pistol);
    assert(runner.Update() && backend.last.start == 4);
    runner.OnFirePressed();
    assert(runner.Update()); // pistol keeps wall while firing
    assert(backend.last.effect == Effect::Weapon);
    runner.OnWeaponChanged(Weapon::Rifle);
    assert(runner.Update());
    assert(backend.last.effect == Effect::Vibration);
    runner.OnReloadStarted();
    assert(runner.Update());
    assert(backend.last.effect == Effect::Weapon);
    runner.OnReloadFinished();
    runner.OnFireReleased();
    assert(runner.Update());
    assert(backend.last.effect == Effect::Weapon);
    assert(runner.ClearTriggerEffects());
    assert(backend.last.effect == Effect::Off);
    std::puts("runner: OnWeaponChanged/OnFirePressed/Released/OnReloadStarted/Finished/Clear");
}

void client_no_bridge_tests() {
    // No bridge running: Start validates, Submit never blocks, Stop joins.
    psvr2client::Client client;
    assert(!client.Start("short", (const std::uint8_t*)"tok", 3)); // bad name
    const std::uint8_t token[] = {'t', 'o', 'k', 'e', 'n'};
    char pipe[128];
    std::snprintf(pipe, sizeof(pipe), "\\\\.\\pipe\\SingularityPsvr2_Test_%lu",
                  (unsigned long)GetCurrentProcessId());
    assert(client.Start(pipe, token, sizeof(token)));
    Command cmd = ProfileFor(Weapon::Shotgun, false);
    for (int i = 0; i < 10; ++i) client.Submit(cmd); // dedup on worker; never blocks
    client.Stop();
    psvr2client::Stats s = client.GetStats();
    assert(s.sends == 0); // nothing sent: no bridge, no Hello completed
    client.Stop(); // double-stop safe
    std::puts("client: validation, non-blocking submit, clean stop with no bridge");
}
} // namespace

int main() {
    ipc_codec_tests();
    config_tests();
    pack_uniqueness_tests();
    runner_event_tests();
    client_no_bridge_tests();
    std::puts("PASS");
    return 0;
}
