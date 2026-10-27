#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "../../spikes/view_matrix/psvr2_ipc.h"

// Minimal ABI declarations for the public PSVR2 Toolkit CAPI exports.
// Field sizes/layout match the installed CAPI contract; no toolkit source is vendored.
enum class Controller : std::uint8_t { Left = 0, Right = 1, Both = 2 };
struct TriggerCommand {
    std::int32_t mode;
    std::uint8_t padding[4];
    union {
        std::uint8_t raw[48];
        struct { std::uint8_t start, end, strength, padding[45]; } weapon;
        struct { std::uint8_t position, amplitude, frequency, padding[45]; } vibration;
    } data;
};
static_assert(sizeof(TriggerCommand) == 56, "CAPI trigger command ABI mismatch");
using GetModuleHandleFn = void* (*)();
using InitFn = int (*)();
using DeinitFn = void (*)();
using DriverActiveFn = bool (*)();
using SetTriggerFn = int (*)(Controller, const TriggerCommand&);

struct Backend {
    HMODULE loader = nullptr, capi = nullptr;
    InitFn init = nullptr;
    DeinitFn deinit = nullptr;
    DriverActiveFn active = nullptr;
    SetTriggerFn set = nullptr;
    bool initialized = false;

    bool Load(const wchar_t* path) {
        loader = LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!loader) return false;
        auto get = reinterpret_cast<GetModuleHandleFn>(GetProcAddress(loader, "psvr2_toolkit_loader_get_module_handle"));
        if (get) capi = static_cast<HMODULE>(get());
        // Current Toolkit builds also export CAPI directly from the loader DLL;
        // use that surface when the optional internal module handle is null.
        if (!capi) capi = loader;
        init = reinterpret_cast<InitFn>(GetProcAddress(capi, "psvr2_toolkit_init"));
        deinit = reinterpret_cast<DeinitFn>(GetProcAddress(capi, "psvr2_toolkit_deinit"));
        active = reinterpret_cast<DriverActiveFn>(GetProcAddress(capi, "psvr2_toolkit_get_driver_active"));
        set = reinterpret_cast<SetTriggerFn>(GetProcAddress(capi, "psvr2_toolkit_set_trigger_effect"));
        return init && deinit && active && set;
    }
    bool Initialize() {
        if (!active || !init) return false;
        bool driverActive = false;
        int result = -4;
        __try {
            driverActive = active();
            if (driverActive) result = init();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            driverActive = false;
        }
        initialized = driverActive && result == 0;
        return initialized;
    }
    void Off(Controller hand) {
        if (!initialized || !set) return;
        TriggerCommand c{}; c.mode = 0;
        __try { set(hand, c); } __except (EXCEPTION_EXECUTE_HANDLER) { }
    }
    bool Apply(const psvr2ipc::EffectDesc& e) {
        if (!initialized || !set || e.effect > 2) return false;
        TriggerCommand c{};
        c.mode = e.effect == 0 ? 0 : (e.effect == 1 ? 2 : 3); // Toolkit OFF / WEAPON / VIBRATION
        if (e.effect == 1) {
            c.data.weapon.start = e.start; c.data.weapon.end = e.end;
            c.data.weapon.strength = e.strength;
        } else if (e.effect == 2) {
            c.data.vibration.position = e.position;
            c.data.vibration.amplitude = e.amplitude;
            c.data.vibration.frequency = e.frequency;
        }
        // Wire uses Right=0, Left=1; Toolkit enum uses Left=0, Right=1.
        const Controller hand = e.hand == 0 ? Controller::Right : Controller::Left;
        int result = -4;
        __try { result = set(hand, c); } __except (EXCEPTION_EXECUTE_HANDLER) { }
        return result == 0;
    }
    void Shutdown() {
        if (initialized) {
            Off(Controller::Left); Off(Controller::Right);
            __try { deinit(); } __except (EXCEPTION_EXECUTE_HANDLER) { }
            initialized = false;
        }
        if (loader) FreeLibrary(loader);
        loader = capi = nullptr;
    }
    ~Backend() { Shutdown(); }
};

static bool AsciiToken(const wchar_t* arg, std::uint8_t out[psvr2ipc::kMaxTokenBytes], std::size_t& n) {
    n = wcslen(arg);
    if (n < 16 || n > psvr2ipc::kMaxTokenBytes) return false;
    for (std::size_t i = 0; i < n; ++i) {
        if (arg[i] > 0x7f) return false;
        out[i] = (std::uint8_t)arg[i];
    }
    return psvr2ipc::TokenValid(out, n);
}

int wmain(int argc, wchar_t** argv) {
    if (argc < 3) {
        fwprintf(stderr, L"usage: SingularityPsvr2Bridge.exe <loader-path> <token-or-probe> [--hardware|--probe-capi]\n");
        return 2;
    }
    if (wcscmp(argv[2], L"--probe-capi") == 0) {
        Backend probe;
        if (!probe.Load(argv[1])) { fwprintf(stderr, L"FAIL: loader or required CAPI export unavailable\n"); return 4; }
        fwprintf(stderr, L"PASS: x64 loader loaded; init/deinit/driver-active/set-trigger-effect exports resolved (not initialized)\n");
        return 0;
    }
    std::uint8_t token[psvr2ipc::kMaxTokenBytes]{};
    std::size_t tokenLen = 0;
    if (!AsciiToken(argv[2], token, tokenLen)) return 3;
    const bool hardware = argc > 3 && wcscmp(argv[3], L"--hardware") == 0;
    Backend backend;
    if (hardware) {
        if (!backend.Load(argv[1])) { fwprintf(stderr, L"CAPI load/export failure\n"); return 4; }
        if (!backend.Initialize()) { fwprintf(stderr, L"PSVR2 driver inactive or CAPI init failed\n"); return 5; }
        fwprintf(stderr, L"PSVR2 CAPI ready; authenticated pipe active\n");
    } else {
        fwprintf(stderr, L"dry bridge active; hardware calls disabled\n");
    }

    for (;;) {
        HANDLE pipe = CreateNamedPipeW(L"\\\\.\\pipe\\SingularityPsvr2",
            PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 0, (DWORD)psvr2ipc::kMaxMessageBytes, 1000, nullptr);
        if (pipe == INVALID_HANDLE_VALUE) return 6;
        const BOOL connected = ConnectNamedPipe(pipe, nullptr) ? TRUE : GetLastError() == ERROR_PIPE_CONNECTED;
        if (!connected) { CloseHandle(pipe); continue; }
        psvr2ipc::ServerSession client(token, tokenLen);
        std::uint8_t message[psvr2ipc::kMaxMessageBytes]{};
        bool done = false;
        while (!done) {
            DWORD got = 0;
            if (!ReadFile(pipe, message, (DWORD)sizeof(message), &got, nullptr) || !got) break;
            psvr2ipc::Decoded decoded;
            const int accepted = client.Accept(message, got, decoded);
            if (accepted != 0) break;
            if (decoded.type == psvr2ipc::MsgType::TriggerState) {
                if (hardware) backend.Apply(decoded.fx);
            } else if (decoded.type == psvr2ipc::MsgType::Shutdown) done = true;
        }
        if (hardware) { backend.Off(Controller::Left); backend.Off(Controller::Right); }
        FlushFileBuffers(pipe); DisconnectNamedPipe(pipe); CloseHandle(pipe);
    }
}
