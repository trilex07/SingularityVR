#pragma once

// =============================================================================
// Generic XR postprocess stage (research/nvidia-vr-upscaling)
//
// A modular postprocess interface with pluggable backends. The existing NIS
// bridge (NisBridge_*) is one backend. A future DLSS-NR backend would be
// another, if NGX ever supports x86/D3D11.
//
// Design properties:
//   - No behavior change when disabled (mode None = passthrough).
//   - No dependency initialization when disabled (NGX/DLSS DLLs never loaded).
//   - Per-eye input/output (no sharing of temporal history across eyes).
//   - Device-generation aware (resources released on Reset).
//   - Frame-generation aware (temporal backends can key on frame serial).
//   - Clean shutdown (all resources released).
//   - Reset/recreate handling (device generation counter invalidates cache).
//
// The interface is intentionally small: one ProcessEye call, one Shutdown,
// one InvalidateDevice. Backend selection is by mode enum.
// =============================================================================

#include <d3d11.h>
#include <cstdint>

enum class XrPostprocessMode : int {
    None = 0,       // passthrough, no processing
    Nis = 1,        // existing NIS bridge (already integrated in d3d9.cpp)
    DlssNr = 2,     // future: DLSS-NR backend (blocked by x64/D3D12)
};

struct XrPostprocessInput {
    ID3D11Texture2D* color;         // input color texture (D3D11)
    ID3D11Texture2D* depth;         // input depth texture (optional, may be null)
    uint32_t eye;                   // 0=left, 1=right
    uint64_t frameGeneration;      // frame serial (for temporal backends)
    uint32_t width;
    uint32_t height;
    DXGI_FORMAT format;
};

struct XrPostprocessOutput {
    ID3D11Texture2D* color;         // output color texture (may equal input for passthrough)
};

// Process one eye. Returns true on success. When mode is None, output.color
// == input.color (passthrough). When mode is DlssNr and the backend is
// unavailable, returns false and output.color == input.color (caller falls
// back to native path).
bool XrPostprocess_ProcessEye(XrPostprocessInput& input, XrPostprocessOutput& output);

// Release all resources. Call on device reset or shutdown.
void XrPostprocess_Shutdown();

// Invalidate all cached resources (device generation change). Call from
// Hook_ResetDevice / Hook_ResetDeviceEx before the original reset.
void XrPostprocess_InvalidateDevice();

// Current mode (for diagnostics).
XrPostprocessMode XrPostprocess_GetMode();

// Set mode. Returns true if the mode is available (e.g., DlssNr returns false
// if NGX is x64-only).
bool XrPostprocess_SetMode(XrPostprocessMode mode);

// Current device generation (for diagnostics).
uint32_t XrPostprocess_GetDeviceGeneration();
