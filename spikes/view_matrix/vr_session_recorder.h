#pragma once

// Fixed-size, allocation-free runtime recorder.  The render/input path only
// copies a sample into a small buffer; the file is touched when that buffer is
// full or when recording stops.  The stream is deliberately simple so a
// desktop replay tool can consume it without loading the game or OpenXR.
#include <windows.h>
#include <cstdint>
#include <cstdio>

namespace vrrec {

static const std::uint32_t kVersion = 1;
static const std::uint32_t kMagic = 0x31524356u; // "VCR1"

enum Flags : std::uint32_t {
    Gameplay = 1u << 0, Menu = 1u << 1, Panel = 1u << 2,
    UiValid = 1u << 3, UiDown = 1u << 4, WeaponHolstered = 1u << 5,
    TwoHand = 1u << 6, Prompt = 1u << 7, Shadow = 1u << 8,
    Fire = 1u << 9, Melee = 1u << 10, Recenter = 1u << 11,
};

#pragma pack(push, 1)
struct FileHeader {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t sampleBytes;
    std::uint32_t reserved;
};

struct Sample {
    std::uint64_t frame;
    std::uint32_t timeMs;
    std::uint32_t flags;
    float headPos[3];
    float headYaw, headRoll;
    float handPos[2][3];
    float handYaw[2], handPitch[2], handRoll[2];
    float trigger[2], grip[2];
    float stick[2][2];
    float uiU, uiV;
    float muzzle[3];
    float hip[3];
    std::uint32_t weapon;
    std::uint32_t promptAction;
};
#pragma pack(pop)

class Recorder {
public:
    bool StartDefault() {
        char root[MAX_PATH] = {};
        DWORD n = GetEnvironmentVariableA("LOCALAPPDATA", root, (DWORD)sizeof(root));
        if (!n || n >= sizeof(root)) return false;
        char dir[MAX_PATH] = {};
        _snprintf_s(dir, sizeof(dir), _TRUNCATE, "%s\\SingularityVR\\vr_sessions", root);
        char parent[MAX_PATH] = {};
        _snprintf_s(parent, sizeof(parent), _TRUNCATE, "%s\\SingularityVR", root);
        CreateDirectoryA(parent, nullptr);
        CreateDirectoryA(dir, nullptr);
        SYSTEMTIME st{}; GetLocalTime(&st);
        char path[MAX_PATH] = {};
        _snprintf_s(path, sizeof(path), _TRUNCATE,
                    "%s\\session_%04u-%02u-%02u_%02u-%02u-%02u.vrrec", dir,
                    st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        return Start(path);
    }

    bool Start(const char* path) {
        Stop();
        if (!path || !*path) return false;
        file_ = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) return false;
        FileHeader h{kMagic, kVersion, (std::uint32_t)sizeof(Sample), 0};
        DWORD wrote = 0;
        if (!WriteFile(file_, &h, (DWORD)sizeof(h), &wrote, nullptr) || wrote != sizeof(h)) {
            Stop(); return false;
        }
        count_ = 0; active_ = true;
        return true;
    }

    void Stop() {
        if (file_ == INVALID_HANDLE_VALUE) { active_ = false; count_ = 0; return; }
        Flush(); CloseHandle(file_); file_ = INVALID_HANDLE_VALUE;
        active_ = false; count_ = 0;
    }

    bool Active() const { return active_; }

    void Tick(const Sample& s) {
        if (!active_) return;
        samples_[count_++] = s;
        if (count_ == kCapacity) Flush();
    }

    void Flush() {
        if (file_ == INVALID_HANDLE_VALUE || !count_) return;
        DWORD wrote = 0;
        const DWORD bytes = (DWORD)(count_ * sizeof(Sample));
        if (!WriteFile(file_, samples_, bytes, &wrote, nullptr) || wrote != bytes) {
            // Fail closed.  A partial stream is more useful than stalling the game.
            CloseHandle(file_); file_ = INVALID_HANDLE_VALUE; active_ = false; count_ = 0;
            return;
        }
        count_ = 0;
    }

private:
    static const std::uint32_t kCapacity = 64;
    HANDLE file_ = INVALID_HANDLE_VALUE;
    Sample samples_[kCapacity]{};
    std::uint32_t count_ = 0;
    bool active_ = false;
};

} // namespace vrrec
