#include "../arm_ik_session.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
bool close(float a, float b, float e = 1e-3f) { return std::fabs(a - b) < e; }
float dot(armik::V a, armik::V b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

void identity(float m[4][4]) {
    std::memset(m, 0, sizeof(float) * 16);
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
}

void authored_bends() {
    armik::V synthetic{};
    assert(armnatural::PreferredBend({0, 0, 0}, {0.2f, 1.0f, 0}, {1, 0, 0}, synthetic));
    assert(close(synthetic.x, 0.f) && close(synthetic.y, 1.f) && close(synthetic.z, 0.f));

    const auto& bind = armnatural::TmdBind();
    assert(bind.ready);
    armik::V left{}, right{};
    assert(armnatural::PreferredBend(armbind::Position(bind.bind[7]), armbind::Position(bind.bind[8]),
                                     armbind::Position(bind.bind[9]), left));
    assert(armnatural::PreferredBend(armbind::Position(bind.bind[49]), armbind::Position(bind.bind[50]),
                                     armbind::Position(bind.bind[51]), right));
    assert(close(left.x, -0.5809f, 0.02f) && close(left.y, 0.8132f, 0.02f) && close(left.z, -0.0363f, 0.02f));
    assert(close(right.x, 0.3110f, 0.02f) && close(right.y, 0.3539f, 0.02f) && close(right.z, 0.8821f, 0.02f));
    assert(std::fabs(left.z) < 0.9f && std::fabs(right.x) > 0.1f);
    assert(dot(left, right) < 0.95f);
    std::puts("authored bends: synthetic +Y, distinct TMD left/right poles");
}

void pole_disagreement_and_retained_side() {
    const armik::V shoulder{0, 0, 0};
    const armik::V preferred{0, 1, 0};
    const float upper = 0.30f, fore = 0.28f;
    const armik::V targets[] = {{0.20f, 0.05f, 0}, {0.05f, 0.20f, 0}, {0, 0.25f, 0},
                                {0.05f, 0.20f, 0}, {0.20f, 0.05f, 0}};
    armik::State held{};
    armik::V prevBend{};
    float freshPole = 0.f, heldPole = 0.f;
    for (int i = 0; i < 5; ++i) {
        armik::Solution retained{}, freshSol{};
        armik::State fresh{};
        assert(armik::Solve(shoulder, targets[i], upper, fore, preferred, held, retained));
        assert(armik::Solve(shoulder, targets[i], upper, fore, preferred, fresh, freshSol));
        if (i > 0) assert(dot(held.bend, prevBend) > 0.f);
        prevBend = held.bend;
        if (i == 2) {
            freshPole = dot(freshSol.elbow, {1, 0, 0});
            heldPole = dot(retained.elbow, {1, 0, 0});
        }
    }
    assert(freshPole > 0.2f && heldPole < -0.2f);
    std::puts("pole: fresh and retained elbows disagree; retained bend side stays put");
}

void smooth_pole_crossing() {
    const armik::V shoulder{0, 0, 0};
    const armik::V preferred{0, 1, 0};
    armik::State held{};
    armik::V prev{};
    for (int i = 0; i <= 16; ++i) {
        const float a = -1.2f + 2.4f * (float)i / 16.f;
        armik::V target{std::sin(a) * 0.22f, std::cos(a) * 0.22f, 0.f};
        armik::Solution solved{};
        assert(armik::Solve(shoulder, target, 0.30f, 0.28f, preferred, held, solved));
        if (i > 0) assert(dot(held.bend, prev) > 0.f);
        prev = held.bend;
    }
    std::puts("pole crossing: retained bend does not flip");
}

void one_advance_per_frame() {
    armiksession::Session s;
    float id[4][4];
    identity(id);
    armbind::Deltas out{};
    assert(armiksession::Consider(s, 0, true, 4, 1, id, out) == armiksession::Step::Solved);
    assert(armiksession::Consider(s, 1, true, 4, 1, id, out) == armiksession::Step::Solved);
    const unsigned leftAdvances = s.hand[0].advances;
    const unsigned rightAdvances = s.hand[1].advances;
    const armik::V leftBend = s.hand[0].state.bend;
    assert(armiksession::Consider(s, 0, true, 4, 1, id, out) == armiksession::Step::Reuse);
    assert(armiksession::Consider(s, 1, true, 4, 1, id, out) == armiksession::Step::Reuse);
    assert(s.hand[0].advances == leftAdvances && s.hand[1].advances == rightAdvances);
    assert(dot(s.hand[0].state.bend, leftBend) > 0.99f);

    float shifted[4][4];
    identity(shifted);
    shifted[3][0] = 5.f;
    assert(!armiksession::MeshDeltaCompatible(id, shifted));
    assert(armiksession::Consider(s, 0, true, 4, 1, shifted, out) == armiksession::Step::Incompatible);
    assert(s.hand[0].advances == leftAdvances);
    assert(dot(s.hand[0].state.bend, leftBend) > 0.99f);
    std::puts("frame token: second section reuses; a different meshDelta does not advance");
}

void tracking_loss_is_per_hand() {
    armiksession::Session s;
    float id[4][4];
    identity(id);
    armbind::Deltas out{};
    assert(armiksession::Consider(s, 0, true, 1, 1, id, out) == armiksession::Step::Solved);
    assert(armiksession::Consider(s, 1, true, 1, 1, id, out) == armiksession::Step::Solved);
    const armik::V rightBend = s.hand[1].state.bend;
    const unsigned rightAdvances = s.hand[1].advances;
    assert(armiksession::Consider(s, 0, false, 2, 1, id, out) == armiksession::Step::Lost);
    assert(!s.hand[0].state.valid && !s.hand[0].haveResult);
    assert(s.hand[1].state.valid);
    assert(dot(s.hand[1].state.bend, rightBend) > 0.99f);
    assert(armiksession::Consider(s, 1, true, 2, 1, id, out) == armiksession::Step::Solved);
    assert(s.hand[1].advances == rightAdvances + 1);
    assert(s.hand[0].advances == 0);
    assert(armiksession::Consider(s, 0, true, 3, 1, id, out) == armiksession::Step::Solved);
    assert(s.hand[0].state.valid && s.hand[0].advances == 1);

    const armik::V leftBend = s.hand[0].state.bend;
    assert(armiksession::Consider(s, 1, false, 4, 1, id, out) == armiksession::Step::Lost);
    assert(!s.hand[1].state.valid);
    assert(s.hand[0].state.valid && dot(s.hand[0].state.bend, leftBend) > 0.99f);
    std::puts("tracking: losing one hand clears only that hand");
}

void reset_and_failed_solve() {
    armiksession::Session s;
    float id[4][4];
    identity(id);
    armbind::Deltas out{};
    assert(armiksession::Consider(s, 0, true, 1, 1, id, out) == armiksession::Step::Solved);
    assert(armiksession::Consider(s, 1, true, 1, 1, id, out) == armiksession::Step::Solved);
    const armik::State kept = s.hand[0].state;
    float bad[4][4];
    identity(bad);
    bad[3][0] = std::nanf("");
    assert(armiksession::Consider(s, 0, true, 2, 1, bad, out) == armiksession::Step::Failed);
    assert(s.hand[0].state.valid == kept.valid);
    assert(close(s.hand[0].state.bend.x, kept.bend.x) && close(s.hand[0].state.bend.y, kept.bend.y) &&
           close(s.hand[0].state.bend.z, kept.bend.z));
    assert(s.hand[0].advances == 1);
    assert(s.hand[1].state.valid);

    armiksession::Reset(s);
    assert(!s.hand[0].state.valid && !s.hand[1].state.valid);
    assert(!s.hand[0].haveResult && !s.hand[1].haveResult);
    assert(armiksession::Consider(s, 0, true, 3, 1, id, out) == armiksession::Step::Solved);
    assert(s.hand[0].advances == 1 && s.hand[0].state.valid);
    std::puts("reset: failed solve keeps the previous chain; camera reset starts clean");
}

void mode_zero_does_nothing() {
    armiksession::Session s;
    float id[4][4];
    identity(id);
    armbind::Deltas out{};
    assert(armiksession::Consider(s, 0, true, 1, 0, id, out) == armiksession::Step::Disabled);
    assert(armiksession::Consider(s, 1, true, 1, 0, id, out) == armiksession::Step::Disabled);
    assert(!s.hand[0].state.valid && !s.hand[1].state.valid);
    assert(s.hand[0].advances == 0 && s.hand[1].advances == 0);

    assert(armiksession::Consider(s, 0, true, 2, 1, id, out) == armiksession::Step::Solved);
    assert(s.hand[0].state.valid);
    assert(armiksession::Consider(s, 0, true, 3, 0, id, out) == armiksession::Step::Disabled);
    assert(!s.hand[0].state.valid && !s.hand[0].haveResult);
    assert(s.hand[0].advances == 0);
    std::puts("ArmIKMode 0: no solve, and leaving mode 1 drops the chain");
}
}  // namespace

int main() {
    authored_bends();
    pole_disagreement_and_retained_side();
    smooth_pole_crossing();
    one_advance_per_frame();
    tracking_loss_is_per_hand();
    reset_and_failed_solve();
    mode_zero_does_nothing();
    std::puts("PASS");
    return 0;
}
