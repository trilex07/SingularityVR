#pragma once

namespace vraudiofocus {
constexpr uintptr_t kBranchRva = 0x00de2d78;
constexpr uintptr_t kDefaultGainRva = 0x01396a8c;
constexpr uintptr_t kFocusGainRva = 0x017b7414;
constexpr SHORT kNativeBranch = 0x1f74; // je to xorps xmm0, xmm0
constexpr SHORT kKeepGain = static_cast<SHORT>(0x9090);
bool enabled = true;

bool IsFocusGainBranch(const unsigned char* image) {
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 1024) return false;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        nt->FileHeader.TimeDateStamp != 0x4c362bed || nt->OptionalHeader.SizeOfImage != 0x1bac000) return false;

    // The branch follows the real foreground test. The nonzero arm loads
    // Raven's gain, while the other arm zeros it before the same store.
    const auto* branch = image + kBranchRva;
    const unsigned char select[]{0x85,0xf6,0x74,0x1f,0xf3,0x0f,0x10,0x05};
    const unsigned char afterDefault[]{0xeb,0x18};
    const unsigned char zero[]{0x0f,0x57,0xc0};
    const unsigned char store[]{0xf3,0x0f,0x11,0x05};
    const auto defaultGain = reinterpret_cast<uintptr_t>(image) + kDefaultGainRva;
    const auto focusGain = reinterpret_cast<uintptr_t>(image) + kFocusGainRva;
    return memcmp(branch - 2, select, sizeof(select)) == 0 &&
        memcmp(branch + 6, &defaultGain, sizeof(uint32_t)) == 0 &&
        memcmp(branch + 10, afterDefault, sizeof(afterDefault)) == 0 &&
        memcmp(branch + 0x21, zero, sizeof(zero)) == 0 &&
        memcmp(branch + 0x2b, store, sizeof(store)) == 0 &&
        memcmp(branch + 0x2f, &focusGain, sizeof(uint32_t)) == 0;
}

void Update(bool xrAudioActive) {
    static bool applied = false, rejected = false;
    const bool keepAudio = enabled && xrAudioActive;
    if (keepAudio == applied || (rejected && !applied)) return;
    const auto image = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    if (!image || (keepAudio && !IsFocusGainBranch(image))) {
        if (!rejected) Log("VR audio focus: native focus-gain branch signature mismatch");
        rejected = true;
        return;
    }

    auto* branch = reinterpret_cast<volatile SHORT*>(image + kBranchRva);
    const SHORT before = applied ? kKeepGain : kNativeBranch;
    const SHORT after = keepAudio ? kKeepGain : kNativeBranch;
    DWORD protection = 0;
    if (!VirtualProtect(image + kBranchRva, sizeof(SHORT), PAGE_EXECUTE_READWRITE, &protection)) {
        if (!rejected) Log("VR audio focus: VirtualProtect failed");
        rejected = true;
        return;
    }
    const bool changed = _InterlockedCompareExchange16(branch, after, before) == before;
    if (changed) FlushInstructionCache(GetCurrentProcess(), image + kBranchRva, sizeof(SHORT));
    DWORD discarded = 0;
    const bool protectedAgain = VirtualProtect(image + kBranchRva, sizeof(SHORT), protection, &discarded) != FALSE;
    if (!changed || !protectedAgain) {
        if (!rejected) Log("VR audio focus: patch contract changed=%d protectionRestored=%d", changed, protectedAgain);
        rejected = true;
    }
    if (!changed) { applied = false; return; }
    applied = keepAudio;
    Log("VR audio focus: %s", applied ? "native gain kept for active XR" : "native focus mute restored");
}
}
