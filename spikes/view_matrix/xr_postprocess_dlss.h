#pragma once

// =============================================================================
// DLSS-NR backend — capability probe (research/nvidia-vr-upscaling)
//
// Feasibility result (2026-10-09): NO-GO for an embedded x86 backend.
//
//   1. NGX import libraries (nvsdk_ngx_s.lib / nvsdk_ngx_d.lib) are
//      x64-only. NVIDIA NGX Programming Guide: "Both files are located
//      in the \lib\x64 folder (NGX is provided as a 64bit only library)."
//      They cannot be linked from a 32-bit process.
//   2. The reference layer drives DLSS-NR exclusively through
//      NVSDK_NGX_D3D12_* entry points (NgxNr.cpp: ID3D12Device,
//      ID3D12GraphicsCommandList, ID3D12Resource). There is no D3D11
//      or D3D9 path. Singularity bridges D3D9 -> D3D11 for XR
//      submission; no D3D12 device exists in the pipeline.
//   3. nvngx_dlssnr.dll validates the return address of calls into it
//      and refuses unrecognized callers (0xBAD00002). The reference
//      works around this with a separate caller shim DLL built /Od /MT.
//   4. DLSS-NR requires color, depth, motion vectors, normals, albedo,
//      roughness, exposure, current/previous matrices and jitter.
//      Singularity's UE3 D3D9 pipeline produces color only.
//
// What this module provides is the capability layer: a safe, one-time
// probe that inspects any NGX runtime DLLs present on the machine
// (reading their PE headers, never loading them into this process) and
// reports exactly why the backend cannot engage. If NVIDIA ever ships
// an x86 NGX with D3D11 entry points, the probe will say so and a real
// backend can be grown behind this interface.
// =============================================================================

#include <cstdint>

struct XrPostprocessDlssStatus {
    enum class Reason : int {
        Available = 0,      // x86 NGX + D3D11 entry points present (never seen yet)
        NotPresent = 1,     // no NGX runtime DLLs found on this machine
        X64Only = 2,        // DLL present but PE machine type is x64
        D3D12Only = 3,      // (static fact) NGX DLSS-NR entry points are D3D12-only
        MissingInputs = 4,  // (static fact) motion/normal/albedo/roughness/exposure absent
    };

    Reason reason = Reason::NotPresent;
    uint32_t dllsFound = 0;                 // NGX DLLs inspected
    uint32_t machineCodes[4] = {};          // PE machine codes seen (0x014c = i386, 0x8664 = x64)
    bool entryPointsAreD3D12Only = true;    // static fact about NGX DLSS-NR
    bool requiredInputsAvailable = false;   // static fact about the UE3 D3D9 pipeline
};

// One-time, side-effect-free probe. Reads PE headers from disk only.
// Never LoadLibrary's anything, so a 32-bit process cannot accidentally
// pull in an x64 runtime.
XrPostprocessDlssStatus XrPostprocessDlss_Probe();

// Human-readable reason (stable ASCII, for logs).
const char* XrPostprocessDlss_ReasonText(XrPostprocessDlssStatus::Reason r);

// Future real backend entry points. Both are honest stubs today:
// Init always refuses (returns false) so the caller stays on the
// native path; Shutdown is a no-op.
bool XrPostprocessDlss_Init(void* /*d3d11Device*/);
void XrPostprocessDlss_Shutdown();
