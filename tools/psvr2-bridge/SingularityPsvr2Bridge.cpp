#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <sddl.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#include "../../spikes/view_matrix/psvr2_ipc.h"

namespace {

struct Security {
    SECURITY_ATTRIBUTES attributes = {sizeof(SECURITY_ATTRIBUTES), nullptr, FALSE};
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    ~Security() {
        if (descriptor) LocalFree(descriptor);
    }
};

bool CurrentUserSecurity(Security& security) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
    DWORD needed = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &needed);
    BYTE buffer[256] = {};
    if (needed > sizeof(buffer) || !GetTokenInformation(token, TokenUser, buffer, needed, &needed)) {
        CloseHandle(token);
        return false;
    }
    CloseHandle(token);
    LPSTR sid = nullptr;
    const auto* user = reinterpret_cast<const TOKEN_USER*>(buffer);
    if (!ConvertSidToStringSidA(user->User.Sid, &sid)) return false;
    char sddl[320] = {};
    std::snprintf(sddl, sizeof(sddl), "D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;%s)", sid);
    LocalFree(sid);
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(
            sddl, SDDL_REVISION_1, &security.descriptor, nullptr))
        return false;
    security.attributes.lpSecurityDescriptor = security.descriptor;
    return true;
}

class RotatingLogger {
public:
    RotatingLogger() {
        InitializeCriticalSection(&lock_);
        wchar_t tempDir[MAX_PATH] = {};
        const DWORD len = GetTempPathW(MAX_PATH, tempDir);
        if (len > 0 && len < MAX_PATH) {
            swprintf_s(path_, L"%sSingularityVR-psvr2-toolkit.log", tempDir);
            swprintf_s(oldPath_, L"%sSingularityVR-psvr2-toolkit.old.log", tempDir);
        } else {
            wcscpy_s(path_, L"SingularityVR-psvr2-toolkit.log");
            wcscpy_s(oldPath_, L"SingularityVR-psvr2-toolkit.old.log");
        }
    }
    ~RotatingLogger() {
        DeleteCriticalSection(&lock_);
    }

    void Write(const char* category, const char* fmt, ...) {
        char body[512] = {};
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(body, sizeof(body), fmt, args);
        va_end(args);

        SYSTEMTIME st{};
        GetLocalTime(&st);
        char line[640] = {};
        const int lineLen = std::snprintf(
            line, sizeof(line),
            "%04u-%02u-%02u %02u:%02u:%02u.%03u [pid=%lu] [%s] %s\r\n",
            (unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay,
            (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond,
            (unsigned)st.wMilliseconds, (unsigned long)GetCurrentProcessId(),
            category, body);
        if (lineLen <= 0) return;

        std::fwrite(line, 1, (size_t)lineLen, stderr);
        std::fflush(stderr);

        EnterCriticalSection(&lock_);
        RotateIfNeededLocked((DWORD)lineLen);
        HANDLE file = CreateFileW(
            path_, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            WriteFile(file, line, (DWORD)lineLen, &written, nullptr);
            CloseHandle(file);
        }
        LeaveCriticalSection(&lock_);
    }

private:
    static constexpr ULONGLONG kMaxLogBytes = 256ull * 1024ull;

    void RotateIfNeededLocked(DWORD incomingBytes) {
        WIN32_FILE_ATTRIBUTE_DATA data{};
        if (!GetFileAttributesExW(path_, GetFileExInfoStandard, &data)) return;
        const ULONGLONG currentBytes =
            (static_cast<ULONGLONG>(data.nFileSizeHigh) << 32u) |
            static_cast<ULONGLONG>(data.nFileSizeLow);
        if (currentBytes + incomingBytes <= kMaxLogBytes) return;
        MoveFileExW(path_, oldPath_, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }

    CRITICAL_SECTION lock_{};
    wchar_t path_[MAX_PATH] = {};
    wchar_t oldPath_[MAX_PATH] = {};
};

RotatingLogger& Log() {
    static RotatingLogger instance;
    return instance;
}

static HANDLE g_shutdownEvent = nullptr;

BOOL WINAPI ConsoleCtrlHandler(DWORD) {
    if (g_shutdownEvent) SetEvent(g_shutdownEvent);
    return TRUE;
}

enum class WaitOutcome { IoReady, ParentExited, ShutdownRequested, Error };

WaitOutcome WaitForOverlappedOrExit(HANDLE ioHandle, OVERLAPPED& ov, HANDLE parentWatch, DWORD& transferred) {
    transferred = 0;
    HANDLE handles[3] = {ov.hEvent, nullptr, nullptr};
    DWORD count = 1;
    DWORD parentIdx = 0xFFFFFFFFu;
    DWORD shutdownIdx = 0xFFFFFFFFu;
    if (parentWatch) {
        parentIdx = count;
        handles[count++] = parentWatch;
    }
    if (g_shutdownEvent) {
        shutdownIdx = count;
        handles[count++] = g_shutdownEvent;
    }
    const DWORD waitResult = WaitForMultipleObjects(count, handles, FALSE, INFINITE);
    if (waitResult == WAIT_OBJECT_0) {
        if (!GetOverlappedResult(ioHandle, &ov, &transferred, FALSE)) {
            return WaitOutcome::Error;
        }
        return WaitOutcome::IoReady;
    }
    CancelIoEx(ioHandle, &ov);
    DWORD ignored = 0;
    GetOverlappedResult(ioHandle, &ov, &ignored, TRUE);
    if (parentIdx != 0xFFFFFFFFu && waitResult == WAIT_OBJECT_0 + parentIdx) {
        return WaitOutcome::ParentExited;
    }
    if (shutdownIdx != 0xFFFFFFFFu && waitResult == WAIT_OBJECT_0 + shutdownIdx) {
        return WaitOutcome::ShutdownRequested;
    }
    return WaitOutcome::Error;
}

// Minimal ABI declarations for the public PSVR2 Toolkit CAPI exports.
// Field sizes/layout match the installed CAPI contract; no toolkit source is vendored.
enum class Controller : std::uint8_t { Left = 0, Right = 1, Both = 2 };
struct TriggerCommand {
    std::int32_t mode;
    std::uint8_t padding[4];
    union {
        std::uint8_t raw[48];
        struct { std::uint8_t position, strength, padding[46]; } feedback;
        struct { std::uint8_t start, end, strength, padding[45]; } weapon;
        struct { std::uint8_t position, amplitude, frequency, padding[45]; } vibration;
        struct { std::uint8_t start, end, startStrength, endStrength, padding[44]; } slope;
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

    static void* SafeGetModuleHandle(GetModuleHandleFn fn) {
        __try { return fn(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    static bool SafeActive(DriverActiveFn fn) {
        __try { return fn(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static int SafeInit(InitFn fn) {
        __try { return fn(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return -4; }
    }
    static void SafeDeinit(DeinitFn fn) {
        __try { fn(); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    static int SafeSet(SetTriggerFn fn, Controller hand, const TriggerCommand& cmd) {
        __try { return fn(hand, cmd); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return -4; }
    }

    bool Load(const wchar_t* path) {
        loader = LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!loader) return false;
        auto get = reinterpret_cast<GetModuleHandleFn>(GetProcAddress(loader, "psvr2_toolkit_loader_get_module_handle"));
        if (get) capi = static_cast<HMODULE>(SafeGetModuleHandle(get));
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
        if (!init) return false;
        if (initialized) return true;
        const int result = SafeInit(init);
        initialized = (result == 0);
        return initialized;
    }
    void Off(Controller hand) {
        if (!initialized) Initialize();
        if (!initialized || !set) return;
        TriggerCommand c{};
        c.mode = 0;
        SafeSet(set, hand, c);
    }
    bool Apply(const psvr2ipc::EffectDesc& e) {
        if (!initialized) Initialize();
        if (!initialized || !set || e.effect > 4) return false;
        if (active && !SafeActive(active)) return false;
        TriggerCommand c{};
        // EffectDesc: 0=Off, 1=Weapon, 2=Vibration, 3=Feedback, 4=SlopeFeedback
        // Toolkit CAPI: 0=OFF, 1=FEEDBACK, 2=WEAPON, 3=VIBRATION, 5=SLOPE_FEEDBACK
        if (e.effect == 0) {
            c.mode = 0;
        } else if (e.effect == 1) {
            c.mode = 2;
            c.data.weapon.start = e.start;
            c.data.weapon.end = e.end;
            c.data.weapon.strength = e.strength;
        } else if (e.effect == 2) {
            c.mode = 3;
            c.data.vibration.position = e.position;
            c.data.vibration.amplitude = e.amplitude;
            c.data.vibration.frequency = e.frequency;
        } else if (e.effect == 3) {
            c.mode = 1;
            c.data.feedback.position = e.position;
            c.data.feedback.strength = e.strength;
        } else if (e.effect == 4) {
            c.mode = 5;
            c.data.slope.start = e.start;
            c.data.slope.end = e.end;
            c.data.slope.startStrength = e.startStrength;
            c.data.slope.endStrength = e.endStrength;
        }
        // Wire uses Right=0, Left=1; Toolkit enum uses Left=0, Right=1.
        const Controller hand = e.hand == 0 ? Controller::Right : Controller::Left;
        return SafeSet(set, hand, c) == 0;
    }
    void Shutdown() {
        if (initialized) {
            Off(Controller::Left);
            Off(Controller::Right);
            SafeDeinit(deinit);
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

static DWORD ParseParentPid(int argc, wchar_t** argv) {
    for (int i = 3; i + 1 < argc; ++i) {
        if (wcscmp(argv[i], L"--parent") == 0) {
            const unsigned long v = wcstoul(argv[i + 1], nullptr, 10);
            if (v > 0) return (DWORD)v;
        }
    }
    wchar_t env[64] = {};
    const DWORD n = GetEnvironmentVariableW(L"SINGULARITY_PSVR2_PARENT_PID", env, 64);
    if (n > 0 && n < 64) {
        const unsigned long v = wcstoul(env, nullptr, 10);
        if (v > 0) return (DWORD)v;
    }
    return 0;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc < 3) {
        Log().Write("bridge", "usage: SingularityPsvr2Bridge.exe <loader-path> <token-or-probe> [--hardware|--probe-capi] [--parent <pid>] [--once]");
        return 2;
    }
    if (wcscmp(argv[2], L"--probe-capi") == 0) {
        Backend probe;
        if (!probe.Load(argv[1])) {
            Log().Write("bridge", "FAIL: loader or required CAPI export unavailable");
            return 4;
        }
        const int initRes = Backend::SafeInit(probe.init);
        const bool activeRes = Backend::SafeActive(probe.active);
        Log().Write("bridge", "PASS: x64 loader loaded; SafeInit()=%d SafeActive()=%d", initRes, activeRes ? 1 : 0);
        if (initRes == 0 && probe.deinit) Backend::SafeDeinit(probe.deinit);
        return 0;
    }
    std::uint8_t token[psvr2ipc::kMaxTokenBytes]{};
    std::size_t tokenLen = 0;
    if (!AsciiToken(argv[2], token, tokenLen)) return 3;

    bool hardware = false;
    bool once = false;
    for (int i = 3; i < argc; ++i) {
        if (wcscmp(argv[i], L"--hardware") == 0) hardware = true;
        else if (wcscmp(argv[i], L"--once") == 0) once = true;
    }

    g_shutdownEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    const DWORD parentPid = ParseParentPid(argc, argv);
    HANDLE parentWatch = nullptr;
    if (parentPid != 0) {
        parentWatch = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
        if (parentWatch) {
            Log().Write("bridge", "Watching parent process pid=%lu for auto-exit", (unsigned long)parentPid);
        } else {
            Log().Write("bridge", "OpenProcess(SYNCHRONIZE) failed for parent pid=%lu err=%lu",
                        (unsigned long)parentPid, (unsigned long)GetLastError());
        }
    }

    Security security;
    if (!CurrentUserSecurity(security)) {
        Log().Write("bridge", "CurrentUserSecurity failed err=%lu", (unsigned long)GetLastError());
        if (parentWatch) CloseHandle(parentWatch);
        if (g_shutdownEvent) CloseHandle(g_shutdownEvent);
        return 6;
    }

    Backend backend;
    if (hardware) {
        if (!backend.Load(argv[1])) {
            Log().Write("bridge", "CAPI load/export failure");
            if (parentWatch) CloseHandle(parentWatch);
            if (g_shutdownEvent) CloseHandle(g_shutdownEvent);
            return 4;
        }
        if (backend.Initialize()) {
            const bool driverActive = backend.active ? Backend::SafeActive(backend.active) : false;
            Log().Write("bridge", "PSVR2 CAPI ready (init=0 driverActive=%d); authenticated pipe active", driverActive ? 1 : 0);
        } else {
            Log().Write("bridge", "PSVR2 CAPI init deferred; authenticated pipe active (will retry on demand)");
        }
    } else {
        Log().Write("bridge", "dry bridge active; hardware calls disabled");
    }

    bool keepServing = true;
    while (keepServing) {
        if (parentWatch && WaitForSingleObject(parentWatch, 0) == WAIT_OBJECT_0) {
            Log().Write("bridge", "Parent process pid=%lu exited before accept; shutting down", (unsigned long)parentPid);
            break;
        }
        if (g_shutdownEvent && WaitForSingleObject(g_shutdownEvent, 0) == WAIT_OBJECT_0) {
            Log().Write("bridge", "Shutdown signal received; exiting");
            break;
        }

        HANDLE pipe = CreateNamedPipeW(
            L"\\\\.\\pipe\\SingularityPsvr2",
            PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
            1, 0, (DWORD)psvr2ipc::kMaxMessageBytes * 16, 1000, &security.attributes);
        if (pipe == INVALID_HANDLE_VALUE) {
            Log().Write("bridge", "CreateNamedPipeW failed err=%lu", (unsigned long)GetLastError());
            if (parentWatch) CloseHandle(parentWatch);
            if (g_shutdownEvent) CloseHandle(g_shutdownEvent);
            return 6;
        }

        OVERLAPPED connectOv{};
        connectOv.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        bool connected = false;
        if (ConnectNamedPipe(pipe, &connectOv)) {
            connected = true;
        } else {
            const DWORD err = GetLastError();
            if (err == ERROR_PIPE_CONNECTED) {
                connected = true;
            } else if (err == ERROR_IO_PENDING) {
                DWORD ignored = 0;
                const WaitOutcome out = WaitForOverlappedOrExit(pipe, connectOv, parentWatch, ignored);
                if (out == WaitOutcome::IoReady) {
                    connected = true;
                } else {
                    if (out == WaitOutcome::ParentExited)
                        Log().Write("bridge", "Parent process pid=%lu exited while waiting for client; shutting down",
                                    (unsigned long)parentPid);
                    keepServing = false;
                }
            }
        }
        CloseHandle(connectOv.hEvent);

        if (connected) {
            Log().Write("bridge", "Client connected");
            psvr2ipc::ServerSession client(token, tokenLen);
            std::uint8_t message[psvr2ipc::kMaxMessageBytes]{};
            bool done = false;
            while (!done && keepServing) {
                OVERLAPPED readOv{};
                readOv.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                DWORD got = 0;
                bool readOk = false;
                if (ReadFile(pipe, message, (DWORD)sizeof(message), &got, &readOv)) {
                    readOk = true;
                } else {
                    const DWORD err = GetLastError();
                    if (err == ERROR_IO_PENDING) {
                        const WaitOutcome out = WaitForOverlappedOrExit(pipe, readOv, parentWatch, got);
                        if (out == WaitOutcome::IoReady) {
                            readOk = true;
                        } else {
                            if (out == WaitOutcome::ParentExited)
                                Log().Write("bridge", "Parent process pid=%lu exited during active session; shutting down",
                                            (unsigned long)parentPid);
                            keepServing = false;
                        }
                    }
                }
                CloseHandle(readOv.hEvent);
                if (!readOk || !got) break;

                psvr2ipc::Decoded decoded;
                const int accepted = client.Accept(message, got, decoded);
                if (accepted != 0) {
                    Log().Write("trigger", "rejected message reason=%d bytes=%lu", accepted, (unsigned long)got);
                    break;
                }
                if (decoded.type == psvr2ipc::MsgType::Hello) {
                    Log().Write("bridge", "Client authenticated (seq=%u)", decoded.sequence);
                } else if (decoded.type == psvr2ipc::MsgType::TriggerState) {
                    const bool applied = hardware ? backend.Apply(decoded.fx) : false;
                    Log().Write("trigger",
                                "seq=%u effect=%u hand=%u start=%u end=%u strength=%u pos=%u amp=%u freq=%u hw=%d",
                                decoded.sequence, (unsigned)decoded.fx.effect, (unsigned)decoded.fx.hand,
                                (unsigned)decoded.fx.start, (unsigned)decoded.fx.end, (unsigned)decoded.fx.strength,
                                (unsigned)decoded.fx.position, (unsigned)decoded.fx.amplitude,
                                (unsigned)decoded.fx.frequency, applied ? 1 : 0);
                } else if (decoded.type == psvr2ipc::MsgType::Shutdown) {
                    Log().Write("bridge", "Client sent Shutdown (seq=%u)", decoded.sequence);
                    done = true;
                }
            }
            if (hardware) {
                backend.Off(Controller::Left);
                backend.Off(Controller::Right);
            }
            FlushFileBuffers(pipe);
            DisconnectNamedPipe(pipe);
            Log().Write("bridge", "Client disconnected; triggers reset to OFF");
        }
        CloseHandle(pipe);
        if (once) break;
    }

    backend.Shutdown();
    if (parentWatch) CloseHandle(parentWatch);
    if (g_shutdownEvent) CloseHandle(g_shutdownEvent);
    Log().Write("bridge", "Bridge exited cleanly");
    return 0;
}
