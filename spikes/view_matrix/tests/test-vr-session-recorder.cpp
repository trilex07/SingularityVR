#include "../vr_session_recorder.h"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    const char* path = "vr_session_recorder_test.vrrec";
    vrrec::Recorder r;
    assert(r.Start(path));
    vrrec::Sample s{};
    s.frame = 7; s.flags = vrrec::Gameplay | vrrec::Fire | vrrec::Melee | vrrec::Recenter;
    s.handPos[1][2] = -0.42f; s.uiU = .5f; s.uiV = .25f; s.weapon = 11387;
    for (int i = 0; i < 65; ++i) { s.frame = (std::uint64_t)i; r.Tick(s); }
    r.Stop();
    FILE* f = nullptr; assert(fopen_s(&f, path, "rb") == 0 && f);
    vrrec::FileHeader h{}; assert(fread(&h, 1, sizeof(h), f) == sizeof(h));
    assert(h.magic == vrrec::kMagic && h.version == vrrec::kVersion);
    assert(h.sampleBytes == sizeof(vrrec::Sample));
    vrrec::Sample got{}; assert(fread(&got, 1, sizeof(got), f) == sizeof(got));
    assert(got.frame == 0 && got.flags == s.flags && got.weapon == s.weapon);
    fclose(f); remove(path);
    std::puts("PASS: fixed-size VR session recorder header, buffered samples, replay-readable stream");
}
