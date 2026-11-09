#pragma once
#include <Windows.h>
#include <string>

// The installer provisions the x64 client independently of Steam's library
// location. Keep explicit/legacy installations working; never search the CWD.
inline std::wstring ResolveToolkitLoader(const wchar_t* requested) {
    const auto file = [](const wchar_t* path) {
        const DWORD attributes = GetFileAttributesW(path);
        return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    };
    if (requested && file(requested)) return requested;
    wchar_t local[32768]{};
    const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, 32768);
    if (n && n < 32768) {
        std::wstring installed(local);
        installed += L"\\Trilex\\PSVR2Toolkit\\capi\\psvr2_toolkit_capi_loader.dll";
        if (file(installed.c_str())) return installed;
    }
    return requested ? requested : L"";
}
