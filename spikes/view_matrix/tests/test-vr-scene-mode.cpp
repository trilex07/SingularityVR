#include "../vr_scene_mode.h"
#include "../cutscene_mono.h"
#include "../menu_submit_policy.h"
#include "../frontend_menu_comfort.h"
#include <cassert>
#include <cstdio>

using vrscene::Controller;
using vrscene::Mode;
using vrscene::Signals;

int main() {
    assert(vrscene::CompletedFrameMode(Mode::CutsceneFlat,Mode::GameplayStereo)==Mode::CutsceneFlat);
    assert(vrscene::CompletedFrameMode(Mode::GameplayStereo,Mode::CutsceneFlat)==Mode::GameplayStereo);
    assert(vrscene::CompletedFrameMode(Mode::GameplayStereo,Mode::VideoFlat)==Mode::VideoFlat);
    assert(vrscene::CompletedFrameMode(Mode::CutsceneFlat,Mode::MenuFlat)==Mode::MenuFlat);
    assert(vrscene::RecoveryFrameMode(Mode::GameplayStereo,true,0)==Mode::CutsceneFlat);
    assert(vrscene::RecoveryFrameMode(Mode::GameplayStereo,true,1)==Mode::GameplayStereo);
    assert(vrscene::RecoveryFrameMode(Mode::GameplayStereo,false,0)==Mode::GameplayStereo);
    assert(vrscene::RecoveryFrameMode(Mode::MenuFlat,true,0)==Mode::MenuFlat);
    assert(vrscene::RecoveryFrameMode(Mode::VideoFlat,true,0)==Mode::VideoFlat);
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
    for(int cls=0;cls<=4;++cls) {
        assert(!vrscene::FrontendMenuPresentation(false,cls,true));
        assert(!vrscene::FrontendMenuPresentation(true,cls,false));
        assert(vrscene::FrontendMenuPresentation(true,cls,true)==(cls==1 || cls==2));
        assert(!vrscene::FrontendMenuPresentation(true,cls,true,true,true));
    }
    // Real Steam trace: nativeOn=1 at frame941, then SPIIS Start/Quit focus.
    Controller iis;
    iis.Advance({true,false}); iis.Advance({true,false});
    assert(iis.Current()==Mode::CutsceneFlat);
    iis.Advance({true,false,true,false,vrscene::FrontendMenuPresentation(true,1,true,true,false)});
    assert(iis.Current()==Mode::MenuFlat && iis.NeedsMonoPair() && MenuSubmitForcesFlat(1));
    Controller frontend;
    frontend.Advance({true,false,false,false});
    frontend.Advance({true,false,false,false});
    assert(frontend.Current()==Mode::CutsceneFlat);
    const auto takeover=frontend.Advance({true,false,true,false,true});
    assert(takeover.to==Mode::MenuFlat && takeover.reason==16 && !frontend.Flat());
    // Confirmed Bink wins over stale frontend focus on a new-game handoff.
    frontend.Advance({true,true,true,false,true});
    assert(frontend.Current()==Mode::VideoFlat);
    frontend.Advance({true,false,true,false,true});
    assert(frontend.Current()==Mode::MenuFlat);
    frontend.Advance({true,false,false,false,false});
    frontend.Advance({true,false,false,false,false});
    assert(frontend.Current()==Mode::CutsceneFlat);
    for(unsigned i=0;i<Controller::kExitFrames;++i)frontend.Advance(gameplay);
    assert(frontend.Current()==Mode::GameplayStereo);
    for(int cls=0;cls<=4;++cls)for(int focus=0;focus<=1;++focus)
    for(int cine=0;cine<=1;++cine)for(int paused=0;paused<=1;++paused) {
        assert(!CinemaMenuIdenticalCrop(false,cls,focus,cine,paused));
        assert(CinemaMenuIdenticalCrop(true,cls,focus,cine,paused)==
            (focus && !(cine && paused) && (cls==2 || cls==4)));
        assert(!CinemaMenuIdenticalCrop(true,cls,focus,cine,paused,true));
        assert(!CinemaMenuPauseOwnsPresentation(true,cls,focus,cine,paused));
    }
    Controller cinemaPause;
    cinemaPause.Advance({true,false,false,false});
    cinemaPause.Advance({true,false,false,false});
    assert(cinemaPause.Current()==Mode::CutsceneFlat);
    // Pause during cutscene stays in CutsceneFlat (flat Cinema Mode screen).
    auto p=cinemaPause.Advance({true,false,true,true,false,false});
    assert(!p.changed && cinemaPause.Current()==Mode::CutsceneFlat && cinemaPause.Flat());
    assert(cinemaPause.NeedsMonoPair());
    for(unsigned i=0;i<Controller::kExitFrames;++i)cinemaPause.Advance(gameplay);
    assert(cinemaPause.Current()==Mode::GameplayStereo);
    // Reproduce sparse Bink observations followed by a focused frontend with
    // stale native cinema. Missing video observations retain hysteresis; frontend
    // ownership must enable the clean provider rather than force GameplayStereo.
    Controller cold;
    cold.Advance({false,true});
    for(unsigned i=0;i<Controller::kExitFrames-1;++i) {
        cold.Advance({}); assert(cold.Current()==Mode::VideoFlat);
        cold.Advance({false,true});
    }
    const bool owns=frontmenu::OwnsPresentation(true,2,true,true,false);
    assert(owns);
    cold.Advance({true,false,true,false,owns});
    assert(cold.Current()==Mode::MenuFlat && cold.NeedsMonoPair());
    frontmenu::Signals clean{true,true,true,true,true,false,false,true,true,true,false,2};
    assert(frontmenu::Eligible(clean));
    clean.menuMode=false; assert(!frontmenu::Eligible(clean));
    // Returning from gameplay with a pawn must use exactly the same ownership.
    cold.Advance({}); assert(cold.Current()==Mode::GameplayStereo);
    cold.Advance({true,false,true,false,owns});
    assert(cold.Current()==Mode::MenuFlat);
    Controller scripted;
    assert(!scripted.Advance({true, false, false, false, false, false}).changed);
    auto sf = scripted.Advance({true, false, false, false, false, false});
    assert(sf.changed && sf.to == Mode::CutsceneFlat);
    assert(vrscene::ScriptedCameraFlatPresentation(true, true, true, false, false, false));
    assert(!vrscene::ScriptedCameraFlatPresentation(true, true, false, false, false, false));
    assert(!vrscene::ScriptedCameraFlatPresentation(true, false, true, false, false, false));
    std::puts("PASS: cinematic hysteresis, video mono, noisy signal, menu classification, scripted cinema and owned frontend");
}
