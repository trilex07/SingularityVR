// The no-HMD crash read ESI=0x3E0ED111 and faulted at ESI+0x2C.
// Alignment must reject that pointer before any load. A later NOACCESS
// page must also fail without killing the process: alignment is not enough.
#include "../script_node_candidate.h"

#include <cstdint>
#include <cstdio>

static int g_fails = 0;

static void Expect(bool cond, const char* name) {
    if (cond) {
        std::printf("PASS %s\n", name);
        return;
    }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    const uintptr_t crashEsi = 0x3E0ED111u;
    const uintptr_t dumpFrame = 0x022AFCE4u;
    Expect(!ScriptNodeCandidate(crashEsi, 0), "recorded unaligned ESI rejected");
    Expect(!ScriptNodeCandidate(crashEsi, dumpFrame), "recorded ESI rejected beside the dump frame");
    int32_t unused = 0x7E;
    Expect(!TryLoadAlignedI32(crashEsi + 0x2C, &unused), "recorded fault address is not loaded");
    Expect(unused == 0x7E, "refused load does not write the output");

    Expect(ScriptNodeCandidate(0x3E0ED110u, 0), "aligned neighbour of the crash ESI accepted");
    Expect(ScriptNodeCandidate(0x01000000u, 0), "aligned low bound accepted");
    Expect(!ScriptNodeCandidate(0x01000001u, 0), "unaligned low bound rejected");
    Expect(!ScriptNodeCandidate(0x00FFFFFCu, 0), "pointer below the window rejected");
    Expect(!ScriptNodeCandidate(0x7FFF0004u, 0), "pointer above the window rejected");
    Expect(!ScriptNodeCandidate(0x10000002u, dumpFrame), "2-mod-4 pointer rejected");
    Expect(!ScriptNodeCandidate(dumpFrame + 0x100u, dumpFrame), "stack neighbour rejected");
    Expect(ScriptNodeCandidate((dumpFrame + 0x20000u) & ~uintptr_t{3}, dumpFrame),
           "aligned pointer outside the stack window accepted");

    int32_t live = 0x11223344;
    int32_t got = 0;
    Expect(TryLoadAlignedI32(reinterpret_cast<uintptr_t>(&live), &got) && got == live,
           "aligned readable dword loads");
    Expect(!TryLoadAlignedI32(reinterpret_cast<uintptr_t>(&live) + 1, &got),
           "unaligned readable address refused");

    void* page = VirtualAlloc(nullptr, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Expect(page != nullptr, "committed page allocated");
    if (page) {
        *reinterpret_cast<int32_t*>(page) = 7;
        DWORD old = 0;
        Expect(VirtualProtect(page, 0x1000, PAGE_NOACCESS, &old) != 0, "page set to NOACCESS");
        const uintptr_t aligned = reinterpret_cast<uintptr_t>(page);
        Expect((aligned & 3u) == 0, "NOACCESS probe is aligned");
        got = 0x7E;
        Expect(!TryLoadAlignedI32(aligned, &got), "aligned NOACCESS load fails safely");
        Expect(got == 0x7E, "failed NOACCESS load leaves the output unchanged");
        VirtualFree(page, 0, MEM_RELEASE);
    }

    return g_fails == 0 ? 0 : 1;
}
