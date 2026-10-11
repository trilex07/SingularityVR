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

    Expect(MemoryFaultFilter(EXCEPTION_ACCESS_VIOLATION) == EXCEPTION_EXECUTE_HANDLER,
           "access violation is handled");
    Expect(MemoryFaultFilter(EXCEPTION_IN_PAGE_ERROR) == EXCEPTION_EXECUTE_HANDLER,
           "in-page error is handled");
    Expect(MemoryFaultFilter(EXCEPTION_INT_DIVIDE_BY_ZERO) == EXCEPTION_CONTINUE_SEARCH,
           "divide by zero is not swallowed");
    Expect(MemoryFaultFilter(0xE0000001u) == EXCEPTION_CONTINUE_SEARCH,
           "unrelated exception code is not swallowed");

    alignas(16) uint32_t validFrame[16] = {};
    validFrame[4] = 0x00010000u;
    uint32_t copied[16] = {};
    Expect(TryLoadFrameSlots(reinterpret_cast<uintptr_t>(validFrame), copied, 16),
           "valid frame slots load");
    Expect(copied[4] == validFrame[4], "valid frame slot value preserved");
    Expect(!ScriptNodeCandidate(copied[4], reinterpret_cast<uintptr_t>(validFrame)),
           "below-window slot from a valid frame is not a candidate");
    int32_t named = 99;
    Expect(TryLoadNodeName(reinterpret_cast<uintptr_t>(validFrame), 16, 0x2C, &named) && named == -1,
           "valid frame with an out-of-window node does not fault");

    void* deadFrame = VirtualAlloc(nullptr, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Expect(deadFrame != nullptr, "frame page allocated");
    if (deadFrame) {
        DWORD old = 0;
        Expect(VirtualProtect(deadFrame, 0x1000, PAGE_NOACCESS, &old) != 0, "frame page set to NOACCESS");
        uint32_t deadSlots[16] = {};
        deadSlots[0] = 0xABu;
        Expect(!TryLoadFrameSlots(reinterpret_cast<uintptr_t>(deadFrame), deadSlots, 16),
               "inaccessible FFrame slot source fails");
        Expect(deadSlots[0] == 0xABu, "failed frame load leaves the slot buffer unchanged");
        int32_t deadName = 0x7E;
        Expect(!TryLoadNodeName(reinterpret_cast<uintptr_t>(deadFrame), 0, 0x2C, &deadName),
               "inaccessible decided-node pointer load fails");
        Expect(deadName == 0x7E, "failed node-pointer load leaves the name unchanged");
        VirtualFree(deadFrame, 0, MEM_RELEASE);
    }

    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    void* guard = VirtualAlloc(nullptr, info.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Expect(guard != nullptr, "guard page allocated");
    if (guard) {
        DWORD old = 0;
        Expect(VirtualProtect(guard, info.dwPageSize, PAGE_READWRITE | PAGE_GUARD, &old) != 0,
               "guard page armed");
        bool propagated = false;
        __try {
            int32_t sink = 0;
            TryLoadAlignedI32(reinterpret_cast<uintptr_t>(guard), &sink);
        } __except (GetExceptionCode() == 0x80000001u ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            propagated = true;
        }
        Expect(propagated, "guard-page exception is not swallowed");
        VirtualFree(guard, 0, MEM_RELEASE);
    }

    void* hotFrame = nullptr;
    void* hotNode = nullptr;
    for (uintptr_t base = 0x20000000u; base < 0x50000000u && (!hotFrame || !hotNode); base += 0x10000u) {
        if (!hotFrame) hotFrame = VirtualAlloc(reinterpret_cast<void*>(base), 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        else if (!hotNode) hotNode = VirtualAlloc(reinterpret_cast<void*>(base), 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }
    Expect(hotFrame && hotNode, "in-range hotpath pages allocated");
    double guardedNs = 0;
    double rawNs = 0;
    if (hotFrame && hotNode) {
        *reinterpret_cast<int32_t*>(static_cast<unsigned char*>(hotNode) + 0x2C) = 17;
        auto* slots = static_cast<uint32_t*>(hotFrame);
        slots[0] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hotNode));
        int32_t check = 0;
        Expect(TryLoadNodeName(reinterpret_cast<uintptr_t>(hotFrame), 0, 0x2C, &check) && check == 17,
               "valid in-range frame and node return the name");
        const int iters = 200000;
        LARGE_INTEGER freq{}, t0{}, t1{};
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&t0);
        for (int i = 0; i < iters; ++i) {
            int32_t idx = 0;
            TryLoadNodeName(reinterpret_cast<uintptr_t>(hotFrame), 0, 0x2C, &idx);
            if (idx != 17) check = -1;
        }
        QueryPerformanceCounter(&t1);
        guardedNs = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)freq.QuadPart / iters;
        QueryPerformanceCounter(&t0);
        for (int i = 0; i < iters; ++i) {
            const uintptr_t node = *reinterpret_cast<const volatile uintptr_t*>(hotFrame);
            check = *reinterpret_cast<const volatile int32_t*>(node + 0x2C);
        }
        QueryPerformanceCounter(&t1);
        rawNs = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)freq.QuadPart / iters;
        std::printf("HOTPATH guarded %.1f ns/call, raw %.1f ns/call\n", guardedNs, rawNs);
        Expect(check == 17, "hotpath samples stayed on the valid name");
        // A few thousand script calls per second. 2 us/call would be about 0.3 ms
        // inside a 90 Hz frame; measured cost should stay well under that.
        Expect(guardedNs < 2000.0, "guarded node-name load stays under 2 us");
        VirtualFree(hotFrame, 0, MEM_RELEASE);
        VirtualFree(hotNode, 0, MEM_RELEASE);
    }

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
