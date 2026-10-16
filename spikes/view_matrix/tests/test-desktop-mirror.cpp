#include "../desktop_mirror.h"
#include <cstdio>
#include <cmath>
#include <cstdarg>
#include <initializer_list>

void Log(const char* fmt, ...) {
    (void)fmt;
    va_list ap; va_start(ap, fmt); va_end(ap);
}

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

struct FaultOps {
    int fail=0, step=0, back=17, candidate=0, commits=0;
    bool Pass() { return ++step!=fail; }
    bool CopyEye() { return Pass(); }
    bool ClearCandidate() { candidate=0; return Pass(); }
    bool FitCandidate() { candidate=23; return Pass(); }
    bool Commit() { ++commits; if (!Pass()) return false; back=candidate; return true; }
};

int main() {
    for(int fail=1;fail<=4;++fail) {
        FaultOps ops;ops.fail=fail;
        const auto stage=desktop_mirror::ComposeTransactional(ops,true);
        Expect(stage!=desktop_mirror::ComposeStage::Done && ops.back==17 && ops.commits==(fail==4?1:0),
               "failed composition never clears submitted image and short-circuits");
    }
    FaultOps ops;
    Expect(desktop_mirror::ComposeTransactional(ops,true)==desktop_mirror::ComposeStage::Done &&
           ops.back==23 && ops.commits==1,"one completed transaction commits exactly once");
    Expect(desktop_mirror::SourceLeft(5264, 2, false, false) == 2632, "right eye is the second half");
    Expect(desktop_mirror::SourceLeft(5264, 2, true, false) == 0, "swapped right eye is the first half");
    Expect(desktop_mirror::SourceLeft(5264, 1, false, false) == 0, "left eye is the first half");
    Expect(desktop_mirror::SourceLeft(5264, 1, true, false) == 2632, "swapped left eye is the second half");
    Expect(desktop_mirror::SourceLeft(5264, 2, false, true) == 0, "cropped frame keeps the repeated half");
    Expect(desktop_mirror::SourceLeft(5264, 2, true, true) == 2632, "cropped swap keeps the other repeated half");
    Expect(desktop_mirror::SourceLeft(5264, 2, false, false, true) == 0, "mono/cutscene frame forces left half even for mode 2");
    Expect(desktop_mirror::SourceLeft(5264, 2, true, false, true) == 2632, "mono/cutscene swap frame forces swapped half even for mode 2");
    RECT dst{};
    Expect(desktop_mirror::FitDestination(3840,1080,1920,1080,1920,1080,dst) &&
           dst.left==0 && dst.right==3840 && dst.top==0 && dst.bottom==1080,
           "Present to 16:9 window preserves 16:9 source");
    Expect(desktop_mirror::FitDestination(1920,1080,1080,1080,1920,1080,dst) &&
           dst.left==420 && dst.right==1500 && dst.top==0 && dst.bottom==1080,
           "square eye pillarboxed on widescreen TV");
    Expect(desktop_mirror::FitDestination(1000,1000,1920,1080,1000,1000,dst) &&
           dst.top==218 && dst.bottom==781 && dst.left==0 && dst.right==1000,
           "wide film letterboxed without cropping");
    Expect(desktop_mirror::FitDestination(5264,2684,2632,2684,1920,1080,dst),
           "PSVR2 eye fit available");
    const double shownAspect = double(dst.right-dst.left)*1920/5264 /
                               (double(dst.bottom-dst.top)*1080/2684);
    Expect(std::fabs(shownAspect - 2632.0/2684) < .001,
           "nonmatching backbuffer and TV aspects compensated");
    Expect(!desktop_mirror::FitDestination(0,1080,1920,1080,1920,1080,dst), "zero dimensions rejected");
    const RECT full{ 0, 0, 5264, 2684 };
    const RECT part{ 0, 0, 2632, 2684 };
    const RECT over{ -8, -8, 5272, 2692 };
    const RECT win{ 0, 0, 1920, 1080 };
    Expect(desktop_mirror::IsFullBackbufferSrc(nullptr, nullptr, 5264, 2684), "null rects proceed to mirror");
    Expect(desktop_mirror::IsFullBackbufferSrc(&full, nullptr, 5264, 2684), "full-bb src rect proceeds to mirror");
    Expect(desktop_mirror::IsFullBackbufferSrc(&over, nullptr, 5264, 2684), "oversized src rect proceeds to mirror");
    Expect(!desktop_mirror::IsFullBackbufferSrc(&part, nullptr, 5264, 2684), "half-bb src rect stays native");
    Expect(!desktop_mirror::IsFullBackbufferSrc(nullptr, &win, 5264, 2684), "any dst rect stays native");
    Expect(!desktop_mirror::IsFullBackbufferSrc(&full, &win, 5264, 2684), "full src with dst stays native");
    RECT vdst{};
    Expect(desktop_mirror::VisibleEyeDst(0, 0, 1920, 1080, 5264, 2684, 5264, 2684, 2632, 2684, vdst) &&
           vdst.left == 430 && vdst.top == 0 && vdst.right == 1489 && vdst.bottom == 1080,
           "1080p window corner shows aspect-fit eye");
    RECT fdst{}, vfull{};
    Expect(desktop_mirror::FitDestination(5264, 2684, 2632, 2684, 5264, 2684, fdst) &&
           desktop_mirror::VisibleEyeDst(0, 0, 5264, 2684, 5264, 2684, 5264, 2684, 2632, 2684, vfull) &&
           vfull.left == fdst.left && vfull.top == fdst.top &&
           vfull.right == fdst.right && vfull.bottom == fdst.bottom,
           "fully visible window matches legacy centering");
    Expect(desktop_mirror::VisibleEyeDst(0, 0, 0, 0, 5264, 2684, 5264, 2684, 2632, 2684, vdst),
           "empty visible box falls back without failing");
    RECT windowFit{};
    Expect(desktop_mirror::FitDestination(5264,2684,2632,2684,1920,1080,windowFit) &&
           desktop_mirror::VisibleEyeDst(0, 0, 1920, 1080, 5264, 2684, 1920, 1080, 2632, 2684, vdst) &&
           EqualRect(&windowFit,&vdst),
           "differently sized window retains full-window aspect fit (old implementation fails)");
    for (UINT ww : {800u, 1280u, 1920u, 2560u}) for (UINT wh : {600u, 720u, 1080u, 1440u}) {
        RECT a{}, b{};
        Expect(desktop_mirror::FitDestination(5264,2684,2632,2684,ww,wh,a) &&
               desktop_mirror::VisibleEyeDst(0,0,(LONG)ww,(LONG)wh,5264,2684,ww,wh,2632,2684,b) &&
               EqualRect(&a,&b), "visible full client equals direct fit for arbitrary monitor shape");
    }
    RECT work{0,0,1920,1040}, outer{};
    Expect(desktop_mirror::WindowBox(work,16,39,outer) && EqualRect(&work,&outer),
           "1080p work area includes title/borders/taskbar, not oversized SBS");
    work={-3840,100,0,2220};
    Expect(desktop_mirror::WindowBox(work,16,39,outer) && outer.right-outer.left==1936 &&
           outer.bottom-outer.top==1119 && outer.left>=work.left && outer.right<=work.right,
           "4K second monitor negative origin caps physical client at 1920x1080");
    work={1920,-600,3200,120};
    Expect(desktop_mirror::WindowBox(work,16,39,outer) && EqualRect(&work,&outer),
           "small monitor is fully contained including negative Y");
    work={0,0,10,10};
    Expect(!desktop_mirror::WindowBox(work,16,39,outer), "invalid work area rejected");
    RECT mdst{}, msrc{}; bool useSrc = false, needClr = true;
    Expect(desktop_mirror::ComputeMirrorDestination(5264, 2684, 2632, 2684, 5264, 2684, 0, false, 0, 0, 0, 0, mdst, msrc, useSrc, needClr) &&
           needClr && !useSrc && mdst.left == fdst.left && mdst.right == fdst.right,
           "ComputeMirrorDestination mode 0 matches legacy fit");
    Expect(desktop_mirror::ComputeMirrorDestination(5264, 2684, 2632, 2684, 1920, 1080, 1, false, 0, 0, 0, 0, mdst, msrc, useSrc, needClr) &&
           !needClr && !useSrc && mdst.left == 0 && mdst.top == 0 && mdst.right == 5264 && mdst.bottom == 2684,
           "ComputeMirrorDestination mode 1 stretches to full backbuffer");
    Expect(desktop_mirror::ComputeMirrorDestination(5264, 2684, 2632, 2684, 1920, 1080, 2, false, 0, 0, 0, 0, mdst, msrc, useSrc, needClr) &&
           !needClr && useSrc && mdst.left == 0 && mdst.top == 0 && mdst.right == 5264 && mdst.bottom == 2684,
           "ComputeMirrorDestination mode 2 fills full backbuffer with cropped scratch");
    const double cropAspect = double(msrc.right - msrc.left) / double(msrc.bottom - msrc.top);
    const double targetAspect = 5264.0 / 2684.0;
    Expect(std::fabs(cropAspect - targetAspect) < .002,
           "ComputeMirrorDestination mode 2 preserves target aspect ratio");
    // Reset guard: the window fit stays display-only, VR keeps SteamVR SBS.
    // Trusted 5544x2828 SBS, fitted client 1920x1080.
    using desktop_mirror::DecideResetSize;
    Expect(!DecideResetSize(5544, 2828, 5544, 2828, 1920, 1080, true, true).enforced,
           "identical SBS passes untouched");
    Expect(!DecideResetSize(0, 0, 0, 0, 1920, 1080, true, true).enforced,
           "no trusted size passes through (fail-open)");
    Expect(!DecideResetSize(1920, 1080, 5544, 2828, 1920, 1080, false, true).enforced,
           "fit off passes the window size");
    Expect(!DecideResetSize(1920, 1080, 5544, 2828, 1920, 1080, true, false).enforced,
           "flat mode passes the window size");
    {
        const auto d = DecideResetSize(1920, 1080, 5544, 2828, 1920, 1080, true, true);
        Expect(d.enforced && d.w == 5544 && d.h == 2828,
               "window-following shrink redirected to SBS");
    }
    {
        const auto d = DecideResetSize(0, 0, 5544, 2828, 1920, 1080, true, true);
        Expect(d.enforced && d.w == 5544 && d.h == 2828,
               "zero (client-sized) request redirected to SBS");
    }
    {
        const auto d = DecideResetSize(1280, 720, 5544, 2828, 1920, 1080, true, true);
        Expect(!d.enforced && d.w == 1280 && d.h == 720,
               "explicit smaller choice kept, not fought");
    }
    {
        const auto d = DecideResetSize(6000, 3000, 5544, 2828, 1920, 1080, true, true);
        Expect(!d.enforced,
               "explicit larger size passes for the copy path to follow");
    }
    {
        const auto d = DecideResetSize(0, 0, 5544, 2828, 0, 0, true, true);
        Expect(!d.enforced,
               "unreadable client cannot judge a zero request");
    }
    // Stamp gate: same-size/format RT0 receives mirror pixels, else skipped.
    {
        D3DSURFACE_DESC a{}, b{};
        a.Width = b.Width = 640; a.Height = b.Height = 360;
        a.Format = b.Format = D3DFMT_A8R8G8B8;
        Expect(desktop_mirror::ShouldStampRt(a, b), "same RT/backbuffer stamps");
        b.Width = 320;
        Expect(!desktop_mirror::ShouldStampRt(a, b), "size mismatch skips");
        b.Width = 640; b.Format = D3DFMT_X8R8G8B8;
        Expect(!desktop_mirror::ShouldStampRt(a, b), "format mismatch skips");
    }
    return g_fails == 0 ? 0 : 1;
}
