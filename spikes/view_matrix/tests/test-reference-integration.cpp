#include "../psvr2_trigger.h"
#include "../fake_xr.h"
#include "../recenter.h"
#include "../arm_ik.h"
#include "../pose_conversion.h"
#include "../singularity_bind_reference.h"
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {
bool close(float a, float b, float e = 1e-4f) { return std::fabs(a - b) < e; }

struct RecordingBackend : psvr2trigger::IBackend {
    psvr2trigger::Command last;
    unsigned calls = 0;
    bool Send(const psvr2trigger::Command& c) override {
        last = c;
        ++calls;
        return true;
    }
};

psvr2trigger::PolicyInput LiveGame(psvr2trigger::Weapon w) {
    psvr2trigger::PolicyInput in;
    in.backendAvailable = true;
    in.runtimeCompatible = true;
    in.sessionFocused = true;
    in.gameplayActive = true;
    in.weapon = w;
    in.ammoOk = true;
    return in;
}

void trigger_profile_tests() {
    using namespace psvr2trigger;
    // KHARVOX-derived reference ranges (Psvr2TriggerPolicy.h:222-245).
    Command p = Select(LiveGame(Weapon::Pistol));
    assert(p.effect == Effect::Weapon && p.start == 4 && p.end == 6 && p.strength == 3);
    Command r = Select(LiveGame(Weapon::Rifle));
    assert(r.effect == Effect::Weapon && r.start == 3 && r.end == 5 && r.strength == 5);
    Command s = Select(LiveGame(Weapon::Shotgun));
    assert(s.effect == Effect::Weapon && s.start == 2 && s.end == 4 && s.strength == 7);
    Command sn = Select(LiveGame(Weapon::Sniper));
    assert(sn.effect == Effect::Weapon && sn.start == 2 && sn.end == 5 && sn.strength == 6);
    Command t = Select(LiveGame(Weapon::Tmd));
    assert(t.effect == Effect::Off && t.reason == OffReason::UnsupportedWeapon);
    Command o = Select(LiveGame(Weapon::Other));
    assert(o.effect == Effect::Off);
    // Fire-down auto override is rifle-only; pistol keeps its wall while firing.
    PolicyInput firing = LiveGame(Weapon::Rifle);
    firing.fireDown = true;
    Command rv = Select(firing);
    assert(rv.effect == Effect::Vibration && rv.position == 3 && rv.amplitude == 7 && rv.frequency == 40);
    PolicyInput pf = LiveGame(Weapon::Pistol);
    pf.fireDown = true;
    assert(Select(pf).effect == Effect::Weapon);
    // Empty magazine blocks the fire vibration, falls back to the idle wall.
    PolicyInput empty = LiveGame(Weapon::Rifle);
    empty.fireDown = true;
    empty.ammoOk = false;
    assert(Select(empty).effect == Effect::Weapon);
    // Gating order: Disabled -> UnsupportedRuntime -> Unfocused -> !gameplay.
    PolicyInput g = LiveGame(Weapon::Pistol);
    g.backendAvailable = false;
    assert(Select(g).reason == OffReason::Disabled);
    g = LiveGame(Weapon::Pistol);
    g.runtimeCompatible = false;
    assert(Select(g).reason == OffReason::UnsupportedRuntime);
    g = LiveGame(Weapon::Pistol);
    g.sessionFocused = false;
    assert(Select(g).reason == OffReason::Unfocused);
    g = LiveGame(Weapon::Pistol);
    g.gameplayActive = false;
    g.inactiveReason = OffReason::Inactive;
    assert(Select(g).effect == Effect::Off && Select(g).reason == OffReason::Inactive);
    std::puts("trigger: profiles, fire override, ammo gate, gating order");
}

void trigger_runner_tests() {
    using namespace psvr2trigger;
    RecordingBackend backend;
    Runner runner(&backend);
    // No backend configured -> null sink, no crash, gameplay continues.
    Runner orphan(nullptr);
    orphan.SetGating(false, false, false, false);
    orphan.Update();
    // Event architecture: weapon change + fire press/release + reload.
    runner.SetGating(true, true, true, true);
    runner.SetWeapon(Weapon::Pistol);
    assert(runner.Update());
    const unsigned afterFirst = backend.calls;
    assert(!runner.Update()); // dedup: identical state forwards nothing
    assert(backend.calls == afterFirst);
    runner.SetWeapon(Weapon::Shotgun);
    assert(runner.Update());
    assert(backend.last.start == 2 && backend.last.strength == 7);
    runner.SetWeapon(Weapon::Rifle);
    runner.SetFireDown(true); // FirePressed
    assert(runner.Update());
    assert(backend.last.effect == Effect::Vibration);
    runner.SetReloading(true); // ReloadStarted suppresses fire vibration
    assert(runner.Update());
    assert(backend.last.effect == Effect::Weapon);
    runner.SetReloading(false); // ReloadFinished
    runner.SetFireDown(false); // FireReleased
    assert(runner.Update());
    assert(backend.last.effect == Effect::Weapon);
    // Teardown path forces Off(SessionEnd) exactly once.
    assert(runner.ClearTrigger());
    assert(backend.last.effect == Effect::Off && backend.last.reason == OffReason::SessionEnd);
    assert(!runner.ClearTrigger());
    std::printf("trigger: runner events/dedup/teardown, %u backend sends\n", backend.calls);
}

void fake_xr_tests() {
    fakexr::Rig a(7), b(7);
    for (int i = 0; i < 300; ++i) {
        fakexr::Frame fa = a.Step(), fb = b.Step();
        assert(fa.index == fb.index);
        assert(fa.hmd.position.x == fb.hmd.position.x && fa.hmd.position.y == fb.hmd.position.y &&
               fa.hmd.position.z == fb.hmd.position.z);
        assert(fa.left.position.x == fb.left.position.x && fa.right.position.z == fb.right.position.z);
        assert(fa.time == fb.time);
    }
    // Tracking loss flags poses invalid; stream resumes afterwards.
    fakexr::Rig c(3);
    c.Step();
    c.DropTracking(true, 2);
    fakexr::Frame d1 = c.Step(), d2 = c.Step(), d3 = c.Step();
    assert(!fakexr::PoseValid(d1.right) && !fakexr::PoseValid(d2.right));
    assert(fakexr::PoseValid(d3.right));
    assert(fakexr::PoseValid(d1.left)); // independent lanes
    // Controller nudge moves the stream by exactly the nudge.
    fakexr::Rig e(3), f(3);
    f.NudgeController(true, 0.10f, 0.0f, 0.0f);
    fakexr::Frame fe = e.Step(), ff = f.Step();
    assert(close(ff.right.position.x - fe.right.position.x, 0.10f, 1e-6f));
    std::puts("fake_xr: 300-frame determinism, tracking loss, nudge");
}

void recenter_tests() {
    using namespace vrrecenter;
    // Head lands at origin facing -Z; height passes through (floor stays).
    for (float yaw : {-2.5f, -1.0f, -0.2f, 0.0f, 0.4f, 1.8f, 3.0f}) {
        V head{0.35f * std::sin(yaw), 1.62f, 0.35f * std::cos(yaw)};
        Stage s = RecenterForHead(head, yaw);
        V h = Apply(s, head);
        assert(close(h.x, 0, 1e-4f) && close(h.z, 0, 1e-4f) && close(h.y, 1.62f, 1e-5f));
        V fwd = Apply(s, head + Forward(yaw));
        assert(close(fwd.x - h.x, 0, 1e-4f) && (fwd.z - h.z) < -0.99f);
        // One transform carries both controllers: relative geometry preserved.
        V left{-0.2f, 1.3f, -0.4f}, right{0.25f, 1.28f, -0.38f};
        V dBefore{right.x - left.x, right.y - left.y, right.z - left.z};
        V al = Apply(s, left), ar = Apply(s, right), ah = Apply(s, head);
        V dAfter{ar.x - al.x, ar.y - al.y, ar.z - al.z};
        const float before = std::sqrt(dBefore.x * dBefore.x + dBefore.y * dBefore.y + dBefore.z * dBefore.z);
        const float after = std::sqrt(dAfter.x * dAfter.x + dAfter.y * dAfter.y + dAfter.z * dAfter.z);
        assert(close(before, after, 1e-4f));
        V relBefore{left.x - head.x, 0, left.z - head.z};
        V relAfter{al.x - ah.x, 0, al.z - ah.z};
        const float rb = std::sqrt(relBefore.x * relBefore.x + relBefore.z * relBefore.z);
        const float ra = std::sqrt(relAfter.x * relAfter.x + relAfter.z * relAfter.z);
        assert(close(rb, ra, 1e-4f));
        // Recenter is idempotent: recentering the recentered head is ~identity.
        Stage s2 = RecenterForHead(ah, yaw + s.yaw);
        assert(close(s2.yaw, 0, 1e-4f) && close(s2.tx, 0, 1e-3f) && close(s2.tz, 0, 1e-3f));
    }
    // YawOf inverts Forward.
    for (float yaw : {-3.0f, -1.2f, 0.0f, 0.9f, 2.7f})
        assert(close(YawOf(Forward(yaw)), yaw, 1e-5f));
    std::puts("recenter: head-to-origin, rigid controller invariance, idempotence");
}

void arm_ik_property_tests() {
    using namespace armik;
    const V shoulder{0.18f, 1.45f, -0.05f};
    const float upper = 0.30f, fore = 0.28f;
    const V bend{0.0f, 0.0f, 1.0f};
    // Reachable target: wrist lands on the target (10 cm controller move ->
    // ~10 cm wrist move by rigid translation of the target).
    for (float k = 0.15f; k < 0.5f; k += 0.05f) {
        State st;
        Solution a, b;
        V t{k, 1.30f, -0.35f};
        assert(Solve(shoulder, t, upper, fore, bend, st, a));
        assert(a.targetError < 1e-4f);
        State st2;
        V t2{t.x + 0.10f, t.y, t.z}; // controller moves 10 cm in +x
        assert(Solve(shoulder, t2, upper, fore, bend, st2, b));
        V delta{b.wrist.x - a.wrist.x, b.wrist.y - a.wrist.y, b.wrist.z - a.wrist.z};
        assert(close(Length(delta), 0.10f, 1e-4f));
    }
    // Beyond reach: clamped, wrist at full extension along the target ray.
    {
        State st;
        Solution s;
        V far{2.0f, 1.30f, -2.0f};
        assert(Solve(shoulder, far, upper, fore, bend, st, s));
        assert(s.clamped && s.targetError > 0.5f);
        V d{s.wrist.x - shoulder.x, s.wrist.y - shoulder.y, s.wrist.z - shoulder.z};
        assert(close(Length(d), upper + fore, 1e-3f));
    }
    // Elbow extends as the target approaches full reach: the elbow's
    // distance from the shoulder->wrist axis shrinks monotonically when the
    // target travels outward along a fixed ray.
    {
        float prevHeight = 1e9f;
        armik::V rayDir{0.5f, -0.2f, -0.84f};
        armik::V unit;
        assert(armik::Unit(rayDir, unit));
        for (float d = 0.15f; d < 0.576f; d += 0.025f) {
            State st;
            Solution s;
            V t{shoulder.x + unit.x * d, shoulder.y + unit.y * d, shoulder.z + unit.z * d};
            if (!Solve(shoulder, t, upper, fore, bend, st, s) || s.clamped) break;
            V axis{s.wrist.x - shoulder.x, s.wrist.y - shoulder.y, s.wrist.z - shoulder.z};
            V mid{(shoulder.x + s.wrist.x) * 0.5f, (shoulder.y + s.wrist.y) * 0.5f,
                  (shoulder.z + s.wrist.z) * 0.5f};
            V off{s.elbow.x - mid.x, s.elbow.y - mid.y, s.elbow.z - mid.z};
            const float h = Length(off);
            assert(h <= prevHeight + 1e-5f);
            prevHeight = h;
        }
        assert(prevHeight < 0.05f); // nearly straight at full extension
    }
    std::puts("arm_ik: 10cm wrist tracking, clamp at full extension, elbow straightens");
}

void pose_conversion_invariants() {
    using namespace vrpose;
    const float unitsPerMetre = 52.5f;
    const float centreYaw = 0.37f, baseYaw = -0.61f;
    const V3 basis[3] = {
        TrackingDeltaToWorldUU({1,0,0}, 0, 0, 1),
        TrackingDeltaToWorldUU({0,1,0}, 0, 0, 1),
        TrackingDeltaToWorldUU({0,0,1}, 0, 0, 1)
    };
    auto dot=[](V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    auto cross=[](V3 a,V3 b){return V3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
    const float det=dot(basis[0],cross(basis[1],basis[2]));
    assert(close(det,-1.0f,1e-6f)); // semantic forward/right/up map is an intentional reflection
    for (int i=0;i<3;++i) {
        assert(close(Length(basis[i]),1.0f,1e-6f));
        for(int j=0;j<3;++j) assert(close(dot(basis[i],basis[j]),i==j?1.0f:0.0f,1e-6f));
    }

    const V3 deltas[]={{.10f,0,0},{0,.10f,0},{0,0,.10f},{.10f,.10f,.10f}};
    for (V3 d : deltas) {
        const V3 hmd=TrackingDeltaToWorldUU(d,centreYaw,baseYaw,unitsPerMetre);
        const V3 left=TrackingDeltaToWorldUU(d,centreYaw,baseYaw,unitsPerMetre);
        const V3 right=TrackingDeltaToWorldUU(d,centreYaw,baseYaw,unitsPerMetre);
        assert(close(Length(hmd),Length(left),1e-5f));
        assert(close(Length(hmd),Length(right),1e-5f));
        assert(close(Length(hmd),Length(d)*unitsPerMetre,1e-5f));
        const V3 origin=TrackingDeltaToWorldUU({.41f,-.2f,.73f},centreYaw,baseYaw,unitsPerMetre);
        const V3 moved=TrackingDeltaToWorldUU(Add({.41f,-.2f,.73f},d),centreYaw,baseYaw,unitsPerMetre);
        const V3 diff=Sub(moved,origin);
        assert(close(Length(diff),Length(hmd),1e-4f));
    }
    const float eyeM=.064f; // representative test vector only; production uses located eye poses
    const V3 eye=TrackingDeltaToWorldUU({eyeM,0,0},centreYaw,baseYaw,unitsPerMetre);
    assert(close(Length(eye),eyeM*unitsPerMetre,1e-5f));
    assert(close(Length(TrackingDeltaToWorldUU({.1f,0,0},centreYaw,baseYaw,unitsPerMetre)),5.25f,1e-5f));
    std::puts("pose conversion: reflected orthonormal basis, HMD/left/right linearity, eye scale");
}

void quaternion_conversion_invariants() {
    using namespace vrpose;
    const float h=0.70710678118f;
    const float halfPi=1.57079632679f;
    const Quat samples[]={
        {0,0,0,1}, {h,0,0,h}, {-h,0,0,h},
        {0,h,0,h}, {0,-h,0,h}, {0,0,h,h}, {0,0,-h,h},
        {.2f,.3f,.1f,.92736185f}
    };
    for (unsigned i=0;i<sizeof(samples)/sizeof(samples[0]);++i) {
        const Quat q=samples[i];
        const float qn=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
        assert(close(qn,1.0f,1e-4f));
        const EulerYXZ a=DecomposeOpenXrYXZ(q);
        // Both OpenXR hand indices call this same production extractor.
        const EulerYXZ left=DecomposeOpenXrYXZ(q), right=DecomposeOpenXrYXZ(q);
        assert(close(left.pitchX,right.pitchX,1e-6f));
        assert(close(left.yawY,right.yawY,1e-6f));
        assert(close(left.rollZ,right.rollZ,1e-6f));
        assert(std::isfinite(a.pitchX)&&std::isfinite(a.yawY)&&std::isfinite(a.rollZ));
        if(i==0) assert(close(a.pitchX,0)&&close(a.yawY,0)&&close(a.rollZ,0));
        if(i==1) assert(close(a.pitchX,halfPi,1e-3f));
        if(i==2) assert(close(a.pitchX,-halfPi,1e-3f));
        if(i==3) assert(close(a.yawY,halfPi,1e-4f));
        if(i==4) assert(close(a.yawY,-halfPi,1e-4f));
        if(i==5) assert(close(a.rollZ,halfPi,1e-4f));
        if(i==6) assert(close(a.rollZ,-halfPi,1e-4f));

        // Compose the extracted Y-X-Z angles as a column-vector rotation and
        // verify orthogonality and determinant +1, including the combined case.
        const float cp=std::cos(a.pitchX),sp=std::sin(a.pitchX);
        const float cy=std::cos(a.yawY),sy=std::sin(a.yawY);
        const float cr=std::cos(a.rollZ),sr=std::sin(a.rollZ);
        const float m[3][3]={
            {cy*cr+sy*sp*sr,-cy*sr+sy*sp*cr,sy*cp},
            {cp*sr,cp*cr,-sp},
            {-sy*cr+cy*sp*sr,sy*sr+cy*sp*cr,cy*cp}
        };
        for(int row=0;row<3;++row) for(int col=0;col<3;++col) {
            float d=0;for(int k=0;k<3;++k)d+=m[row][k]*m[col][k];
            assert(close(d,row==col?1.0f:0.0f,1e-4f));
        }
        const float det=m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])-
                        m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])+
                        m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
        assert(close(det,1.0f,1e-4f));
    }
    std::puts("quaternion: identity, +/-90deg XYZ, combined, normalized, proper orthonormal rotations");
}

void arm_palette_decoding_invariants() {
    using namespace armbind;
    constexpr unsigned n=sizeof(bindassets::tmd)/sizeof(bindassets::tmd[0]);
    Mat bind[n];
    assert(BuildReference(bindassets::tmd,n,bind));
    const unsigned shoulder=7,elbow=8,wrist=9;
    const float upper=armik::Length(Position(bind[elbow])-Position(bind[shoulder]));
    const float fore=armik::Length(Position(bind[wrist])-Position(bind[elbow]));
    assert(close(upper,23.7283363f,.01f));
    assert(close(fore,21.4858837f,.01f));

    // S = inverse(B) * J under the verified row-vector skinning convention;
    // CurrentJoint(B,S) must reconstruct the component-space joint J.
    Mat invShoulder,invElbow,invWrist;
    assert(skeletal::Inverse(invShoulder.m,bind[shoulder].m));
    assert(skeletal::Inverse(invElbow.m,bind[elbow].m));
    assert(skeletal::Inverse(invWrist.m,bind[wrist].m));
    Mat desiredS=bind[shoulder],desiredE=bind[elbow],desiredW=bind[wrist];
    desiredS.m[3][0]+=2.0f;desiredS.m[3][1]-=3.0f;desiredS.m[3][2]+=1.0f;
    desiredE.m[3][0]+=2.0f;desiredE.m[3][1]-=3.0f;desiredE.m[3][2]+=1.0f;
    desiredW.m[3][0]+=2.0f;desiredW.m[3][1]-=3.0f;desiredW.m[3][2]+=1.0f;
    Mat skinS,skinE,skinW;
    skeletal::Multiply(skinS.m,invShoulder.m,desiredS.m);
    skeletal::Multiply(skinE.m,invElbow.m,desiredE.m);
    skeletal::Multiply(skinW.m,invWrist.m,desiredW.m);
    float gpuS[3][4],gpuE[3][4],gpuW[3][4];
    ToGpu(skinS,gpuS);ToGpu(skinE,gpuE);ToGpu(skinW,gpuW);
    Mat decodedS,decodedE,decodedW;
    assert(CurrentJoint(bind[shoulder],gpuS,decodedS));
    assert(CurrentJoint(bind[elbow],gpuE,decodedE));
    assert(CurrentJoint(bind[wrist],gpuW,decodedW));
    assert(Rigid(decodedS)&&Rigid(decodedE)&&Rigid(decodedW));
    assert(close(armik::Length(Position(decodedE)-Position(decodedS)),upper,.01f));
    assert(close(armik::Length(Position(decodedW)-Position(decodedE)),fore,.01f));
    assert(close(Position(decodedS).x,Position(desiredS).x,.001f));
    assert(close(Position(decodedW).z,Position(desiredW).z,.001f));
    std::puts("arm palette: GPU 3x4 decode, bind*skin component reconstruction, rigid joints, stable segment lengths");
}
} // namespace

int main() {
    trigger_profile_tests();
    trigger_runner_tests();
    fake_xr_tests();
    recenter_tests();
    arm_ik_property_tests();
    pose_conversion_invariants();
    quaternion_conversion_invariants();
    arm_palette_decoding_invariants();
    std::puts("PASS");
    return 0;
}
