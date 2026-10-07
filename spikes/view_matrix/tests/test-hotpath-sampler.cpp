#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../hotpath_sampler.h"
#include <cassert>

int main() {
    hotpath::sampler s;
    s.configure(false);
    {
        hotpath::scope disabled(s, hotpath::Family::draw_hook);
    }
    assert(s.read(hotpath::Family::draw_hook).calls == 0);
    assert(s.counter(hotpath::Counter::xr_sync_actions) == 0);

    s.configure(true, 5);
    for (int frame = 0; frame < 5; ++frame) {
        s.count(hotpath::Counter::xr_sync_actions);
        {
            hotpath::scope total(s, hotpath::Family::frame_total);
            for (int call = 0; call < 64; ++call) {
                hotpath::scope draw(s, hotpath::Family::draw_hook);
            }
        }
        const bool ended = s.advance_frame();
        assert(ended == (frame == 4));
    }
    assert(!s.active());
    assert(s.frame_token() == 0);
    assert(s.read(hotpath::Family::frame_total).calls == 5);
    const auto draw = s.read(hotpath::Family::draw_hook);
    assert(draw.calls == 320);
    assert(draw.samples == 10);
    assert(s.milliseconds_per_frame(hotpath::Family::frame_total, 5) >= 0.0);

    assert(s.counter(hotpath::Counter::xr_sync_actions) == 5);
    s.configure(true, 2);
    assert(s.read(hotpath::Family::draw_hook).calls == 0);
    assert(s.counter(hotpath::Counter::xr_sync_actions) == 0);
    assert(s.next_xr_frame() == 1);
    assert(s.next_xr_frame() == 2);
    assert(!s.advance_frame());
    assert(s.advance_frame());
    assert(s.next_xr_frame() == 2);
}
