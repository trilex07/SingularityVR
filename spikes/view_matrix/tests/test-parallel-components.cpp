#include "../fake_xr.h"
#include "../recenter.h"
#include "../grip_latch.h"
#include "../input_chord.h"
#include "../ui_pointer.h"
#include "../fp_shadow.h"
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {
bool close(float a, float b, float e = 1e-4f) { return std::fabs(a - b) < e; }

// Yaw out of a yaw-only quaternion (fake_xr emits YawQuat rotations).
float QuatYaw(const fakexr::Quat& q) { return 2.0f * std::atan2(q.y, q.w); }
fakexr::Vec QForward(const fakexr::Quat& q) {
    // Rotate (0,0,-1) by yaw-only q.
    const float yaw = QuatYaw(q);
    return fakexr::Vec{(float)-std::sin(yaw), 0, (float)-std::cos(yaw)};
}

void grip_latch_tests() {
    using namespace griplatch;
    GripLatch latch;
    const std::uint32_t pistol = 1, rifle = 2;
    std::uint64_t t = 1000;
    // Press outside radius -> no latch.
    assert(latch.Update(true, 0.30f, true, pistol, t) == State::Unlatched);
    // Move inside radius while held -> latch.
    t += 16;
    assert(latch.Update(true, 0.10f, true, pistol, t) == State::Latched);
    // Move far away while still held -> STILL LATCHED.
    t += 16;
    assert(latch.Update(true, 0.80f, true, pistol, t) == State::Latched);
    // Grip release -> unlatch.
    t += 16;
    assert(latch.Update(false, 0.05f, true, pistol, t) == State::Unlatched);
    // Re-latch, then brief tracking loss -> grace (preserved).
    t += 16;
    assert(latch.Update(true, 0.10f, true, pistol, t) == State::Latched);
    t += 16;
    assert(latch.Update(true, 0.10f, false, pistol, t) == State::Grace);
    t += 100;
    assert(latch.Update(true, 0.10f, false, pistol, t) == State::Grace);
    // Restored while held -> latch preserved.
    t += 100;
    assert(latch.Update(true, 0.40f, true, pistol, t) == State::Latched);
    // Long loss -> timeout unlatch.
    t += 16;
    assert(latch.Update(true, 0.10f, false, pistol, t) == State::Grace);
    t += 600;
    assert(latch.Update(true, 0.10f, false, pistol, t) == State::Unlatched);
    // Re-latch, then weapon switch with the new support far away -> unlatch.
    t += 16;
    assert(latch.Update(true, 0.05f, true, pistol, t) == State::Latched);
    t += 16;
    assert(latch.Update(true, 0.50f, true, rifle, t) == State::Unlatched);
    // New support in radius while still held -> re-acquire under new weapon
    // (same rule as "move inside radius while held").
    t += 16;
    assert(latch.Update(true, 0.05f, true, rifle, t) == State::Latched);
    // Release-then-press requires re-acquire (no sticky latch).
    t += 16;
    assert(latch.Update(false, 0.05f, true, rifle, t) == State::Unlatched);
    t += 16;
    assert(latch.Update(true, 0.30f, true, rifle, t) == State::Unlatched);
    std::puts("grip: outside/inside/far-held/release/grace/restore/timeout/switch");
}

void recenter_regression_tests() {
    using namespace vrrecenter;
    // Case 1: head yaw +30 deg -> recenter -> local yaw ~0.
    // Case 2: head yaw -90 deg -> recenter -> local yaw ~0.
    for (float targetYaw : {30.0f * 3.14159265f / 180.0f, -90.0f * 3.14159265f / 180.0f}) {
        fakexr::Rig rig(11);
        rig.SetHeadTurnRate(targetYaw / 2.0f); // reach targetYaw in 2 s
        fakexr::Frame f{};
        for (int i = 0; i < 180; ++i) f = rig.Step();
        const float yaw = QuatYaw(f.hmd.orientation);
        assert(close(yaw, targetYaw, 0.02f));
        V head{f.hmd.position.x, f.hmd.position.y, f.hmd.position.z};
        Stage s = RecenterForHead(head, yaw);
        const V h = Apply(s, head);
        assert(close(h.x, 0, 1e-4f) && close(h.z, 0, 1e-4f));
        const V fwd = Apply(s, head + Forward(yaw));
        assert(close(YawOf(V{fwd.x - h.x, 0, fwd.z - h.z}), 0, 1e-3f));
        // Relative controller-to-HMD transforms unchanged (position only;
        // orientation is yaw-shifted rigidly, verified via vector length).
        for (const fakexr::Pose* c : {&f.left, &f.right}) {
            V cpos{c->position.x, c->position.y, c->position.z};
            const V before{cpos.x - head.x, cpos.y - head.y, cpos.z - head.z};
            const V ac = Apply(s, cpos);
            const V after{ac.x - h.x, ac.y - h.y, ac.z - h.z};
            const float lb = std::sqrt(before.x * before.x + before.y * before.y +
                                       before.z * before.z);
            const float la = std::sqrt(after.x * after.x + after.y * after.y +
                                       after.z * after.z);
            assert(close(lb, la, 1e-4f));
        }
    }
    // Case 3: physical offset (+0.25 X, +0.15 Z) -> center corrected.
    {
        fakexr::Rig rig(5);
        fakexr::Frame f = rig.Step();
        V head{f.hmd.position.x + 0.25f, f.hmd.position.y, f.hmd.position.z + 0.15f};
        Stage s = RecenterForHead(head, 0.0f);
        const V h = Apply(s, head);
        assert(close(h.x, 0, 1e-5f) && close(h.z, 0, 1e-5f) &&
               close(h.y, f.hmd.position.y, 1e-6f));
    }
    std::puts("recenter: +30deg/-90deg yaw zeroing, offset correction, relative invariance");
}

void chord_tests() {
    using namespace inputchord;
    StickChord chord;
    std::uint64_t t = 5000;
    // Singles do nothing.
    assert(!chord.Update(true, false, t));
    t += 50;
    assert(!chord.Update(false, false, t)); // release rearms
    assert(!chord.Update(false, true, t += 50));
    assert(!chord.Update(false, false, t += 50));
    // L3 then R3 inside window -> exactly one event.
    assert(!chord.Update(true, false, t += 50));
    assert(chord.Update(true, true, t += 100));
    assert(!chord.Update(true, true, t += 100)); // hold: no repeat
    assert(!chord.Update(true, true, t += 500));
    assert(!chord.Update(false, false, t += 50)); // release rearms
    // R3 then L3 inside window -> exactly one event.
    assert(!chord.Update(false, true, t += 50));
    assert(chord.Update(true, true, t += 100));
    assert(!chord.Update(false, false, t += 50));
    // Simultaneous press -> one event.
    assert(chord.Update(true, true, t += 50));
    assert(!chord.Update(false, false, t += 50));
    // First released before second -> cancel, no event.
    assert(!chord.Update(true, false, t += 50));
    assert(!chord.Update(false, false, t += 50));
    assert(!chord.Update(false, true, t += 50));
    assert(!chord.Update(false, false, t += 50));
    // Stale first press: second arrives after window -> no event, no repeat.
    assert(!chord.Update(true, false, t += 50));
    assert(!chord.Update(true, true, t += 500));
    assert(!chord.Update(true, true, t += 50));
    assert(!chord.Update(false, false, t += 50));
    // After rearm the chord works again.
    assert(!chord.Update(true, false, t += 50));
    assert(chord.Update(true, true, t += 50));
    assert(!chord.Update(false, false, t += 50));
    std::puts("chord: singles/cancel/order/window/hold-no-repeat/rearm");
}

void menu_harness_tests() {
    using namespace uipointer;
    // Head-relative plane from a fake HMD frame; controller ray from the
    // fake right hand; verify top/center/bottom hits map to matching canvas
    // UVs and agree across both eyes (visual == click for every row).
    fakexr::Rig rig(21);
    int samples = 0;
    for (int step = 0; step < 60; ++step) {
        fakexr::Frame f = rig.Step();
        const V head{f.hmd.position.x, f.hmd.position.y, f.hmd.position.z};
        const float yaw = QuatYaw(f.hmd.orientation);
        const float cy = std::cos(yaw), sy = std::sin(yaw);
        // Basis vectors rotated by head yaw (forward -Z at yaw 0).
        const V fwd{-sy, 0, -cy}, right{cy, 0, -sy}, up{0, 1, 0};
        Plane plane;
        assert(HeadPlane(head, fwd, right, up, 1.2f, 1.04f, 1.06f, 0.70f, plane));
        // Controller ray: right-hand position toward the panel point.
        const V origin{f.right.position.x, f.right.position.y, f.right.position.z};
        for (float v : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
            const V target =
                plane.centre + plane.up * ((0.5f - v) * 2 * plane.halfHeight * 0.70f);
            const V dir = target - origin;
            Hit hit;
            assert(Intersect(origin, dir, plane, 1920, 1080, hit));
            assert(close(hit.u, 0.5f) && close(hit.v, v, 2e-4f));
            // Both eyes agree on the same canvas coordinate.
            for (int eye = 0; eye < 2; ++eye) {
                const V eyePos = head + right * (eye ? 0.032f : -0.032f);
                ScreenPoint sp;
                assert(ProjectPoint(hit.world, eyePos, fwd, right, up, 1.04f, 1.06f, sp));
                assert(close(sp.u, 0.5f, 0.02f)); // IPD shift only, same item
            }
            ++samples;
        }
    }
    std::printf("menu: %d top/center/bottom row hits, per-eye agreement\n", samples);
}

void fp_shadow_tests() {
    using namespace fpshadow;
    const unsigned kSig[] = {5799, 3314, 390};
    const char vbA = 0, ibA = 0, vbB = 0;
    DrawKey color{};
    color.verts = 5799;
    color.first = 0;
    color.prims = 1000;
    color.vb = &vbA;
    color.ib = &ibA;
    color.stride = 32;
    color.layoutShadow = false;
    color.rtIsScene = true;
    // 1. FP color draw -> false.
    assert(!IsFpShadowCasterDraw(color, kSig, 3, &color, 1));
    DrawKey world = color;
    world.vb = &vbB; // different mesh, same count
    world.layoutShadow = true;
    world.rtIsScene = false;
    // 2. World shadow (count collides, geometry differs) -> false.
    assert(!IsFpShadowCasterDraw(world, kSig, 3, &color, 1));
    DrawKey fpShadow = color;
    fpShadow.layoutShadow = true;
    fpShadow.rtIsScene = false;
    // 3. FP shadow sibling (same geometry tuple) -> true.
    assert(IsFpShadowCasterDraw(fpShadow, kSig, 3, &color, 1));
    // 4. Same verts, different range -> false (run-215 shape).
    DrawKey other = fpShadow;
    other.startIndex = 42;
    assert(!IsFpShadowCasterDraw(other, kSig, 3, &color, 1));
    // 5. Offscreen but color shader (depth-prime) -> false.
    DrawKey prime = color;
    prime.rtIsScene = false;
    assert(!IsFpShadowCasterDraw(prime, kSig, 3, &color, 1));
    // 6. Empty signature set -> always false.
    assert(!IsFpShadowCasterDraw(fpShadow, nullptr, 0, &color, 1));
    assert(!IsFpShadowCasterDraw(fpShadow, kSig, 0, &color, 1));
    std::puts("fp_shadow: color/world/sibling/range/prime/empty-set cases");
}
} // namespace

int main() {
    grip_latch_tests();
    recenter_regression_tests();
    chord_tests();
    menu_harness_tests();
    fp_shadow_tests();
    std::puts("PASS");
    return 0;
}
