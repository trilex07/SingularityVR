#pragma once
#include <cfgmgr32.h>
#include "input_classification_cache.h"

InputClassificationCache g_inputClassificationCache;
BOOL (__cdecl *g_originalClassifyInput)(const GUID*) = nullptr;
HCMNOTIFICATION g_inputDeviceNotification = nullptr;

DWORD CALLBACK InputDeviceChanged(HCMNOTIFICATION, PVOID, CM_NOTIFY_ACTION,
                                  PCM_NOTIFY_EVENT_DATA, DWORD) {
    // Called on a Windows notification thread: no logging, allocation or WMI.
    g_inputClassificationCache.Invalidate();
    return ERROR_SUCCESS;
}
BOOL __cdecl Hook_ClassifyInputDevice(const GUID* product) {
    return g_inputClassificationCache.Query(product, g_originalClassifyInput);
}
void InstallInputClassificationCache() {
    static bool done = false;
    if (done) return;
    const auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE) return;
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        nt->OptionalHeader.SizeOfImage <= 0xDD1600) { done = true; return; }
    auto target = base + 0xDD12F0;
    // Verified against the read-only, unpacked runtime snapshot from capture
    // 20261006-205508. Refuse other revisions and retry while Steam is unpacking.
    const unsigned char prologue[] = {0x55,0x8B,0xEC,0x81,0xEC,0x8C,0,0,0,
                                     0x53,0x33,0xDB,0x6A,0x4C};
    if (memcmp(target, prologue, sizeof(prologue)) != 0 ||
        target[0x30F] != 0xC3) return;
    unsigned fingerprint = 2166136261u;
    for (unsigned i = 0; i < 0x310; ++i) fingerprint = (fingerprint ^ target[i]) * 16777619u;
    if (fingerprint != 0x9785AF65u) return;
    HMODULE cm = GetModuleHandleW(L"cfgmgr32.dll");
    if (!cm) cm = LoadLibraryW(L"cfgmgr32.dll");
    using Register = CONFIGRET (WINAPI*)(PCM_NOTIFY_FILTER,PVOID,PCM_NOTIFY_CALLBACK,PHCMNOTIFICATION);
    using Unregister = CONFIGRET (WINAPI*)(HCMNOTIFICATION);
    auto reg = cm ? reinterpret_cast<Register>(GetProcAddress(cm,"CM_Register_Notification")) : nullptr;
    auto unreg = cm ? reinterpret_cast<Unregister>(GetProcAddress(cm,"CM_Unregister_Notification")) : nullptr;
    done = true;
    if (!reg || !unreg) { Log("input-device cache unavailable: no topology notification API"); return; }
    CM_NOTIFY_FILTER filter{};
    filter.cbSize = sizeof(filter);
    filter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
    filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    if (reg(&filter,nullptr,InputDeviceChanged,&g_inputDeviceNotification) != CR_SUCCESS) {
        Log("input-device cache unavailable: topology notification registration failed"); return;
    }
    const MH_STATUS init = MH_Initialize();
    if ((init != MH_OK && init != MH_ERROR_ALREADY_INITIALIZED) ||
        MH_CreateHook(target,reinterpret_cast<void*>(Hook_ClassifyInputDevice),
                      reinterpret_cast<void**>(&g_originalClassifyInput)) != MH_OK ||
        MH_EnableHook(target) != MH_OK) {
        unreg(g_inputDeviceNotification); g_inputDeviceNotification = nullptr;
        Log("input-device cache hook failed; original classification retained"); return;
    }
    Log("input-device cache ACTIVE at Singularity.exe+0xDD12F0; original cold classification, invalidate on device change");
}
void ReportInputClassificationCache() {
    static unsigned frames = 0;
    if (!g_originalClassifyInput || ++frames < 600) return;
    frames = 0;
    Log("InputDeviceCache hits=%ld coldWmiCalls=%ld (cumulative)",
        InterlockedCompareExchange(&g_inputClassificationCache.hits,0,0),
        InterlockedCompareExchange(&g_inputClassificationCache.misses,0,0));
}
