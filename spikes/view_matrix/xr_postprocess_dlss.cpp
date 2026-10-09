// =============================================================================
// DLSS-NR backend — capability probe implementation (research/nvidia-vr-upscaling)
//
// Reads PE headers of any NGX runtime DLLs found on the machine and reports
// whether an x86 NGX exists. Deliberately never loads the DLLs: a 32-bit
// process that LoadLibrary's an x64 DLL simply fails, and the probe must be
// able to inspect the machine without side effects.
// =============================================================================

#include "xr_postprocess_dlss.h"

#include <cstdio>
#include <cstring>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace {

// PE machine codes we care about.
constexpr uint32_t kMachineI386 = 0x014c;
constexpr uint32_t kMachineAmd64 = 0x8664;

// Read the PE machine type of a DLL on disk. Returns 0 if the file is not a
// PE image or cannot be read. Bounded: reads only the DOS header, then seeks
// to the PE header and reads the signature plus the COFF machine field.
uint32_t ReadPeMachine(const wchar_t* path) {
    HANDLE file = CreateFileW(path, GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    uint8_t dos[64] = {};
    DWORD bytesRead = 0;
    if (!ReadFile(file, dos, sizeof(dos), &bytesRead, nullptr) ||
        bytesRead < 64 || dos[0] != 'M' || dos[1] != 'Z') {
        CloseHandle(file);
        return 0;
    }
    const uint32_t peOffset = *reinterpret_cast<const uint32_t*>(dos + 0x3c);

    uint8_t pe[24] = {};
    DWORD peRead = 0;
    const LONG seek = SetFilePointer(file, static_cast<LONG>(peOffset),
                                     nullptr, FILE_BEGIN);
    if (peOffset == 0 || seek == INVALID_SET_FILE_POINTER ||
        !ReadFile(file, pe, sizeof(pe), &peRead, nullptr) || peRead < 24 ||
        pe[0] != 'P' || pe[1] != 'E' || pe[2] != 0 || pe[3] != 0) {
        CloseHandle(file);
        return 0;
    }
    CloseHandle(file);

    // COFF header starts 4 bytes after the signature; Machine is its first field.
    return *reinterpret_cast<const uint16_t*>(pe + 4);
}

// Candidate DLL names, in probe order.
const wchar_t* kDllNames[] = {
    L"_nvngx.dll",          // NGX core, shipped beside the layer / driver
    L"nvngx_dlssnr.dll",    // DLSS-NR runtime
};

// Search locations: exe dir, System32 (as seen by this process) and the real
// 64-bit System32 (sysnative alias under WoW64).
void CollectSearchDirs(wchar_t (*out)[MAX_PATH], uint32_t* count) {
    uint32_t n = 0;

    wchar_t exePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
        wchar_t* slash = wcsrchr(exePath, L'\\');
        if (slash) {
            *slash = L'\0';
            wcscpy_s(out[n++], MAX_PATH, exePath);
        }
    }

    wchar_t sysDir[MAX_PATH] = {};
    if (n < 4 && GetSystemDirectoryW(sysDir, MAX_PATH) > 0) {
        wcscpy_s(out[n++], MAX_PATH, sysDir);
    }

    wchar_t winDir[MAX_PATH] = {};
    if (n < 4 && GetWindowsDirectoryW(winDir, MAX_PATH) > 0) {
        wchar_t sysNative[MAX_PATH] = {};
        swprintf_s(sysNative, MAX_PATH, L"%s\\sysnative", winDir);
        wcscpy_s(out[n++], MAX_PATH, sysNative);
    }

    *count = n;
}

}  // namespace

XrPostprocessDlssStatus XrPostprocessDlss_Probe() {
    XrPostprocessDlssStatus status;

    wchar_t dirs[4][MAX_PATH] = {};
    uint32_t dirCount = 0;
    CollectSearchDirs(dirs, &dirCount);

    for (const wchar_t* dllName : kDllNames) {
        for (uint32_t d = 0; d < dirCount && status.dllsFound < 4; ++d) {
            wchar_t candidate[MAX_PATH] = {};
            swprintf_s(candidate, MAX_PATH, L"%s\\%s", dirs[d], dllName);
            const uint32_t machine = ReadPeMachine(candidate);
            if (machine == 0) {
                continue;
            }
            status.machineCodes[status.dllsFound++] = machine;

            if (machine == kMachineI386) {
                // An x86 NGX would be news. It still cannot engage:
                // NGX DLSS-NR entry points are D3D12-only and the
                // UE3 pipeline cannot supply motion vectors, normals,
                // albedo, roughness or exposure.
                status.reason = XrPostprocessDlssStatus::Reason::D3D12Only;
            } else if (status.reason != XrPostprocessDlssStatus::Reason::D3D12Only) {
                status.reason = XrPostprocessDlssStatus::Reason::X64Only;
            }
        }
    }

    if (status.dllsFound == 0) {
        status.reason = XrPostprocessDlssStatus::Reason::NotPresent;
    }

    // Static facts, independent of what is installed on this machine.
    status.entryPointsAreD3D12Only = true;
    status.requiredInputsAvailable = false;

    return status;
}

const char* XrPostprocessDlss_ReasonText(XrPostprocessDlssStatus::Reason r) {
    switch (r) {
        case XrPostprocessDlssStatus::Reason::Available:
            return "available";
        case XrPostprocessDlssStatus::Reason::NotPresent:
            return "NGX runtime DLLs not present on this machine";
        case XrPostprocessDlssStatus::Reason::X64Only:
            return "NGX runtime is x64-only (import libs in lib\\x64); cannot link/load from a 32-bit process";
        case XrPostprocessDlssStatus::Reason::D3D12Only:
            return "NGX DLSS-NR entry points are D3D12-only (NVSDK_NGX_D3D12_*); no D3D11/D3D9 path";
        case XrPostprocessDlssStatus::Reason::MissingInputs:
            return "required inputs unavailable (motion vectors, normals, albedo, roughness, exposure)";
    }
    return "unknown";
}

bool XrPostprocessDlss_Init(void* /*d3d11Device*/) {
    // Honest stub: no x86 NGX exists, so there is nothing to initialize.
    // Returns false so the caller stays on the native path.
    return false;
}

void XrPostprocessDlss_Shutdown() {
    // Nothing to release; the probe never created resources.
}
