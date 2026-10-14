#pragma once
#include <windows.h>
#include <cstdint>

// Diagnostic-only build. No new hooks, rendering state, heap allocations, or
// per-bind logging. Randomized sparse samples avoid periodic draw-list aliasing.
#ifndef TEXCACHE_TIMING
#define TEXCACHE_TIMING 0
#endif
namespace textiming {
inline constexpr bool enabled=TEXCACHE_TIMING!=0;
inline constexpr unsigned limit=20000;
enum class Route : unsigned {native_hit,positive_hit,legacy_native,noncandidate,descriptor_error,
    hashed_native,hashed_positive,count};
inline const char* Name(Route route) {
    switch(route) {
    case Route::native_hit:return "native_hit";
    case Route::positive_hit:return "positive_hit";
    case Route::legacy_native:return "legacy_native";
    case Route::noncandidate:return "noncandidate";
    case Route::descriptor_error:return "descriptor_error";
    case Route::hashed_native:return "hashed_native";
    case Route::hashed_positive:return "hashed_positive";
    default:return "unknown";
    }
}
inline bool Select(std::uint32_t& state) noexcept {
    state^=state<<13;state^=state>>17;state^=state<<5;
    return (state&1023u)==0; // approximately 1/1024, not every1024th draw
}
struct alignas(8) Record {
    volatile LONG64 samples=0,classifyTicks=0,forwardTicks=0,maxClassifyTicks=0,maxForwardTicks=0;
};
struct Snapshot {LONG64 samples,classifyTicks,forwardTicks,maxClassifyTicks,maxForwardTicks;};
class Profiler {
    volatile LONG active_=0,tickets_=0;
    ULONGLONG deadline_=0;
    LARGE_INTEGER frequency_{};
    Record records_[static_cast<unsigned>(Route::count)]{};
public:
    void Start() noexcept {
        if constexpr(enabled) {
            // Exactly once at initialization, before hook installation.
            QueryPerformanceFrequency(&frequency_);
            deadline_=GetTickCount64()+300000;
            InterlockedExchange(&active_,frequency_.QuadPart>0?1:0);
        }
    }
    void Stop() noexcept {if constexpr(enabled)InterlockedExchange(&active_,0);}
    bool Claim(std::uint32_t& sequence) noexcept {
        if constexpr(!enabled)return false;
        if(!InterlockedCompareExchange(&active_,0,0) || !Select(sequence))return false;
        // Check the wall bound only after sparse selection, never every bind.
        if(GetTickCount64()>=deadline_){Stop();return false;}
        if(InterlockedIncrement(&tickets_)>static_cast<LONG>(limit)){Stop();return false;}
        return true;
    }
    void Add(Route route,LONGLONG classify,LONGLONG forward) noexcept {
        auto& record=records_[static_cast<unsigned>(route)];
        // Completion can follow Stop(); never reset storage with scopes in flight.
        InterlockedExchangeAdd64(&record.classifyTicks,classify);
        InterlockedExchangeAdd64(&record.forwardTicks,forward);
        Max(record.maxClassifyTicks,classify);Max(record.maxForwardTicks,forward);
        InterlockedIncrement64(&record.samples);
    }
    static void Max(volatile LONG64& target,LONG64 value) noexcept {
        LONG64 observed=InterlockedCompareExchange64(&target,0,0);
        while(value>observed) {
            const LONG64 prior=InterlockedCompareExchange64(&target,value,observed);
            if(prior==observed)break;observed=prior;
        }
    }
    Snapshot Read(Route route) noexcept {
        auto& r=records_[static_cast<unsigned>(route)];
        return {InterlockedCompareExchange64(&r.samples,0,0),InterlockedCompareExchange64(&r.classifyTicks,0,0),
            InterlockedCompareExchange64(&r.forwardTicks,0,0),InterlockedCompareExchange64(&r.maxClassifyTicks,0,0),
            InterlockedCompareExchange64(&r.maxForwardTicks,0,0)};
    }
    double Us(LONG64 ticks) const noexcept {return frequency_.QuadPart?double(ticks)*1e6/frequency_.QuadPart:0.0;}
};
class Scope {
    Profiler* profiler_=nullptr;
    LARGE_INTEGER begin_{};
public:
    explicit Scope(Profiler& profiler) noexcept {
        if constexpr(enabled) {
            thread_local std::uint32_t sequence=0x723fe501u;
            if(profiler.Claim(sequence)){profiler_=&profiler;QueryPerformanceCounter(&begin_);}
        }
    }
    template<class Forward> HRESULT Finish(Route route,Forward&& forward) noexcept {
        if(!profiler_)return forward();
        LARGE_INTEGER classified{},ended{};QueryPerformanceCounter(&classified);
        const HRESULT result=forward();QueryPerformanceCounter(&ended);
        profiler_->Add(route,classified.QuadPart-begin_.QuadPart,ended.QuadPart-classified.QuadPart);
        profiler_=nullptr;return result; // preserve exact HRESULT / exactly one call
    }
};
}
