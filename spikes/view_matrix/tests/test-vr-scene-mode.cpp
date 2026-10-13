#include "../vr_scene_mode.h"
#include "../cutscene_mono.h"
#include <cassert>
#include <cstdio>

using vrscene::Controller;
using vrscene::Mode;
using vrscene::Signals;

int main() {
    Controller c;
    Signals gameplay{};
    assert(c.Current() == Mode::GameplayStereo);
    assert(!c.Advance({true,false,false,false}).changed);
    auto enter = c.Advance({true,false,false,false});
    assert(enter.changed && enter.to == Mode::CutsceneFlat && c.Flat());
    assert(!c.Advance({true,false,false,false}).changed);
    assert(!c.Advance({false,false,false,false}).changed);
    assert(!c.Advance({false,false,false,false}).changed);
    assert(!c.Advance({false,false,false,false}).changed);
    auto exit = c.Advance({false,false,false,false});
    assert(exit.changed && exit.to == Mode::GameplayStereo && !c.Flat());

    Controller noisy;
    assert(!noisy.Advance({true,false,false,false}).changed);
    assert(noisy.Advance({false,false,false,false}).to == Mode::GameplayStereo);
    assert(!noisy.Advance({true,false,false,false}).changed);

    Controller video;
    auto v = video.Advance({false,true,false,false});
    assert(v.changed && v.to == Mode::VideoFlat && video.Flat());
    assert(!video.NeedsMonoPair());
    for (unsigned i=0; i<Controller::kExitFrames-1; ++i)
        assert(!video.Advance(gameplay).changed);
    assert(video.Advance(gameplay).to == Mode::GameplayStereo);

    Controller handoff;
    handoff.Advance({true,true,false,false});
    assert(handoff.Current()==Mode::VideoFlat);
    const CameraChangeCtx loading{true,false,false,true,true};
    const bool retained=loading.cineOn && !CameraChangeClearsCine(loading);
    handoff.Advance({retained,false,true,true});
    handoff.Advance({retained,false,false,false});
    assert(handoff.Current()==Mode::CutsceneFlat);
    // Native OFF is not overridden; full scene -> gameplay restores stereo.
    for(unsigned i=0;i<Controller::kExitFrames;++i)handoff.Advance(gameplay);
    assert(handoff.Current()==Mode::GameplayStereo && !handoff.Flat());
    handoff.Advance({false,false,true,true});
    assert(handoff.Current()==Mode::MenuFlat);

    Controller menu;
    auto m = menu.Advance({false,false,true,true});
    assert(m.changed && m.to == Mode::MenuFlat && !menu.Flat());
    assert(menu.NeedsMonoPair());
    auto g = menu.Advance(gameplay);
    assert(g.changed && g.to == Mode::GameplayStereo);
    assert(!menu.NeedsMonoPair());
    std::puts("PASS: cinematic hysteresis, video mono, noisy signal, menu classification");
}
