#include "../context_prompt.h"
#include <cassert>
#include <cstdio>

namespace {
void context_prompt_tests() {
    using namespace ctxprompt;
    Tracker tr;
    tr.SetShowFrames(1);
    tr.SetHideFrames(1); // immediate latch for test
    // Hidden -> no icon state
    assert(!tr.Get().visible && !ShowIcon(tr.Get()));
    // Observe prompt appearing (1 frame to latch)
    tr.Observe(true, 0.0f, -0.14f, Action::Interact);
    assert(tr.Get().visible);
    assert(tr.Get().action == Action::Interact);
    assert(!tr.Get().actionVerified); // icon still off
    assert(!ShowIcon(tr.Get()));
    // Action change while visible -> generation bump, actionVerified=false
    const auto gen1 = tr.Get().generation;
    tr.Observe(true, 0.0f, -0.14f, Action::Reload);
    assert(tr.Get().visible && tr.Get().action == Action::Reload);
    assert(!tr.Get().actionVerified);
    assert(tr.Get().generation > gen1);
    // Hide -> immediate invisible, generation bump
    tr.Observe(false, 0, 0, Action::Unknown);
    assert(!tr.Get().visible);
    assert(tr.Get().generation > gen1 + 1);
    // Re-show works
    tr.Observe(true, 0.0f, -0.14f, Action::Interact);
    assert(tr.Get().visible && tr.Get().action == Action::Interact);
    // Exe confirms action -> icon enabled
    tr.ConfirmAction(Action::Interact);
    assert(tr.Get().actionVerified && ShowIcon(tr.Get()));
    // Confirm with wrong action -> no-op
    tr.ConfirmAction(Action::Reload);
    assert(tr.Get().actionVerified && tr.Get().action == Action::Interact);
    // OnHidden() when visible -> immediate hidden + generation bump
    tr.OnHidden();
    assert(!tr.Get().visible);
    std::puts("context_prompt: hidden/shown/change/hide/reshow/confirm/exe-mismatch/onhidden");
}
} // namespace

int main() {
    context_prompt_tests();
    std::puts("PASS");
    return 0;
}