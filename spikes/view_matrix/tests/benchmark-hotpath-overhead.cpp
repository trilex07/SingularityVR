#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../hotpath_sampler.h"
#include <cstdio>
#include <initializer_list>
// Actual PID28884 first480 frames, rounded per-family invocation counts.
// This replays ACCOUNTING ONLY, never native work or nested inclusive timings.
static constexpr unsigned calls[]={1,0,0,2128,2128,1848,14,0,0,1845,3640,3640,3640,10606,1,1,1,1,1,1,4199};
static_assert(sizeof(calls)/sizeof(*calls)==static_cast<unsigned>(hotpath::Family::count));
int main(){
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
    for(bool enabled:{false,true}){
        hotpath::sampler s;s.configure(enabled,1000);
        LARGE_INTEGER start,end;ULONG64 c0=0,c1=0;
        QueryThreadCycleTime(GetCurrentThread(),&c0);QueryPerformanceCounter(&start);
        constexpr unsigned frames=200;
        for(unsigned frame=0;frame<frames;++frame){
            for(unsigned family=0;family<static_cast<unsigned>(hotpath::Family::count);++family)
                for(unsigned call=0;call<calls[family];++call){hotpath::scope sample(s,static_cast<hotpath::Family>(family));}
        }
        QueryPerformanceCounter(&end);QueryThreadCycleTime(GetCurrentThread(),&c1);
        std::printf("active=%u accounting_ms/frame=%.6f cycles/frame=%.0f calls_other=%llu samples_other=%llu\n",enabled?1:0,
            double(end.QuadPart-start.QuadPart)*1000/frequency.QuadPart/frames,double(c1-c0)/frames,
            s.read(hotpath::Family::other_set_hooks).calls,s.read(hotpath::Family::other_set_hooks).samples);
    }
}
