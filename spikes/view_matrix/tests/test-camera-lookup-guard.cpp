#include "../camera_lookup_guard.h"
#include <cstdio>

static int failures = 0;
static void Check(bool ok, const char* label) {
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    if (!ok) ++failures;
}
static uintptr_t ReadClass(void* object) {
    return *reinterpret_cast<const volatile uintptr_t*>(
        reinterpret_cast<uintptr_t>(object) + 0x34);
}
static uintptr_t ReadOuterClass(void* object) {
    const uintptr_t outer = *reinterpret_cast<const volatile uintptr_t*>(
        reinterpret_cast<uintptr_t>(object) + 0x28);
    return ReadClass(reinterpret_cast<void*>(outer));
}
static uintptr_t Miss(void*) { return 0; }
static uintptr_t RaiseNonMemory(void*) {
    RaiseException(EXCEPTION_INT_DIVIDE_BY_ZERO, 0, 0, nullptr);
    return 0;
}
static bool NonMemoryPropagates() {
    uintptr_t out = 0;
    __try { TryCameraLookup(RaiseNonMemory, nullptr, &out); }
    __except (GetExceptionCode() == EXCEPTION_INT_DIVIDE_BY_ZERO ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return true; }
    return false;
}
int main() {
    SYSTEM_INFO si{}; GetSystemInfo(&si);
    auto* pages = static_cast<unsigned char*>(VirtualAlloc(nullptr, 2 * si.dwPageSize,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Check(pages != nullptr, "private pages allocated");
    if (!pages) return 1;
    const uintptr_t expected = 0x12345678;
    *reinterpret_cast<uintptr_t*>(pages + 0x34) = expected;
    *reinterpret_cast<uintptr_t*>(pages + 0x28) = reinterpret_cast<uintptr_t>(pages + si.dwPageSize);
    *reinterpret_cast<uintptr_t*>(pages + si.dwPageSize + 0x34) = expected;
    uintptr_t out = 0;
    Check(TryCameraLookup(ReadClass, pages, &out) && out == expected, "valid class lookup unchanged");
    Check(TryCameraLookup(ReadOuterClass, pages, &out) && out == expected, "valid nested lookup unchanged");
    DWORD previous = 0;
    Check(VirtualProtect(pages + si.dwPageSize, si.dwPageSize, PAGE_NOACCESS, &previous) != 0,
          "Outer mapping invalidated after successful lookup");
    Check(!TryCameraLookup(ReadOuterClass, pages, &out) && out == expected,
          "nested name/Outer fault contained, output unpublished");
    Check(VirtualProtect(pages, si.dwPageSize, PAGE_NOACCESS, &previous) != 0,
          "cached camera mapping invalidated");
    Check(!TryCameraLookup(ReadClass, pages, &out) && out == expected,
          "exact stale camera Class +0x34 read contained");
    Check(TryCameraLookup(Miss, nullptr, &out) && out == 0, "ordinary miss is not a fault");
    Check(!TryCameraLookup(nullptr, nullptr, &out), "null callback rejected");
    Check(!TryCameraLookup(Miss, nullptr, nullptr), "null output rejected");
    Check(NonMemoryPropagates(), "non-memory exception propagates to caller");
    VirtualFree(pages, 0, MEM_RELEASE);
    out = expected;
    Check(!TryCameraLookup(ReadClass, pages, &out) && out == expected, "released camera cannot publish a result");
    return failures ? 1 : 0;
}
