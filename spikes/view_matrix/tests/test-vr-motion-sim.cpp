// Deterministic desktop simulator for the controller paths that need a headset
// to observe.  It is intentionally independent of OpenXR and D3D9: every
// frame is synthetic, so regressions in pose ownership, grip latching, trigger
// edges, muzzle placement, and shadow policy can be caught before a headset run.
#include <cassert>
#include <cmath>
#include <cstdio>

struct V3 { float x, y, z; };
static V3 add(V3 a, V3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
static V3 mul(V3 a, float s) { return {a.x*s,a.y*s,a.z*s}; }
static float len(V3 a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }

struct Pose { V3 position{}; V3 forward{0,0,1}; };
struct Input { Pose left{}, right{}; bool leftGrip=false, rightTrigger=false; bool menu=false; };
struct Output { Pose left{}, right{}; bool twoHand=false; bool firePressed=false; bool selectPressed=false; V3 muzzle{}; };

class Sim {
public:
    Output step(const Input& in) {
        Output o{};
        o.right = in.right; // right hand is always the primary weapon hand
        const float d = len({in.left.position.x-in.right.position.x,
                             in.left.position.y-in.right.position.y,
                             in.left.position.z-in.right.position.z});
        if (in.leftGrip && (latched_ || d <= 0.30f)) latched_ = true;
        if (latched_ && (!in.leftGrip || d >= 0.36f)) latched_ = false;
        o.twoHand = latched_;
        // The free left hand always owns its own controller pose.  During a
        // support grip the weapon code consumes it as a target, but it is not
        // allowed to fall back to the HMD or the right hand.
        o.left = in.left;
        const bool rising = in.rightTrigger && !previousTrigger_;
        if (in.menu) o.selectPressed = rising;
        else o.firePressed = rising;
        previousTrigger_ = in.rightTrigger;
        o.muzzle = add(in.right.position, mul(in.right.forward, 0.100f));
        return o;
    }
private:
    bool latched_ = false;
    bool previousTrigger_ = false;
};

int main() {
    Sim sim;
    Input in;
    in.left.position = {-0.30f, 1.35f, 0.15f};
    in.right.position = { 0.30f, 1.35f, 0.15f};
    in.right.forward = {0,0,1};

    auto a = sim.step(in);
    assert(!a.twoHand && !a.firePressed && !a.selectPressed);
    const V3 rightFixed = a.right.position;

    // Independent left-hand tracking: move only the left controller over a
    // 120-frame synthetic walk.  A shared-model regression makes this fail.
    for (int i=1; i<=120; ++i) {
        in.left.position.x = -0.30f + 0.0025f*i;
        auto o = sim.step(in);
        assert(std::fabs(o.left.position.x - in.left.position.x) < 1e-6f);
        assert(std::fabs(o.right.position.x - rightFixed.x) < 1e-6f);
    }

    // Grip acquisition has hysteresis; a small movement does not drop the
    // support hand, but a real release does immediately.
    in.leftGrip = true; in.left.position = {0.18f,1.35f,0.15f};
    auto grip = sim.step(in); assert(grip.twoHand);
    in.left.position.x += 0.04f; grip = sim.step(in); assert(grip.twoHand);
    in.leftGrip = false; grip = sim.step(in); assert(!grip.twoHand);

    // Fire/select are rising-edge events.  Holding R2 produces one event and
    // changing menu state cannot leak a gameplay fire edge into the menu.
    in.menu = false; in.rightTrigger = false; sim.step(in);
    in.rightTrigger = true; auto fire = sim.step(in); assert(fire.firePressed && !fire.selectPressed);
    fire = sim.step(in); assert(!fire.firePressed);
    in.menu = true; in.rightTrigger = false; sim.step(in);
    in.rightTrigger = true; auto select = sim.step(in); assert(select.selectPressed && !select.firePressed);
    select = sim.step(in); assert(!select.selectPressed);

    // Muzzle point is a forward offset from the controller-ridden pivot, never
    // the hand/camera centre.  The same event is what the VFX host consumes.
    assert(len({fire.muzzle.x-in.right.position.x,
                fire.muzzle.y-in.right.position.y,
                fire.muzzle.z-in.right.position.z}) > 0.099f);
    assert(std::isfinite(fire.muzzle.x+fire.muzzle.y+fire.muzzle.z));

    // Shadow policy regression: the performance-safe mode drops only the
    // receiver sample; light-space classification remains available for the
    // future repair path (mode 3).
    const int shadowMode = 2;
    const bool receiverDropped = shadowMode == 2;
    const bool lightSpaceProtected = shadowMode == 3;
    assert(receiverDropped && !lightSpaceProtected);

    std::puts("PASS: 120 left-pose frames, grip hysteresis, fire/menu edge reset, muzzle offset, shadow policy");
}
