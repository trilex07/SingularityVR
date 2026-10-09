// =============================================================================
// Generic XR postprocess stage — implementation (research/nvidia-vr-upscaling)
//
// Phase A: generic interface with mode dispatch. NIS backend is a passthrough
// (already integrated in d3d9.cpp). DLSS-NR backend is a stub that returns
// false (NGX core is x64-only, DLSS-NR requires D3D12).
//
// The interface is designed so a future DLSS backend can be added by:
//   1. Implementing the backend in xr_postprocess_dlss.h/.cpp
//   2. Adding a case in the mode dispatch below
//   3. Loading the NGX runtime dynamically (LoadLibrary)
//
// No behavior change when disabled. No dependency initialization when disabled.
// =============================================================================

#include "xr_postprocess.h"

#include <atomic>

// Current mode. Default None (no behavior change).
static std::atomic<int> g_xrPostprocessMode{static_cast<int>(XrPostprocessMode::None)};

// Device generation counter. Incremented on Reset. Resources tagged with
// an old generation are considered stale.
static std::atomic<uint32_t> g_xrPostprocessDeviceGeneration{0};

// ---------------------------------------------------------------------------
// Mode dispatch
// ---------------------------------------------------------------------------

bool XrPostprocess_ProcessEye(XrPostprocessInput& input, XrPostprocessOutput& output) {
    const auto mode = static_cast<XrPostprocessMode>(g_xrPostprocessMode.load(std::memory_order_relaxed));

    // Default: passthrough (output = input). This is the None mode and the
    // fallback for any backend that cannot process the frame.
    output.color = input.color;

    switch (mode) {
        case XrPostprocessMode::None:
            // Passthrough. No processing. No resources created.
            return true;

        case XrPostprocessMode::Nis:
            // NIS is already integrated in d3d9.cpp (NisBridge_* calls in
            // Hook_Present). This stage is a passthrough for NIS — the
            // processing happens before this point in the frame.
            return true;

        case XrPostprocessMode::DlssNr:
            // DLSS-NR backend is blocked:
            //   1. NGX core (nvsdk_ngx_s.lib) is x64-only — cannot link from x86.
            //   2. DLSS-NR entry points are D3D12-only (NVSDK_NGX_D3D12_*).
            //   3. Required inputs (motion vectors, normals, albedo, roughness,
            //      exposure) are not available in the UE3 D3D9 pipeline.
            //
            // Return false so the caller falls back to the native path.
            // output.color is already set to input.color (passthrough).
            return false;
    }

    // Unknown mode: passthrough.
    return true;
}

void XrPostprocess_Shutdown() {
    // No resources to release in Phase A. Future backends will release here.
    g_xrPostprocessMode.store(static_cast<int>(XrPostprocessMode::None), std::memory_order_relaxed);
}

void XrPostprocess_InvalidateDevice() {
    // Increment device generation. Future backends will use this to
    // invalidate cached resources.
    g_xrPostprocessDeviceGeneration.fetch_add(1, std::memory_order_relaxed);
}

XrPostprocessMode XrPostprocess_GetMode() {
    return static_cast<XrPostprocessMode>(g_xrPostprocessMode.load(std::memory_order_relaxed));
}

bool XrPostprocess_SetMode(XrPostprocessMode mode) {
    switch (mode) {
        case XrPostprocessMode::None:
        case XrPostprocessMode::Nis:
            g_xrPostprocessMode.store(static_cast<int>(mode), std::memory_order_relaxed);
            return true;

        case XrPostprocessMode::DlssNr:
            // NGX core is x64-only. DLSS-NR requires D3D12. This backend
            // is not available in the x86/D3D9 pipeline.
            return false;
    }
    return false;
}

uint32_t XrPostprocess_GetDeviceGeneration() {
    return g_xrPostprocessDeviceGeneration.load(std::memory_order_relaxed);
}
