// Tests the PC-label -> PS-icon mapping used by the Scaleform rewrite.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../../spikes/view_matrix/prompt_label_rewrite.h"

using namespace promptroles;

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}

int main() {
    Check(std::strcmp(IconForLabel("Enter"), "Cross") == 0, "Enter -> Cross");
    Check(std::strcmp(IconForLabel("Esc"), "Circle") == 0, "Esc -> Circle");
    Check(std::strcmp(IconForLabel("Left Shift / Pad 9"), "L3") == 0, "Sprint row -> L3");
    Check(std::strcmp(IconForLabel("Q / Pad 5"), "Square") == 0, "Age row -> Square");
    Check(std::strcmp(IconForLabel("Mouse 3 / Pad 6"), "R1") == 0, "Impulse row -> R1");
    Check(std::strcmp(IconForLabel("E / Pad 10"), "R3") == 0, "Anti-Gravity row -> R3");
    Check(std::strcmp(IconForLabel("H / Pad 11"), "DpadUp") == 0, "Health row -> DpadUp");
    Check(IconForLabel("totally unrelated") == nullptr, "unknown stays text");
    Check(std::strcmp(IconForLabel("Use Health Pack"), "DpadUp") == 0, "longest match wins");
    if (g_fail) { std::fprintf(stderr, "label-rewrite tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: PC label -> PS icon mapping");
    return 0;
}
