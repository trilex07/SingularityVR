// SingularityVR.PSVR2Bridge — x64 helper process (clean-room).
//
// Architecture (KHARVOX-proven, see docs/REFERENCE_VR_IMPLEMENTATIONS.md):
//   x86 d3d9.dll (psvr2_client.h, write-only pipe)
//     -> named pipe \\.\pipe\SingularityPsvr2_<pid>_<tag>
//     -> this x64 helper (pipe server, session auth, backend dispatch)
//     -> PSVR2Toolkit (ToolkitBackend, compiled only with PSVR2TK_AVAILABLE)
//     -> PSVR2 Sense adaptive trigger (RIGHT hand initially)
//
// Without PSVR2TK_AVAILABLE the bridge runs a DisabledBackend: it still
// authenticates the client, validates every message, dedupes effects, and
// logs what it WOULD apply — the full IPC path is testable with no
// hardware. With --log it also records applies for loopback verification.
//
// Usage:
//   SingularityVR.PSVR2Bridge.exe --pipe <name> --token <text> [--log <file>]
//                                 [--parent <pid>] [--timeout-ms <n>]
// Safety: current-user-only pipe (no remote clients), Hello-first auth with
// per-launch token, monotonic sequences, apply-on-change only, Off+clear on
// disconnect/shutdown/parent-exit/timeout. Exit codes: 0 clean, 1 usage or
// pipe error.
// Build (x64 REQUIRED — PSVR2Toolkit is x64-only):
//   cl /nologo /EHsc /W4 /std:c++17 bridge_main.cpp

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "trigger_backend.h"

namespace {
using namespace psvr2ipc;

inline void CopyBounded(char* dst, std::size_t cap, const char* src) {
    std::size_t n = std::strlen(src);
    if (n > cap - 1) n = cap - 1;
    std::memcpy(dst, src, n);
    dst[n] = '\0';
}

struct Options {
    char pipe[160] = {0};
    char tokenRaw[kMaxTokenBytes + 1] = {0};
    char logPath[260] = {0};
    DWORD parentPid = 0;
    DWORD timeoutMs = 0; // 0 = run until parent exit / Ctrl+C / Shutdown msg
};

bool ParseArgs(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--pipe") == 0 && i + 1 < argc) {
            CopyBounded(o.pipe, sizeof(o.pipe), argv[++i]);
        } else if (std::strcmp(argv[i], "--token") == 0 && i + 1 < argc) {
            CopyBounded(o.tokenRaw, sizeof(o.tokenRaw), argv[++i]);
        } else if (std::strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            CopyBounded(o.logPath, sizeof(o.logPath), argv[++i]);
        } else if (std::strcmp(argv[i], "--parent") == 0 && i + 1 < argc) {
            o.parentPid = (DWORD)std::strtoul(argv[++i], nullptr, 10);
        } else if (std::strcmp(argv[i], "--timeout-ms") == 0 && i + 1 < argc) {
            o.timeoutMs = (DWORD)std::strtoul(argv[++i], nullptr, 10);
        } else {
            return false;
        }
    }
    if (!PipeNameValid(o.pipe, std::strlen(o.pipe))) return false;
    if (!TokenValid((const std::uint8_t*)o.tokenRaw, std::strlen(o.tokenRaw))) return false;
    return true;
}

void LogLine(FILE* log, const char* fmt, const EffectDesc* fx) {
    if (!log) return;
    if (fx)
        std::fprintf(log, "%s effect=%u hand=%u start=%u end=%u str=%u pos=%u amp=%u freq=%u\n",
                     fmt, fx->effect, fx->hand, fx->start, fx->end, fx->strength, fx->position,
                     fx->amplitude, fx->frequency);
    else
        std::fprintf(log, "%s\n", fmt);
    std::fflush(log);
}

bool ReadExact(HANDLE h, std::uint8_t* buf, DWORD n) {
    DWORD got = 0;
    while (got < n) {
        DWORD r = 0;
        if (!ReadFile(h, buf + got, n - got, &r, nullptr) || r == 0) return false;
        got += r;
    }
    return true;
}

volatile bool g_ctrlStop = false;
BOOL WINAPI CtrlHandler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT || type == CTRL_CLOSE_EVENT) {
        g_ctrlStop = true;
        return TRUE;
    }
    return FALSE;
}
} // namespace

int main(int argc, char** argv) {
    Options o;
    if (!ParseArgs(argc, argv, o)) {
        std::puts("usage: SingularityVR.PSVR2Bridge.exe --pipe <name> --token <text> "
                  "[--log <file>] [--parent <pid>] [--timeout-ms <n>]");
        return 1;
    }
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    FILE* log = nullptr;
    if (o.logPath[0]) {
        FILE* f = nullptr;
        if (fopen_s(&f, o.logPath, "w") == 0) log = f;
    }
    LogLine(log, "bridge start", nullptr);

    HANDLE parent = nullptr;
    if (o.parentPid) {
        parent = OpenProcess(SYNCHRONIZE, FALSE, o.parentPid);
        if (!parent) LogLine(log, "warn: parent not watchable", nullptr);
    }

    wchar_t wide[160];
    MultiByteToWideChar(CP_UTF8, 0, o.pipe, -1, wide, 160);
    // Current-user-only, no remote clients (KHARVOX CurrentUserSecurity equivalent:
    // default DACL on a user session pipe + REJECT_REMOTE_CLIENTS).
    HANDLE pipe = CreateNamedPipeW(wide, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |
                                                                PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                   1, 0, 4096, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
        LogLine(log, "error: CreateNamedPipe failed", nullptr);
        if (log) std::fclose(log);
        return 1;
    }
    LogLine(log, "pipe listening", nullptr);

    // Backend lifetime spans the process: init once, clear per disconnect,
    // shutdown once at exit. Never re-enumerate per shot.
    psvr2bridge::DisabledBackend disabled;
    psvr2bridge::ITriggerBackend* toolkit = psvr2bridge::CreateToolkitBackend();
    psvr2bridge::ITriggerBackend* backend = &disabled;
    if (toolkit && toolkit->Init()) {
        backend = toolkit;
        LogLine(log, "backend: psvr2toolkit", nullptr);
    } else {
        if (toolkit) {
            char why[128] = {0};
            _snprintf_s(why, sizeof(why), _TRUNCATE, "toolkit init failed at: %s",
                        toolkit->Status());
            LogLine(log, why, nullptr);
            delete toolkit;
        }
        backend->Init();
        LogLine(log, "backend: disabled (log-only)", nullptr);
    }

    const DWORD startTick = GetTickCount();
    bool running = true;
    while (running && !g_ctrlStop) {
        if (o.timeoutMs && GetTickCount() - startTick > o.timeoutMs) {
            LogLine(log, "timeout", nullptr);
            break;
        }
        if (parent && WaitForSingleObject(parent, 0) == WAIT_OBJECT_0) {
            LogLine(log, "parent exited", nullptr);
            break;
        }
        BOOL connected =
            ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected) {
            if (g_ctrlStop) break;
            continue;
        }
        LogLine(log, "client connected", nullptr);

        ServerSession session((const std::uint8_t*)o.tokenRaw, std::strlen(o.tokenRaw));
        EffectDesc applied{};
        bool haveApplied = false;
        bool shutdownMsg = false;

        for (;;) {
            if (g_ctrlStop) break;
            if (parent && WaitForSingleObject(parent, 0) == WAIT_OBJECT_0) break;
            std::uint8_t msg[kMaxMessageBytes];
            if (!ReadExact(pipe, msg, kHeaderBytes)) break; // disconnect
            // Frame on the header BEFORE session validation: magic, version,
            // type and length checks first so corrupt streams drop fast.
            if (ReadU32(msg + 0) != kMagic || ReadU16(msg + 4) != kVersion) break;
            const std::uint8_t mtype = msg[6];
            if ((mtype < 1 || mtype > 3) || msg[7] != 0) break;
            const std::size_t payload = ReadU16(msg + 16);
            if (msg[18] != 0 || msg[19] != 0 || payload > kMaxTokenBytes) break;
            if (payload && !ReadExact(pipe, msg + kHeaderBytes, (DWORD)payload)) break;
            Decoded d;
            if (session.Accept(msg, kHeaderBytes + payload, d) != 0) break; // auth/sequence
            if (d.type == MsgType::TriggerState) {
                if (!(d.fx == applied) || !haveApplied) {
                    applied = d.fx;
                    haveApplied = true;
                    if (backend->IsDriverActive()) {
                        backend->Apply(d.fx);
                        LogLine(log, "apply(hw)", &d.fx);
                    } else {
                        LogLine(log, "apply(log-only, no driver)", &d.fx);
                    }
                }
            } else if (d.type == MsgType::Shutdown) {
                LogLine(log, "shutdown msg", nullptr);
                shutdownMsg = true;
                running = false;
                break;
            }
            // Hello produces no action beyond auth.
        }
        // Every disconnect/shutdown clears (hardware Off in ToolkitBackend).
        backend->Clear();
        LogLine(log, "clear on disconnect", nullptr);
        DisconnectNamedPipe(pipe);
        if (shutdownMsg) break;
    }

    LogLine(log, "bridge exit", nullptr);
    if (log) std::fclose(log);
    if (backend != &disabled) {
        backend->Shutdown();
        delete backend;
    }
    if (parent) CloseHandle(parent);
    CloseHandle(pipe);
    return 0;
}
