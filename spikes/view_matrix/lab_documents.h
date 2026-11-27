#pragma once

// Private build for a clean campaign in the Steam lab. Only the game's
// Documents import changes. Windows and other processes retain their paths.
#include <shlobj.h>

namespace lab_documents {
using GetFolder = HRESULT (WINAPI*)(HWND, int, HANDLE, DWORD, LPWSTR);
static GetFolder original = nullptr;
static wchar_t scratch[MAX_PATH]{};
static volatile LONG redirects = 0;

static HRESULT WINAPI GetFolderPath(HWND owner, int id, HANDLE user, DWORD kind, LPWSTR path) {
    if ((id & ~CSIDL_FLAG_MASK) != CSIDL_PERSONAL || user != nullptr || kind != SHGFP_TYPE_CURRENT) {
        return original(owner, id, user, kind, path);
    }
    if (!path) return E_INVALIDARG;
    wcscpy_s(path, MAX_PATH, scratch);
    if (InterlockedIncrement(&redirects) <= 8) {
        Log("lab-documents: game request redirected csidl=%#x kind=%lu path=%ls", id, kind, path);
    }
    return S_OK;
}

static bool Install() {
    const DWORD length = GetTempPathW(MAX_PATH, scratch);
    if (!length || length >= MAX_PATH ||
        wcscat_s(scratch, L"Singularity-Quest2-20261127\\documents-lift-checkpoint") != 0) {
        return false;
    }
    const DWORD attributes = GetFileAttributesW(scratch);
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY) ||
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        return false;
    }

    auto* image = reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
    const auto entry = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!entry.VirtualAddress) return false;

    auto* imports = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(image + entry.VirtualAddress);
    for (; imports->Name; ++imports) {
        const auto* library = reinterpret_cast<const char*>(image + imports->Name);
        if (_stricmp(library, "shell32.dll") != 0 || !imports->OriginalFirstThunk) continue;
        auto* names = reinterpret_cast<const IMAGE_THUNK_DATA*>(image + imports->OriginalFirstThunk);
        auto* targets = reinterpret_cast<IMAGE_THUNK_DATA*>(image + imports->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++targets) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) continue;
            const auto* name = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(image + names->u1.AddressOfData);
            if (strcmp(reinterpret_cast<const char*>(name->Name), "SHGetFolderPathW") != 0) continue;
            auto* slot = &targets->u1.Function;
            original = reinterpret_cast<GetFolder>(*slot);
            if (!original || original != &SHGetFolderPathW) return false;
            DWORD oldProtection = 0;
            if (!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &oldProtection)) return false;
            *slot = reinterpret_cast<ULONG_PTR>(&GetFolderPath);
            DWORD ignored = 0;
            if (!VirtualProtect(slot, sizeof(*slot), oldProtection, &ignored)) return false;
            Log("lab-documents: installed private game import scratch=%ls", scratch);
            return true;
        }
    }
    return false;
}
}  // namespace lab_documents
