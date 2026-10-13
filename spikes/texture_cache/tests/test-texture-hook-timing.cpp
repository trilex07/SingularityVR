#define TEXCACHE_TIMING 1
#include "../texture_hook_timing.h"
#include <cstdio>
static int failures=0;
static void Check(bool ok,const char* text){std::printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failures;}
int main() {
    std::uint32_t random=0x723fe501u;unsigned chosen=0;
    for(unsigned n=0;n<1024000;++n)chosen+=textiming::Select(random);
    Check(chosen>850 && chosen<1150,"bounded randomized rate near1/1024 (not periodic draw alias)");
    textiming::Profiler profiler;unsigned forwarded=0;const HRESULT sentinel=E_ACCESSDENIED;
    {textiming::Scope scope(profiler);Check(scope.Finish(textiming::Route::native_hit,[&]{++forwarded;return sentinel;})==sentinel,"disabled preserves HRESULT");}
    Check(profiler.Read(textiming::Route::native_hit).samples==0 && forwarded==1,"disabled neither times nor drops forward");
    profiler.Start();
    for(unsigned n=0;n<200000;++n) {
        textiming::Scope scope(profiler);
        if(scope.Finish(textiming::Route::positive_hit,[&]{++forwarded;return sentinel;})!=sentinel)++failures;
    }
    const auto snapshot=profiler.Read(textiming::Route::positive_hit);
    Check(snapshot.samples>0 && snapshot.classifyTicks>=0 && snapshot.forwardTicks>=0 && forwarded==200001,"classification and forwarded calls have separate nonnegative samples");
    profiler.Stop();const auto before=profiler.Read(textiming::Route::positive_hit).samples;
    for(unsigned n=0;n<20000;++n){textiming::Scope scope(profiler);scope.Finish(textiming::Route::positive_hit,[&]{++forwarded;return S_OK;});}
    Check(profiler.Read(textiming::Route::positive_hit).samples==before && forwarded==220001,"stop preserves forwarding and freezes new samples");
    textiming::Profiler bounded;bounded.Start();random=0x723fe501u;unsigned claims=0;
    // Exercise the sample cap without QPC/driver calls. All real calls remain forwarded.
    for(unsigned n=0;n<30000000;++n)claims+=bounded.Claim(random);
    Check(claims==textiming::limit && !bounded.Claim(random),"hard20000 sample cap, no rearm");
    volatile LONG64 maximum=0;textiming::Profiler::Max(maximum,7);textiming::Profiler::Max(maximum,3);
    Check(maximum==7,"max CPU duration retained");
    return failures?1:0;
}
