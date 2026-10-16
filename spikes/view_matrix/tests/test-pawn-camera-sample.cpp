#include "../pawn_camera_sample.h"
#include "../camera_stabilization.h"
#include <cstdio>
#include <cstring>

static int failures = 0;
static void Check(bool ok, const char* label) {
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    if (!ok) ++failures;
}
int main() {
    SYSTEM_INFO si{}; GetSystemInfo(&si);
    auto* page = static_cast<unsigned char*>(VirtualAlloc(nullptr, 2 * si.dwPageSize,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Check(page != nullptr, "private test pages allocated");
    if (!page) return 1;
    const uintptr_t base = reinterpret_cast<uintptr_t>(page);
    auto* location = reinterpret_cast<float*>(page + 0x54);
    location[0] = 10; location[1] = 20; location[2] = 30;
    *reinterpret_cast<float*>(page + 0x2C8) = 70;
    PawnCameraSample out{};
    Check(TryReadPawnCameraSample(base, 0x54, 0x2C8, &out) &&
        out.location[0] == 10 && out.location[1] == 20 && out.location[2] == 30 && out.eyeHeight == 70,
        "live sample is unchanged");
    float pov[3] = {11,22,103}, residual[3]{};
    Check(ComputeVrCameraStabilization(pov, out.location, out.eyeHeight, residual) &&
        residual[0] == -1 && residual[1] == -2 && residual[2] == -3, "valid camera math unchanged");
    int32_t op = 0, oy = 0, oroll = 0;
    Check(StabilizeVrCameraRotation(100, 200, 115, 205, 50, op, oy, oroll, true) &&
        op == 100 && oy == 200 && oroll == 0, "rotation stabilization locks pitch/yaw and zeros roll");
    Check(StabilizeVrCameraRotation(100, 200, 115, 205, 50, op, oy, oroll, false) &&
        op == 115 && oy == 205 && oroll == 0, "rotation stabilization without shake suppression keeps pov pitch/yaw and zeros roll");
    const PawnCameraSample before = out;

    DWORD previous = 0;
    Check(VirtualProtect(page + si.dwPageSize, si.dwPageSize, PAGE_NOACCESS, &previous) != 0,
        "eye-height page invalidated after a prior readable predicate");
    Check(!TryReadPawnCameraSample(base, 0x54, static_cast<int>(si.dwPageSize), &out),
        "partial location plus inaccessible eye height fails");
    Check(std::memcmp(&before, &out, sizeof(out)) == 0, "partial sample never published");
    // Exact faulting field: X/Y readable, Z on the inaccessible page.
    Check(!TryReadPawnCameraSample(base, static_cast<int>(si.dwPageSize) - 8, 0x2C8, &out),
        "location Z read fault contained");
    Check(std::memcmp(&before, &out, sizeof(out)) == 0, "failed Z read leaves output unchanged");
    Check(VirtualProtect(page, si.dwPageSize, PAGE_NOACCESS, &previous) != 0,
        "previously live pawn page invalidated");
    Check(!TryReadPawnCameraSample(base, 0x54, 0x2C8, &out), "stale aligned pawn fails safely");
    Check(std::memcmp(&before, &out, sizeof(out)) == 0, "stale sample never published");
    Check(!TryReadPawnCameraSample(0, 0x54, 0x2C8, &out), "null pawn refused");
    Check(!TryReadPawnCameraSample(base + 1, 0x54, 0x2C8, &out), "unaligned pawn refused");
    Check(!TryReadPawnCameraSample(base, -4, 0x2C8, &out), "negative offset refused");
    Check(!TryReadPawnCameraSample(UINTPTR_MAX - 3, 0x54, 0x2C8, &out), "address overflow refused");
    Check(MemoryFaultFilter(EXCEPTION_INT_DIVIDE_BY_ZERO) == EXCEPTION_CONTINUE_SEARCH,
        "non-memory exception is not swallowed");
    VirtualFree(page, 0, MEM_RELEASE);
    Check(!TryReadPawnCameraSample(base, 0x54, 0x2C8, &out), "released private page fails safely");
    return failures ? 1 : 0;
}
