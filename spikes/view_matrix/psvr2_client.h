#pragma once
#include "psvr2_ipc.h"
#include "psvr2_trigger.h"

// x86 game-side adaptive-trigger client (clean-room).
//
// KHARVOX reference: src/psvr2/Psvr2IpcClient.cpp — worker thread, write-only
// overlapped pipe, packed-uint64 generation dedup, forward-on-change, 50 ms
// poll, Stop() handshake. This implementation follows the same shape with our
// own psvr2ipc wire format (docs/REFERENCE_VR_IMPLEMENTATIONS.md).
//
// Threading contract:
//   - Submit() is non-blocking and render-thread-safe: it only swaps the
//     cached desired command and bumps a generation counter (a locked section
//     held for nanoseconds). It NEVER waits on the pipe.
//   - All pipe I/O happens on one worker thread owned by Client.
//   - Missing bridge / failed connect / failed write: counted in Stats(),
//     gameplay continues; the worker keeps retrying with backoff.
//   - Stop() joins the worker (bounded wait), best-effort sends Off +
//     Shutdown, then closes the handle. Safe to call with no bridge present.

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace psvr2client {

#ifdef _WIN32
struct Stats {
    unsigned connects = 0;
    unsigned sends = 0;
    unsigned errors = 0;
};

inline psvr2ipc::EffectDesc ToWire(const psvr2trigger::Command& c) {
    psvr2ipc::EffectDesc fx;
    fx.effect = (std::uint8_t)c.effect;
    fx.hand = (std::uint8_t)c.hand;
    fx.start = c.start;
    fx.end = c.end;
    fx.strength = c.strength;
    fx.position = c.position;
    fx.amplitude = c.amplitude;
    fx.frequency = c.frequency;
    fx.startStrength = c.startStrength;
    fx.endStrength = c.endStrength;
    return fx;
}

class Client {
public:
    Client() { InitializeCriticalSection(&lock_); }
    ~Client() { Stop(); DeleteCriticalSection(&lock_); }
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // Validates names only; spawns the worker. Returns false on bad input or
    // thread failure. Connection to the bridge happens asynchronously.
    bool Start(const char* pipeName, const std::uint8_t* token, std::size_t tokenLen) {
        EnterCriticalSection(&lock_);
        if (running_) {
            LeaveCriticalSection(&lock_);
            return true;
        }
        const std::size_t nameLen = pipeName ? std::strlen(pipeName) : 0;
        if (!psvr2ipc::PipeNameValid(pipeName, nameLen) ||
            !psvr2ipc::TokenValid(token, tokenLen)) {
            LeaveCriticalSection(&lock_);
            return false;
        }
        std::memcpy(pipe_, pipeName, nameLen);
        pipe_[nameLen] = '\0';
        std::memcpy(token_, token, tokenLen);
        tokenLen_ = tokenLen;
        stopRequested_ = false;
        thread_ = CreateThread(nullptr, 0, &Client::WorkerEntry, this, 0, nullptr);
        const bool ok = thread_ != nullptr;
        running_ = ok;
        LeaveCriticalSection(&lock_);
        return ok;
    }

    // Cache a new desired command. Non-blocking. Dedupe happens on the
    // worker via generation compare (identical resubmits cost nothing).
    void Submit(const psvr2trigger::Command& cmd) {
        EnterCriticalSection(&lock_);
        const psvr2ipc::EffectDesc fx = ToWire(cmd);
        if (!(fx == desired_)) {
            desired_ = fx;
            ++generation_;
        }
        LeaveCriticalSection(&lock_);
    }

    void Stop() {
        HANDLE thread = nullptr;
        EnterCriticalSection(&lock_);
        if (!running_) {
            LeaveCriticalSection(&lock_);
            return;
        }
        stopRequested_ = true;
        thread = thread_;
        LeaveCriticalSection(&lock_);
        // Bounded join: the worker polls the stop flag every 250 ms, so a
        // 5 s wait cannot hang shutdown; on timeout the handle is still
        // closed and the flag released (worker exits on its own).
        if (thread) {
            WaitForSingleObject(thread, 5000);
            CloseHandle(thread);
        }
        EnterCriticalSection(&lock_);
        thread_ = nullptr;
        running_ = false;
        LeaveCriticalSection(&lock_);
    }

    Stats GetStats() {
        EnterCriticalSection(&lock_);
        Stats s = stats_;
        LeaveCriticalSection(&lock_);
        return s;
    }

private:
    static DWORD WINAPI WorkerEntry(LPVOID p) {
        ((Client*)p)->Worker();
        return 0;
    }
    void Worker() {
        char pipeName[160];
        std::uint8_t token[psvr2ipc::kMaxTokenBytes];
        std::size_t tokenLen = 0;
        EnterCriticalSection(&lock_);
        const std::size_t nameLen = std::strlen(pipe_);
        std::memcpy(pipeName, pipe_, nameLen < sizeof(pipeName) ? nameLen : sizeof(pipeName) - 1);
        pipeName[nameLen < sizeof(pipeName) ? nameLen : sizeof(pipeName) - 1] = '\0';
        tokenLen = tokenLen_;
        std::memcpy(token, token_, tokenLen);
        LeaveCriticalSection(&lock_);

        wchar_t wide[160];
        MultiByteToWideChar(CP_UTF8, 0, pipeName, -1, wide, 160);
        std::uint32_t sequence = 0;
        std::uint64_t lastSentGeneration = 0;
        bool helloDone = false;
        HANDLE h = INVALID_HANDLE_VALUE;

        for (;;) {
            EnterCriticalSection(&lock_);
            const bool stop = stopRequested_;
            const psvr2ipc::EffectDesc desired = desired_;
            const std::uint64_t gen = generation_;
            LeaveCriticalSection(&lock_);
            if (stop) break;

            if (h == INVALID_HANDLE_VALUE) {
                if (WaitNamedPipeW(wide, 250)) {
                    h = CreateFileW(wide, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
                    helloDone = false;
                }
                if (h == INVALID_HANDLE_VALUE) {
                    // Absent bridge: keep polling; render thread unaffected.
                    continue;
                }
                EnterCriticalSection(&lock_);
                ++stats_.connects;
                LeaveCriticalSection(&lock_);
            }

            std::uint8_t msg[psvr2ipc::kMaxMessageBytes];
            std::size_t n = 0;
            const std::uint32_t now = (std::uint32_t)GetTickCount64();
            bool wantSend = false;
            if (!helloDone) {
                n = psvr2ipc::EncodeHello(msg, ++sequence, now, token, tokenLen);
                wantSend = n > 0;
            } else if (gen != lastSentGeneration) {
                n = psvr2ipc::EncodeTrigger(msg, ++sequence, now, desired);
                wantSend = n > 0;
            }
            if (!wantSend) {
                Sleep(50); // KHARVOX 50 ms poll; stop checked at loop top
                continue;
            }
            DWORD written = 0;
            if (!WriteFile(h, msg, (DWORD)n, &written, nullptr) || written != n) {
                CloseHandle(h);
                h = INVALID_HANDLE_VALUE;
                helloDone = false;
                EnterCriticalSection(&lock_);
                ++stats_.errors;
                LeaveCriticalSection(&lock_);
                continue;
            }
            if (!helloDone) {
                helloDone = true;
                lastSentGeneration = 0; // force the pending state after auth
            } else {
                lastSentGeneration = gen;
            }
            EnterCriticalSection(&lock_);
            ++stats_.sends;
            LeaveCriticalSection(&lock_);
        }

        // Best-effort teardown: Off + Shutdown, then close. Failures ignored.
        if (h != INVALID_HANDLE_VALUE) {
            psvr2ipc::EffectDesc off;
            std::uint8_t msg[psvr2ipc::kMaxMessageBytes];
            DWORD written = 0;
            std::size_t n = psvr2ipc::EncodeTrigger(msg, ++sequence, (std::uint32_t)GetTickCount64(), off);
            if (n) WriteFile(h, msg, (DWORD)n, &written, nullptr);
            n = psvr2ipc::EncodeShutdown(msg, ++sequence, (std::uint32_t)GetTickCount64());
            if (n) WriteFile(h, msg, (DWORD)n, &written, nullptr);
            CloseHandle(h);
        }
    }

    CRITICAL_SECTION lock_;
    bool running_ = false;
    bool stopRequested_ = false;
    HANDLE thread_ = nullptr;
    char pipe_[160] = {0};
    std::uint8_t token_[psvr2ipc::kMaxTokenBytes] = {0};
    std::size_t tokenLen_ = 0;
    psvr2ipc::EffectDesc desired_;
    std::uint64_t generation_ = 0;
    Stats stats_;
};
#else
// Non-Windows: compile-time stub so host logic tests still build.
struct Stats {
    unsigned connects = 0, sends = 0, errors = 0;
};
class Client {
public:
    bool Start(const char*, const std::uint8_t*, std::size_t) { return false; }
    void Submit(const psvr2trigger::Command&) {}
    void Stop() {}
    Stats GetStats() { return Stats{}; }
};
#endif

} // namespace psvr2client
