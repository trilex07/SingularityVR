// Loopback sender: drives psvr2_client.h against a running bridge.
// Usage: loopback_sender.exe --pipe <name> --token <text>
// Exit 0 iff at least one command was delivered (bridge acknowledged by
// receiving; presence of stats.sends > 0 proves the full path works).
// Build x86 to mirror production bitness split (x86 client -> x64 bridge).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "../../../spikes/view_matrix/psvr2_client.h"
#include "../../../spikes/view_matrix/psvr2_trigger.h"

int main(int argc, char** argv) {
    const char* pipe = nullptr;
    const char* token = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--pipe") == 0 && i + 1 < argc) pipe = argv[++i];
        if (std::strcmp(argv[i], "--token") == 0 && i + 1 < argc) token = argv[++i];
    }
    if (!pipe || !token) {
        std::puts("usage: loopback_sender.exe --pipe <name> --token <text>");
        return 1;
    }
    psvr2client::Client client;
    if (!client.Start(pipe, (const std::uint8_t*)token, std::strlen(token))) {
        std::puts("FAIL: Start rejected");
        return 1;
    }
    // Pistol wall, then shotgun wall, then rifle fire vibration.
    client.Submit(psvr2trigger::ProfileFor(psvr2trigger::Weapon::Pistol, false));
    Sleep(300);
    client.Submit(psvr2trigger::ProfileFor(psvr2trigger::Weapon::Shotgun, false));
    Sleep(300);
    client.Submit(psvr2trigger::ProfileFor(psvr2trigger::Weapon::Rifle, true));
    Sleep(500); // let the worker deliver before teardown Off+Shutdown
    client.Stop();
    psvr2client::Stats s = client.GetStats();
    std::printf("connects=%u sends=%u errors=%u\n", s.connects, s.sends, s.errors);
    if (s.connects == 0 || s.sends < 3) {
        std::puts("FAIL: expected delivery to the bridge");
        return 1;
    }
    std::puts("PASS: loopback delivery confirmed");
    return 0;
}
