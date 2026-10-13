#include "../texture_hook_timing.h"
#include <cstdio>
static_assert(!textiming::enabled,"normal builds must have timing OFF");
int main() {
    textiming::Profiler profiler;profiler.Start();unsigned forwarded=0;
    for(unsigned n=0;n<10000;++n) {
        textiming::Scope scope(profiler);
        if(scope.Finish(textiming::Route::native_hit,[&]{++forwarded;return E_ACCESSDENIED;})!=E_ACCESSDENIED)return 1;
    }
    const bool ok=forwarded==10000 && profiler.Read(textiming::Route::native_hit).samples==0;
    std::printf("%s default-OFF compiled policy:10000 forwards unchanged,zero samples\n",ok?"PASS":"FAIL");
    return ok?0:1;
}
