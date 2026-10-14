#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../hotpath_sampler.h"
#include <cstdio>
#include <thread>
static int failures=0;
static void Check(bool ok,const char* text){std::printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failures;}
int main() {
    hotpath::sampler sampler;sampler.configure(false);unsigned calls=0;
    auto forward=[&]()->HRESULT{++calls;return E_ACCESSDENIED;};
    {hotpath::scope draw(sampler,hotpath::Family::draw_hook);
        Check(hotpath::forward_draw(sampler,forward)==E_ACCESSDENIED,"disabled preserves failing HRESULT");}
    Check(calls==1 && hotpath::draw_scope_depth==0 && sampler.read(hotpath::Family::native_draw_forward).calls==0,"disabled exact forward/no profile domain");
    sampler.configure(true,2);hotpath::forward_draw(sampler,forward);
    Check(sampler.read(hotpath::Family::native_draw_forward).calls==0,"standalone overlay call excluded");
    unsigned left=0,right=0;
    for(unsigned n=0;n<512000;++n)if(sampler.should_time(hotpath::Family::native_draw_forward)){if(n&1)++right;else ++left;}
    Check(left>380 && left<650 && right>380 && right<650,"sparse selection covers both adjacent eye parities");
    {
        hotpath::scope draw(sampler,hotpath::Family::draw_hook);
        Check(hotpath::draw_scope_depth==1,"draw domain entered");
        {hotpath::scope nested(sampler,hotpath::Family::draw_hook);Check(hotpath::draw_scope_depth==2,"nested draw domain");}
        unsigned foreignCalls=0;
        std::thread foreign([&]{hotpath::forward_draw(sampler,[&]()->HRESULT{++foreignCalls;return S_OK;});});
        foreign.join();
        Check(foreignCalls==1 && hotpath::draw_scope_depth==1 && sampler.read(hotpath::Family::native_draw_forward).calls==0,"another thread cannot inherit parent's draw domain");
        for(unsigned n=0;n<102400;++n)if(hotpath::forward_draw(sampler,forward)!=E_ACCESSDENIED)++failures;
        const auto result=sampler.read(hotpath::Family::native_draw_forward);
        Check(result.calls==102400 && result.samples>150 && result.samples<250 && calls==102402,"102400 exact forwards, randomized sample rate near1/512");
        draw.finish();draw.finish();Check(hotpath::draw_scope_depth==0,"finish idempotent and exits domain");
    }
    try {
        hotpath::scope draw(sampler,hotpath::Family::draw_hook);
        hotpath::forward_draw(sampler,[]()->HRESULT{throw 7;});++failures;
    }catch(int value){Check(value==7 && hotpath::draw_scope_depth==0,"exception propagated; RAII domain released");}
    {
        hotpath::scope draw(sampler,hotpath::Family::draw_hook);sampler.stop();
        const auto count=sampler.read(hotpath::Family::native_draw_forward).calls;
        hotpath::forward_draw(sampler,forward);
        Check(sampler.read(hotpath::Family::native_draw_forward).calls==count,"stop prevents new records even with in-flight parent");
    }
    Check(hotpath::draw_scope_depth==0,"shutdown leaves no profiled domain");
    return failures?1:0;
}
