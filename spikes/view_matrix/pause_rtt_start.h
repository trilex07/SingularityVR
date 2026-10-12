#pragma once

namespace PauseRttStart {
using StartFn = void (__thiscall*)(void*, void*, void*, int, int);
static StartFn original = nullptr;
static unsigned reports = 0;
static bool attempted = false;

static uintptr_t ResolvePauseTarget(uintptr_t record, uintptr_t incoming) {
    const uintptr_t movie = PauseRttProbe::Pointer(record + 0x50);
    const uintptr_t cls = PauseRttProbe::Pointer(movie + OBJ_CLASS);
    char className[96] = {};
    if (!movie || !cls || !NameOf(cls, className, sizeof(className)) ||
        strcmp(className, "RvUIGFxMovie_Pause")) return incoming;
    const int movieOffset = PauseRttProbe::Property(cls, "pMovie", "StructProperty");
    const int textureOffset = PauseRttProbe::Property(cls, "RenderTexture", "ObjectProperty");
    const uintptr_t configured = textureOffset >= 0 ? PauseRttProbe::Pointer(movie + textureOffset) : 0;
    const uintptr_t resource = configured ? PauseRttProbe::Pointer(configured + 0xA4) : 0;
    char targetName[96] = {};
    if (configured) NameOf(configured, targetName, sizeof(targetName));
    const bool valid = movieOffset >= 0 && PauseRttProbe::Pointer(movie + movieOffset) == record &&
        configured && !strcmp(targetName, "RTT_VRPause") && resource &&
        PauseRttProbe::NativeResourceContract(configured, resource);
    // Live pause already receives the dedicated target: keep this read-only.
    const uintptr_t selected = incoming;
    if (reports++ < 12)
        Log("pause-rtt native-start movie=%p record=%p incoming=%p configured=%p selected=%p repair=%d prerequisites=%d",
            (void*)movie, (void*)record, (void*)incoming, (void*)configured, (void*)selected,
            selected != incoming ? 1 : 0, valid ? 1 : 0);
    return selected;
}

static void __fastcall Hook(void* engine, void*, void* record, void* target, int active, int focus) {
    uintptr_t selected = (uintptr_t)target;
    __try { selected = ResolvePauseTarget((uintptr_t)record, selected); }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                  ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        if (reports++ < 12) Log("pause-rtt native-start UNKNOWN: prerequisites inaccessible; retaining incoming target");
    }
    original(engine, record, (void*)selected, active, focus);
}

static void Install() {
    if (attempted) return;
    // Steam may unpack text after proxy attach. Install at the first Present,
    // before menu initialization, using a unique whole-entry signature.
    static const unsigned char signature[] = {
        0x55,0x8B,0xEC,0x8B,0x45,0x10,0x8B,0x55,0x0C,0x83,0xEC,0x64,
        0x53,0x56,0x8B,0x75,0x08,0xC7,0x46,0x3C,0x01,0,0,0,
        0x57,0x8B,0xD9,0x8B,0x4D,0x14,0x89,0x46,0x40,0x33,0xFF,
        0x89,0x4E,0x44,0x89,0x56,0x54};
    const uintptr_t image = (uintptr_t)GetModuleHandleA(nullptr);
    uintptr_t address = image, found = 0;
    unsigned matches = 0;
    for (unsigned regions = 0; regions < 512 && address - image < 0x4000000; ++regions) {
        MEMORY_BASIC_INFORMATION info = {};
        if (!VirtualQuery((void*)address, &info, sizeof(info)) || !info.RegionSize) break;
        if ((uintptr_t)info.AllocationBase != image) break;
        const DWORD protection = info.Protect & 0xFF;
        if (info.State == MEM_COMMIT && !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
            (protection == PAGE_EXECUTE_READ || protection == PAGE_EXECUTE_READWRITE ||
             protection == PAGE_EXECUTE_WRITECOPY)) {
            const unsigned char* bytes = (const unsigned char*)info.BaseAddress;
            for (size_t i = 0; i + sizeof(signature) <= info.RegionSize; ++i) {
                if (bytes[i] == signature[0] && memcmp(bytes+i, signature, sizeof(signature)) == 0) {
                    found = (uintptr_t)(bytes+i); ++matches;
                }
            }
        }
        address = (uintptr_t)info.BaseAddress + info.RegionSize;
    }
    attempted = true;
    if (matches != 1) { Log("pause-rtt native-start hook refused signatureMatches=%u", matches); return; }
    const MH_STATUS create = MH_CreateHook((void*)found, (void*)&Hook, (void**)&original);
    const MH_STATUS enable = create == MH_OK ? MH_EnableHook((void*)found) : create;
    Log("pause-rtt native-start hook target=%p create=%d enable=%d", (void*)found, (int)create, (int)enable);
    if (create == MH_OK && enable != MH_OK) { MH_RemoveHook((void*)found); original = nullptr; }
}
} // namespace PauseRttStart
