#include "../desktop_mirror.h"
#include <cstdio>
#include <cmath>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    Expect(desktop_mirror::SourceLeft(5264, 2, false, false) == 2632, "right eye is the second half");
    Expect(desktop_mirror::SourceLeft(5264, 2, true, false) == 0, "swapped right eye is the first half");
    Expect(desktop_mirror::SourceLeft(5264, 1, false, false) == 0, "left eye is the first half");
    Expect(desktop_mirror::SourceLeft(5264, 1, true, false) == 2632, "swapped left eye is the second half");
    Expect(desktop_mirror::SourceLeft(5264, 2, false, true) == 0, "cropped frame keeps the repeated half");
    Expect(desktop_mirror::SourceLeft(5264, 2, true, true) == 2632, "cropped swap keeps the other repeated half");
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
    return g_fails == 0 ? 0 : 1;
}
