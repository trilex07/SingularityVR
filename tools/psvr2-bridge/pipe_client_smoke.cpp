#include "../../spikes/view_matrix/psvr2_client.h"
#include <cstdio>
#include <cstring>

int main() {
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
    psvr2trigger::Command command;
    command.effect = psvr2trigger::Effect::Weapon;
    command.hand = psvr2trigger::Hand::Right;
    command.start = 4; command.end = 6; command.strength = 3;
    client.Submit(command);
    const DWORD deadline = GetTickCount() + 5000;
    psvr2client::Stats stats{};
    do {
        Sleep(50);
        stats = client.GetStats();
    } while (stats.sends < 2 && GetTickCount() < deadline);
    client.Stop();
    stats = client.GetStats();
    std::printf("pipe smoke: connects=%u sends=%u errors=%u\n", stats.connects, stats.sends, stats.errors);
    return stats.connects > 0 && stats.sends >= 2 ? 0 : 2;
}
