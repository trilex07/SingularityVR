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
    float distantPov[3] = {1000, 2000, 3000};
    Check(!ComputeVrCameraStabilization(distantPov, out.location, out.eyeHeight, residual) &&
        residual[0] == 0 && residual[1] == 0 && residual[2] == 0, "distant pawn offset safely rejected");
    float sprintPov[3] = {10 + 80.0f, 22, 103};
    Check(ComputeVrCameraStabilization(sprintPov, out.location, out.eyeHeight, residual) &&
        std::fabs(residual[0] + 35.0f) < 1e-3f && residual[1] == -2 && residual[2] == -3,
        "large sprint bob is clamped instead of disabling stabilization");
    float raw[3] = {10, 5, 2}, smooth[3] = {0, 0, 0};
    SmoothVrCameraStabilizationOffset(raw, smooth, 0.5f);
    Check(std::fabs(smooth[0] - 5.0f) < 1e-5f && std::fabs(smooth[1] - 2.5f) < 1e-5f && std::fabs(smooth[2] - 1.0f) < 1e-5f,
        "stabilization offset low-pass moves toward raw sample");
    int32_t op = 0, oy = 0, oroll = 0;
    Check(StabilizeVrCameraRotation(100, 200, 115, 205, 50, op, oy, oroll, true) &&
        op == 100 && oy == 200 && oroll == 0, "rotation stabilization locks pitch/yaw and zeros roll");
    Check(StabilizeVrCameraRotation(100, 200, 115, 205, 50, op, oy, oroll, false) &&
        op == 115 && oy == 205 && oroll == 0, "rotation stabilization without shake suppression keeps pov pitch/yaw and zeros roll");

    // Test CancelBasisRoll:
    // 1. Unrolled baseline basis (forward=+X, right=+Y, up=+Z)
    {
        const float fIn[3] = {1, 0, 0}, rIn[3] = {0, 1, 0}, uIn[3] = {0, 0, 1};
        float rOut[3]{}, uOut[3]{}, fOut[3]{}, R[3][3]{};
        Check(CancelBasisRoll(rIn, uIn, fIn, rOut, uOut, fOut, R), "unrolled basis processed");
        Check(std::fabs(rOut[2]) < 1e-5f && std::fabs(rOut[1] - 1.0f) < 1e-5f, "unrolled right preserved");
        Check(std::fabs(fOut[0] - 1.0f) < 1e-5f && std::fabs(fOut[1]) < 1e-5f && std::fabs(fOut[2]) < 1e-5f, "unrolled forward preserved");
    }
    // 2. Basis with 30 deg roll shake injected
    {
        const float ang = 0.523598775f; // 30 deg
        const float fIn[3] = {1, 0, 0};
        const float rIn[3] = {0, std::cos(ang), std::sin(ang)};
        const float uIn[3] = {0, -std::sin(ang), std::cos(ang)};
        float rOut[3]{}, uOut[3]{}, fOut[3]{}, R[3][3]{};
        Check(CancelBasisRoll(rIn, uIn, fIn, rOut, uOut, fOut, R), "rolled basis processed");
        Check(std::fabs(rOut[2]) < 1e-5f && std::fabs(rOut[1] - 1.0f) < 1e-5f, "roll eliminated (right.z == 0, right.y == 1)");
        Check(std::fabs(uOut[2] - 1.0f) < 1e-5f && std::fabs(uOut[1]) < 1e-5f, "upright vertical (up.z == 1, up.y == 0)");
        Check(std::fabs(fOut[0] - 1.0f) < 1e-5f && std::fabs(fOut[1]) < 1e-5f && std::fabs(fOut[2]) < 1e-5f, "forward look invariant");
        // Verify R rotates rIn to rOut and leaves fIn unchanged
        float rotatedR[3] = {
            R[0][0]*rIn[0] + R[0][1]*rIn[1] + R[0][2]*rIn[2],
            R[1][0]*rIn[0] + R[1][1]*rIn[1] + R[1][2]*rIn[2],
            R[2][0]*rIn[0] + R[2][1]*rIn[1] + R[2][2]*rIn[2]
        };
        float rotatedF[3] = {
            R[0][0]*fIn[0] + R[0][1]*fIn[1] + R[0][2]*fIn[2],
            R[1][0]*fIn[0] + R[1][1]*fIn[1] + R[1][2]*fIn[2],
            R[2][0]*fIn[0] + R[2][1]*fIn[1] + R[2][2]*fIn[2]
        };
        Check(std::fabs(rotatedR[2]) < 1e-5f && std::fabs(rotatedR[1] - 1.0f) < 1e-5f, "R matrix unrolls right vector");
        Check(std::fabs(rotatedF[0] - 1.0f) < 1e-5f && std::fabs(rotatedF[1]) < 1e-5f && std::fabs(rotatedF[2]) < 1e-5f, "R matrix preserves forward look vector");
    }

    // 3. Test StabilizeBasisToLook: shake injected in pitch, yaw, and roll
    {
        const float pitchShake = -0.15f; // ~ -8.6 deg melee dip
        const float yawShake   =  0.08f; // ~ +4.6 deg yaw twitch
        const float rollShake  =  0.25f; // ~ +14.3 deg roll
        // Construct shaken basis
        const float cp = std::cos(pitchShake), sp = std::sin(pitchShake);
        const float cy = std::cos(yawShake),   sy = std::sin(yawShake);
        const float fIn[3] = { cp * cy, cp * sy, sp };
        const float r0[3]  = { -sy, cy, 0.0f };
        const float u0[3]  = { fIn[1]*r0[2] - fIn[2]*r0[1], fIn[2]*r0[0] - fIn[0]*r0[2], fIn[0]*r0[1] - fIn[1]*r0[0] };
        // Apply roll shake
        const float cr = std::cos(rollShake), sr = std::sin(rollShake);
        const float rIn[3] = { cr*r0[0] + sr*u0[0], cr*r0[1] + sr*u0[1], cr*r0[2] + sr*u0[2] };
        const float uIn[3] = { -sr*r0[0] + cr*u0[0], -sr*r0[1] + cr*u0[1], -sr*r0[2] + cr*u0[2] };

        // Target look angles: level forward look (pitch=0, yaw=0)
        float rOut[3]{}, uOut[3]{}, fOut[3]{}, R[3][3]{};
        Check(StabilizeBasisToLook(rIn, uIn, fIn, 0.0f, 0.0f, rOut, uOut, fOut, R), "StabilizeBasisToLook processed");
        // Desired forward is +X (1, 0, 0), right is +Y (0, 1, 0), up is +Z (0, 0, 1)
        Check(std::fabs(fOut[0] - 1.0f) < 1e-5f && std::fabs(fOut[1]) < 1e-5f && std::fabs(fOut[2]) < 1e-5f,
              "StabilizeBasisToLook forward look locked to target");
        Check(std::fabs(rOut[0]) < 1e-5f && std::fabs(rOut[1] - 1.0f) < 1e-5f && std::fabs(rOut[2]) < 1e-5f,
              "StabilizeBasisToLook right vector locked (zero roll)");
        Check(std::fabs(uOut[0]) < 1e-5f && std::fabs(uOut[1]) < 1e-5f && std::fabs(uOut[2] - 1.0f) < 1e-5f,
              "StabilizeBasisToLook up vector locked to +Z");

        // Verify R transforms the shaken basis into the target basis
        float rotatedF[3] = {
            R[0][0]*fIn[0] + R[0][1]*fIn[1] + R[0][2]*fIn[2],
            R[1][0]*fIn[0] + R[1][1]*fIn[1] + R[1][2]*fIn[2],
            R[2][0]*fIn[0] + R[2][1]*fIn[1] + R[2][2]*fIn[2]
        };
        float rotatedR[3] = {
            R[0][0]*rIn[0] + R[0][1]*rIn[1] + R[0][2]*rIn[2],
            R[1][0]*rIn[0] + R[1][1]*rIn[1] + R[1][2]*rIn[2],
            R[2][0]*rIn[0] + R[2][1]*rIn[1] + R[2][2]*rIn[2]
        };
        Check(std::fabs(rotatedF[0] - 1.0f) < 1e-5f && std::fabs(rotatedF[1]) < 1e-5f && std::fabs(rotatedF[2]) < 1e-5f,
              "R matrix restores shaken forward to clean look");
        Check(std::fabs(rotatedR[1] - 1.0f) < 1e-5f && std::fabs(rotatedR[2]) < 1e-5f,
              "R matrix restores shaken right to zero-roll");
    }

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
