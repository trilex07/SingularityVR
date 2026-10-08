// Tests the PS prompt role -> icon mapping against the generated atlas table.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../../../spikes/view_matrix/prompt_roles.h"
#include "../../../spikes/view_matrix/prompt_atlas_generated.h"

using namespace promptroles;

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}

static bool HasIcon(const char* name) {
    for (int i = 0; i < kVrPromptIconCount; ++i)
        if (std::strcmp(kVrPromptIconUv[i].name, name) == 0) return true;
    return false;
}

int main() {
    // Every mapped action must resolve to an icon that exists in the atlas.
    const Action all[] = {
        Action::Confirm, Action::Back, Action::Fire, Action::Aim, Action::Interact,
        Action::Jump, Action::Crouch, Action::Age, Action::CycleWeapon, Action::Impulse,
        Action::Dash, Action::AntiGrav, Action::Flashlight, Action::Health, Action::Pause,
        Action::Create, Action::Move, Action::Look, Action::Touchpad,
    };
    for (Action a : all)
        Check(HasIcon(IconName(a)), "action resolves to an atlas icon");

    // The user-visible contract.
    Check(std::strcmp(IconName(Action::Confirm), "Cross") == 0, "Enter/confirm -> Cross");
    Check(std::strcmp(IconName(Action::Interact), "R1") == 0, "interact -> R1");
    Check(std::strcmp(IconName(Action::Fire), "R2") == 0, "fire -> R2");
    Check(std::strcmp(IconName(Action::Aim), "L2") == 0, "aim -> L2");
    Check(std::strcmp(IconName(Action::Age), "Square") == 0, "age -> Square");
    Check(std::strcmp(IconName(Action::CycleWeapon), "Triangle") == 0, "cycle -> Triangle");
    Check(std::strcmp(IconName(Action::Pause), "Options") == 0, "pause -> Options");

    // Atlas sanity: UVs ordered, in range, one cell each.
    for (int i = 0; i < kVrPromptIconCount; ++i) {
        const VrPromptIconUv& u = kVrPromptIconUv[i];
        Check(u.u0 >= 0.0f && u.u1 <= 1.0001f && u.u1 > u.u0, "icon UV in range");
        Check(u.v0 == 0.0f && u.v1 == 1.0f, "icon spans the full atlas height");
        if (i + 1 < kVrPromptIconCount)
            Check(kVrPromptIconUv[i + 1].u0 >= u.u1 - 1e-6f, "icon UVs do not overlap");
    }
    Check(kVrPromptIconCount == 20, "atlas holds 20 icons");

    if (g_fail) { std::fprintf(stderr, "prompt-role tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: PS prompt role mapping + atlas UV table");
    return 0;
}
