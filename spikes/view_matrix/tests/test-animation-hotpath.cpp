#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cstdio>

__declspec(align(8)) volatile LONG64 untilMs=0,fromMs=0;
volatile LONG cached=1,sinkValue=0;

static double elapsed(LARGE_INTEGER a,LARGE_INTEGER b,LARGE_INTEGER f){return (b.QuadPart-a.QuadPart)*1000.0/f.QuadPart;}
int main(){
    const int iterations=4000000;const LONG64 now=GetTickCount64();
    InterlockedExchange64(&untilMs,now+60000);InterlockedExchange64(&fromMs,now-1);
    LARGE_INTEGER f,a,b;QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&a);
    for(int i=0;i<iterations;++i){
        const LONG64 until=InterlockedCompareExchange64(&untilMs,0,0);
        const LONG64 tick=GetTickCount64();
        sinkValue=(until&&tick<until&&tick>=InterlockedCompareExchange64(&fromMs,0,0))?1:0;
    }
    QueryPerformanceCounter(&b);const double oldMs=elapsed(a,b,f);
    QueryPerformanceCounter(&a);
    for(int i=0;i<iterations;++i)sinkValue=cached;
    QueryPerformanceCounter(&b);const double cachedMs=elapsed(a,b,f);
    assert(sinkValue==1&&oldMs>cachedMs);
    std::printf("animation hot path: old deadline checks %.3f ms, cached reads %.3f ms, %.1fx reduction (%d calls)\n",oldMs,cachedMs,oldMs/cachedMs,iterations);
}
