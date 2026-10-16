#include "../../spikes/view_matrix/psvr2_client.h"
#include "../../spikes/view_matrix/psvr2_trigger_config.h"
#include <cstdio>
#include <cstring>

int main() {
    static_assert(sizeof(void*) == 4, "smoke client must be compiled as x86 to match Singularity.exe");
    const char token[] = "SingularitySmokeToken_20261009_A7F2";
    if (!WaitNamedPipeW(L"\\\\.\\pipe\\SingularityPsvr2", 5000)) {
        std::printf("FAIL: WaitNamedPipeW error=%lu\n", GetLastError());
        return 3;
    }
    psvr2client::Client client;
    if (!client.Start("\\\\.\\pipe\\SingularityPsvr2",
                      reinterpret_cast<const std::uint8_t*>(token), std::strlen(token))) {
        std::puts("FAIL: client start rejected pipe/token");
        return 1;
    }

    const psvr2config::Profiles cfg{};
    using psvr2trigger::Weapon;

    auto waitSends = [&](unsigned target) {
        const DWORD deadline = GetTickCount() + 3000;
        while (client.GetStats().sends < target && GetTickCount() < deadline) {
            Sleep(25);
        }
    };

    // 1. TriggerPistol (4,6,3) -> sends: Hello (1) + Pistol (2)
    client.Submit(psvr2config::ProfileFromConfig(Weapon::Pistol, false, cfg));
    waitSends(2);

    // 2. TriggerShotgun (2,4,7) -> sends: 3
    client.Submit(psvr2config::ProfileFromConfig(Weapon::Shotgun, false, cfg));
    waitSends(3);

    // 3. TriggerRifle aim (3,5,5) -> sends: 4
    client.Submit(psvr2config::ProfileFromConfig(Weapon::Rifle, false, cfg));
    waitSends(4);

    // 4. TriggerRifleFire (3,7,40) -> sends: 5
    client.Submit(psvr2config::ProfileFromConfig(Weapon::Rifle, true, cfg));
    waitSends(5);

    // 5. TriggerSniper (2,5,6) -> sends: 6
    client.Submit(psvr2config::ProfileFromConfig(Weapon::Sniper, false, cfg));
    waitSends(6);

    client.Stop();
    const psvr2client::Stats stats = client.GetStats();
    std::printf("pipe smoke: connects=%u sends=%u errors=%u\n", stats.connects, stats.sends, stats.errors);
    return stats.connects > 0 && stats.sends >= 6 && stats.errors == 0 ? 0 : 2;
}
