// =============================================================================
// Standalone test for the generic XR postprocess interface.
// x86, /O2, no NVIDIA dependencies. Tests lifecycle, mode dispatch, and
// fallback behavior.
// =============================================================================

#include "xr_postprocess.h"
#include "xr_postprocess_dlss.h"

#include <cassert>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { ++g_testsPassed; } \
        else { ++g_testsFailed; printf("FAIL: %s (line %d)\n", msg, __LINE__); } \
    } while (0)

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// Mode None: passthrough, output == input, no resources created.
void test_mode_none_passthrough() {
    XrPostprocess_SetMode(XrPostprocessMode::None);

    XrPostprocessInput in{};
    in.color = reinterpret_cast<ID3D11Texture2D*>(0x1234);
    in.depth = nullptr;
    in.eye = 0;
    in.frameGeneration = 42;
    in.width = 5264;
    in.height = 2684;
    in.format = DXGI_FORMAT_B8G8R8A8_UNORM;

    XrPostprocessOutput out{};
    const bool ok = XrPostprocess_ProcessEye(in, out);

    TEST_ASSERT(ok, "None mode should succeed");
    TEST_ASSERT(out.color == in.color, "None mode should passthrough");
    TEST_ASSERT(XrPostprocess_GetMode() == XrPostprocessMode::None, "mode should be None");
}

// Mode Nis: passthrough (NIS already integrated in d3d9.cpp).
void test_mode_nis_passthrough() {
    XrPostprocess_SetMode(XrPostprocessMode::Nis);

    XrPostprocessInput in{};
    in.color = reinterpret_cast<ID3D11Texture2D*>(0x5678);
    in.eye = 1;
    in.frameGeneration = 99;

    XrPostprocessOutput out{};
    const bool ok = XrPostprocess_ProcessEye(in, out);

    TEST_ASSERT(ok, "NIS mode should succeed");
    TEST_ASSERT(out.color == in.color, "NIS mode should passthrough (already integrated)");
}

// Mode DlssNr: returns false (NGX x64-only), output == input (fallback).
void test_mode_dlssnr_unavailable() {
    const bool setOk = XrPostprocess_SetMode(XrPostprocessMode::DlssNr);
    TEST_ASSERT(!setOk, "DlssNr mode should be unavailable (x64-only NGX)");

    // After failed SetMode, mode should remain None (or previous).
    // ProcessEye should still work (passthrough).
    XrPostprocessInput in{};
    in.color = reinterpret_cast<ID3D11Texture2D*>(0xABCD);
    in.eye = 0;

    XrPostprocessOutput out{};
    const bool ok = XrPostprocess_ProcessEye(in, out);

    TEST_ASSERT(ok, "ProcessEye should succeed even when backend unavailable");
    TEST_ASSERT(out.color == in.color, "Fallback should passthrough");
}

// Left/right eye state isolation: processing one eye doesn't affect the other.
void test_eye_isolation() {
    XrPostprocess_SetMode(XrPostprocessMode::None);

    XrPostprocessInput left{};
    left.color = reinterpret_cast<ID3D11Texture2D*>(0x1111);
    left.eye = 0;
    left.frameGeneration = 100;

    XrPostprocessInput right{};
    right.color = reinterpret_cast<ID3D11Texture2D*>(0x2222);
    right.eye = 1;
    right.frameGeneration = 100;

    XrPostprocessOutput outLeft{}, outRight{};
    XrPostprocess_ProcessEye(left, outLeft);
    XrPostprocess_ProcessEye(right, outRight);

    TEST_ASSERT(outLeft.color == left.color, "Left eye passthrough");
    TEST_ASSERT(outRight.color == right.color, "Right eye passthrough");
    TEST_ASSERT(outLeft.color != outRight.color, "Eyes should be independent");
}

// Device generation reset: InvalidateDevice increments generation.
void test_device_generation_reset() {
    const uint32_t genBefore = XrPostprocess_GetDeviceGeneration();
    XrPostprocess_InvalidateDevice();
    const uint32_t genAfter = XrPostprocess_GetDeviceGeneration();
    TEST_ASSERT(genAfter == genBefore + 1, "Device generation should increment");
}

// Dimension change: interface accepts different dimensions without crash.
void test_dimension_change() {
    XrPostprocess_SetMode(XrPostprocessMode::None);

    XrPostprocessInput in{};
    in.color = reinterpret_cast<ID3D11Texture2D*>(0x3333);
    in.eye = 0;
    in.width = 1920;
    in.height = 1080;
    in.format = DXGI_FORMAT_B8G8R8A8_UNORM;

    XrPostprocessOutput out{};
    const bool ok = XrPostprocess_ProcessEye(in, out);
    TEST_ASSERT(ok, "Dimension change should not crash");
    TEST_ASSERT(out.color == in.color, "Passthrough after dimension change");
}

// Resource release: Shutdown doesn't crash and resets mode.
void test_shutdown() {
    XrPostprocess_SetMode(XrPostprocessMode::Nis);
    XrPostprocess_Shutdown();
    TEST_ASSERT(XrPostprocess_GetMode() == XrPostprocessMode::None, "Shutdown should reset mode");

    // ProcessEye after shutdown should still work (passthrough).
    XrPostprocessInput in{};
    in.color = reinterpret_cast<ID3D11Texture2D*>(0x4444);
    XrPostprocessOutput out{};
    const bool ok = XrPostprocess_ProcessEye(in, out);
    TEST_ASSERT(ok, "ProcessEye after shutdown should passthrough");
}

// Invalid texture format: interface accepts without crash.
void test_invalid_format() {
    XrPostprocess_SetMode(XrPostprocessMode::None);

    XrPostprocessInput in{};
    in.color = reinterpret_cast<ID3D11Texture2D*>(0x5555);
    in.format = static_cast<DXGI_FORMAT>(0xFFFF);  // invalid

    XrPostprocessOutput out{};
    const bool ok = XrPostprocess_ProcessEye(in, out);
    TEST_ASSERT(ok, "Invalid format should not crash");
    TEST_ASSERT(out.color == in.color, "Passthrough for invalid format");
}

// Unsupported input combination: depth provided but mode is None.
void test_unsupported_input_combination() {
    XrPostprocess_SetMode(XrPostprocessMode::None);

    XrPostprocessInput in{};
    in.color = reinterpret_cast<ID3D11Texture2D*>(0x6666);
    in.depth = reinterpret_cast<ID3D11Texture2D*>(0x7777);  // depth provided
    in.eye = 0;

    XrPostprocessOutput out{};
    const bool ok = XrPostprocess_ProcessEye(in, out);
    TEST_ASSERT(ok, "Unsupported input should not crash");
    TEST_ASSERT(out.color == in.color, "Passthrough for unsupported input");
}

// DLSS probe: never claims availability on this x86/D3D9 pipeline.
void test_dlss_probe_never_available() {
    const XrPostprocessDlssStatus status = XrPostprocessDlss_Probe();
    TEST_ASSERT(status.reason != XrPostprocessDlssStatus::Reason::Available,
                "DLSS probe must never report Available (NGX is x64/D3D12-only)");
    TEST_ASSERT(status.entryPointsAreD3D12Only, "entry points are D3D12-only");
    TEST_ASSERT(!status.requiredInputsAvailable, "required inputs are unavailable");
}

// DLSS probe: side-effect free. It must not load NGX into our process.
void test_dlss_probe_does_not_load_runtime() {
    XrPostprocessDlss_Probe();
    TEST_ASSERT(GetModuleHandleW(L"_nvngx.dll") == nullptr,
                "probe must not load _nvngx.dll into the process");
    TEST_ASSERT(GetModuleHandleW(L"nvngx_dlssnr.dll") == nullptr,
                "probe must not load nvngx_dlssnr.dll into the process");
}

// DLSS backend: Init refuses, Shutdown is a no-op.
void test_dlss_backend_refuses() {
    TEST_ASSERT(!XrPostprocessDlss_Init(nullptr), "DLSS Init must refuse (no x86 NGX)");
    XrPostprocessDlss_Shutdown();
    TEST_ASSERT(true, "DLSS Shutdown must not crash");
}

// DLSS probe: reason text is stable for every enum value.
void test_dlss_reason_text() {
    TEST_ASSERT(XrPostprocessDlss_ReasonText(
                    XrPostprocessDlssStatus::Reason::NotPresent) != nullptr,
                "reason text must not be null");
    TEST_ASSERT(strlen(XrPostprocessDlss_ReasonText(
                        XrPostprocessDlssStatus::Reason::X64Only)) > 0,
                "reason text must not be empty");
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    printf("=== XR Postprocess Interface Tests ===\n");

    test_mode_none_passthrough();
    test_mode_nis_passthrough();
    test_mode_dlssnr_unavailable();
    test_eye_isolation();
    test_device_generation_reset();
    test_dimension_change();
    test_shutdown();
    test_invalid_format();
    test_unsupported_input_combination();
    test_dlss_probe_never_available();
    test_dlss_probe_does_not_load_runtime();
    test_dlss_backend_refuses();
    test_dlss_reason_text();

    printf("\n=== Results: %d passed, %d failed ===\n", g_testsPassed, g_testsFailed);
    return g_testsFailed == 0 ? 0 : 1;
}
