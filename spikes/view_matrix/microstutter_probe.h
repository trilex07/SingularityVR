#pragma once
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "microstutter_core.h"

// Opt-in, process-local frame/event trace. One render-thread writer and one
// below-normal-priority worker. Event producers only perform atomic operations;
// no hook opens files, allocates memory, walks stacks or waits for a GPU query.
namespace microstutter {
inline constexpr unsigned kCapacity = 512;
inline constexpr unsigned kMaxFrames = 9000;
static SRWLOCK lock = SRWLOCK_INIT;
static Sample ring[kCapacity]{};
static volatile LONG enabled = 0, pendingEvents = 0;
static volatile LONG fireMaxUs = 0, damageMaxUs = 0, lost = 0;
static volatile LONG published = 0;
static HANDLE wake = nullptr;
static HANDLE worker = nullptr;
static char outputPath[MAX_PATH]{};
static LARGE_INTEGER frequency{};
static std::uint64_t previousQpc = 0, beginQpc = 0, cpuBegin = 0;
static unsigned frameId = 0;
static bool activeFrame = false;

static std::uint64_t Qpc() noexcept {
    LARGE_INTEGER v{}; QueryPerformanceCounter(&v);
    return static_cast<std::uint64_t>(v.QuadPart);
}
static std::uint32_t Us(std::uint64_t ticks) noexcept {
    return frequency.QuadPart ? static_cast<std::uint32_t>(
        ticks * 1000000ULL / static_cast<std::uint64_t>(frequency.QuadPart)) : 0;
}
static std::uint64_t ThreadCpu100ns() noexcept {
    FILETIME create{}, exit{}, kernel{}, user{};
    if (!GetThreadTimes(GetCurrentThread(), &create, &exit, &kernel, &user)) return 0;
    ULARGE_INTEGER k{}, u{};
    k.LowPart=kernel.dwLowDateTime; k.HighPart=kernel.dwHighDateTime;
    u.LowPart=user.dwLowDateTime; u.HighPart=user.dwHighDateTime;
    return k.QuadPart + u.QuadPart;
}
static void AtomicMax(volatile LONG* target, LONG value) noexcept {
    LONG old = InterlockedCompareExchange(target, 0, 0);
    while (value > old && InterlockedCompareExchange(target, value, old) != old)
        old = InterlockedCompareExchange(target, 0, 0);
}
static void Mark(Event event) noexcept {
    if (InterlockedCompareExchange(&enabled, 0, 0) == 1)
        InterlockedOr(&pendingEvents, static_cast<LONG>(event));
}
static void NativeDuration(Event event, std::uint64_t startQpc) noexcept {
    if (InterlockedCompareExchange(&enabled, 0, 0) != 1 || !startQpc) return;
    const LONG us = static_cast<LONG>(Us(Qpc() - startQpc));
    AtomicMax(event == Fire ? &fireMaxUs : &damageMaxUs, us);
}
static void Begin(bool gameplayFocused = true) noexcept {
    if (InterlockedCompareExchange(&enabled, 0, 0) != 1) return;
    activeFrame=gameplayFocused;
    if(!gameplayFocused){
        previousQpc=0;
        InterlockedExchange(&pendingEvents,0);
        InterlockedExchange(&fireMaxUs,0);
        InterlockedExchange(&damageMaxUs,0);
        return;
    }
    beginQpc = Qpc();
    cpuBegin = ThreadCpu100ns();
}
static void Finish(std::uint32_t presentCallUs) noexcept {
    if (InterlockedCompareExchange(&enabled, 0, 0) != 1 || !activeFrame) return;
    const std::uint64_t end = Qpc();
    const std::uint64_t cpuEnd = ThreadCpu100ns();
    Sample s{};
    s.frame=++frameId; s.thread=GetCurrentThreadId(); s.qpc=end;
    s.events=static_cast<std::uint32_t>(InterlockedExchange(&pendingEvents,0));
    // End-to-end interval and current hook cost refer to the SAME frame.
    s.frameWallUs=previousQpc ? Us(end-previousQpc) : 0;
    s.hookWallUs=Us(end-beginQpc);
    s.presentCallUs=presentCallUs;
    s.renderThreadCpuUs=cpuEnd>=cpuBegin ? static_cast<std::uint32_t>((cpuEnd-cpuBegin)/10) : 0;
    s.fireNativeMaxUs=static_cast<std::uint32_t>(InterlockedExchange(&fireMaxUs,0));
    s.damageNativeMaxUs=static_cast<std::uint32_t>(InterlockedExchange(&damageMaxUs,0));
    previousQpc=end;
    AcquireSRWLockExclusive(&lock);
    ring[s.frame%kCapacity]=s;
    InterlockedExchange(&published,static_cast<LONG>(s.frame));
    ReleaseSRWLockExclusive(&lock);
    // Worker drains in 100 ms batches; waking/flushing it every frame would
    // create the very CPU and I/O jitter this probe is intended to measure.
    if (frameId >= kMaxFrames) {
        InterlockedExchange(&enabled, 2);
        if(wake) SetEvent(wake);
    }
}
static DWORD WINAPI Worker(void*) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    FILE* file=nullptr;
    if (fopen_s(&file,outputPath,"w") || !file) {
        InterlockedExchange(&enabled,2);
        return 0;
    }
    fputs("frame,thread,qpc,frame_wall_us,hook_wall_us,present_call_us,render_thread_cpu_us,event_mask,fire_native_max_us,damage_native_max_us\n",file);
    unsigned next=1;
    for (;;) {
        if (wake) WaitForSingleObject(wake,100);
        const unsigned newest=static_cast<unsigned>(InterlockedCompareExchange(&published,0,0));
        if (newest >= next+kCapacity) {
            InterlockedAdd(&lost,static_cast<LONG>(newest-next-kCapacity+1));
            next=newest-kCapacity+1;
        }
        bool wrote=false;
        while (next<=newest) {
            Sample s{};
            AcquireSRWLockShared(&lock);
            s=ring[next%kCapacity];
            ReleaseSRWLockShared(&lock);
            if(s.frame==next) {
                fprintf(file,"%u,%u,%llu,%u,%u,%u,%u,%u,%u,%u\n",s.frame,s.thread,
                    static_cast<unsigned long long>(s.qpc),s.frameWallUs,s.hookWallUs,
                    s.presentCallUs,s.renderThreadCpuUs,s.events,s.fireNativeMaxUs,s.damageNativeMaxUs);
                wrote=true;
            } else InterlockedIncrement(&lost);
            ++next;
        }
        if(wrote) fflush(file);
        if(InterlockedCompareExchange(&enabled,0,0)==2 && next>newest) break;
    }
    fprintf(file,"# lost_frames=%ld\n",InterlockedCompareExchange(&lost,0,0));
    fclose(file);
    return 0;
}
static bool Start(const char* logPath) {
    if(!logPath || !*logPath || InterlockedCompareExchange(&enabled,0,0)) return false;
    const char* slash=strrchr(logPath,'\\');
    if(!slash) return false;
    const size_t dir=static_cast<size_t>(slash-logPath);
    if(dir+48>=MAX_PATH) return false;
    memcpy(outputPath,logPath,dir); outputPath[dir]=0;
    sprintf_s(outputPath+dir,MAX_PATH-dir,"\\cpu_microstutter_%lu.csv",GetCurrentProcessId());
    QueryPerformanceFrequency(&frequency);
    if(!frequency.QuadPart) return false;
    wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!wake) return false;
    // Process-owned worker: pin this module, like the existing async logger.
    HMODULE owner=nullptr;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
       reinterpret_cast<LPCSTR>(&Worker),&owner)) {CloseHandle(wake);wake=nullptr;return false;}
    InterlockedExchange(&enabled,1);
    worker=CreateThread(nullptr,0,Worker,nullptr,0,nullptr);
    if(!worker){InterlockedExchange(&enabled,0);if(wake)CloseHandle(wake);wake=nullptr;return false;}
    return true;
}
// Explicit test/tool shutdown. The game path remains bounded to kMaxFrames;
// this is not called by DllMain and never runs on a render hook.
static bool StopAndWait(DWORD timeoutMs=2000) {
    if(!worker) return false;
    InterlockedExchange(&enabled,2);
    if(wake) SetEvent(wake);
    const bool complete=WaitForSingleObject(worker,timeoutMs)==WAIT_OBJECT_0;
    if(complete){CloseHandle(worker);worker=nullptr;if(wake)CloseHandle(wake);wake=nullptr;}
    return complete;
}
}
