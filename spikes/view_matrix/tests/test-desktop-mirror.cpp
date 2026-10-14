#include "../desktop_mirror.h"
#include <cstdio>

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
    return g_fails == 0 ? 0 : 1;
}
