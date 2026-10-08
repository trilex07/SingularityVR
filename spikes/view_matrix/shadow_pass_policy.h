#pragma once

// Staged shadow policy for SingularityVR.
//
// This file is intentionally independent of d3d9.cpp.  It is the side-worktree
// contract that the production hook can consume after the headset shadow pass
// has been captured and identified.  The important invariant is that a
// light-space matrix is never treated as a second eye camera.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace singularity::shadow {

enum class PassKind : std::uint8_t {
    Unknown,
    GameplayEye,
    ShadowLightSpace,
    ReflectionOrProbe,
    UiOrPost,
};

enum class DrawAction : std::uint8_t {
    ConservativeMono,
    StereoEye,
    PreserveNativeShadow,
    MonoShadowReceiver,
    DropDiagnostic,
};

// Evidence available at the SetVSConstantF/Draw seam.  No pointer or object
// scan is needed.  The production hook can fill these values from the already
// cached render target, shader layout and projection constants.
struct PassEvidence {
    float nearPlane = 0.0f;
    float farPlane = 0.0f;
    float projectionW = 0.0f;
    float projectionH = 0.0f;
    std::uint32_t targetWidth = 0;
    std::uint32_t targetHeight = 0;
    std::uint32_t sceneWidth = 0;
    std::uint32_t sceneHeight = 0;
    bool perspective = false;
    bool depthOnly = false;
    bool shaderDeclaresShadow = false;
    bool sampledProjectedShadow = false;
    bool eyeSizedTarget = false;
    bool hasEyeViewport = false;
};

struct PolicyConfig {
    // Matches the observed ForzaTech/UE-style main-camera gate while keeping
    // the values explicit and testable.  The wide far range is intentional:
    // Singularity's gameplay far plane varies by level.
    float gameplayNearMin = 0.06f;
    float gameplayNearMax = 0.20f;
    float gameplayFarMin = 2000.0f;
    // A near plane below this is characteristic of the observed light-space
    // shadow pass (about 0.01), not a reason to apply HMD translation.
    float lightNearMax = 0.04f;
};

inline bool finite(float v) { return std::isfinite(v) != 0; }

inline bool SceneSized(const PassEvidence& e) {
    return e.eyeSizedTarget || (e.sceneWidth != 0 && e.sceneHeight != 0 &&
                                e.targetWidth == e.sceneWidth &&
                                e.targetHeight == e.sceneHeight);
}

inline PassKind Classify(const PassEvidence& e, const PolicyConfig& cfg = {}) {
    // Strong negative evidence comes first.  A shadow shader or depth-only
    // offscreen target must never pass the gameplay gate by accident.
    if (e.shaderDeclaresShadow || e.depthOnly)
        return PassKind::ShadowLightSpace;

    if (!SceneSized(e) && !e.perspective)
        return PassKind::UiOrPost;

    const bool mainGate = e.perspective && e.hasEyeViewport && SceneSized(e) &&
        e.nearPlane >= cfg.gameplayNearMin && e.nearPlane <= cfg.gameplayNearMax &&
        e.farPlane > cfg.gameplayFarMin;
    if (mainGate)
        return PassKind::GameplayEye;

    if (e.nearPlane > 0.0f && e.nearPlane <= cfg.lightNearMax &&
        (!SceneSized(e) || e.depthOnly))
        return PassKind::ShadowLightSpace;

    if (!e.perspective || !e.hasEyeViewport)
        return PassKind::UiOrPost;
    return PassKind::Unknown;
}

inline bool IsFiniteEvidence(const PassEvidence& e) {
    return finite(e.nearPlane) && finite(e.farPlane) && finite(e.projectionW) &&
           finite(e.projectionH);
}

// `dropDiagnostic` is deliberately an explicit test switch.  It is never the
// default and cannot be selected implicitly by an unknown classification.
inline DrawAction Decide(const PassEvidence& e, bool dropDiagnostic = false,
                         const PolicyConfig& cfg = {}) {
    if (!IsFiniteEvidence(e)) return DrawAction::ConservativeMono;
    switch (Classify(e, cfg)) {
    case PassKind::GameplayEye:
        return e.sampledProjectedShadow ? DrawAction::MonoShadowReceiver
                                        : DrawAction::StereoEye;
    case PassKind::ShadowLightSpace:
        return dropDiagnostic ? DrawAction::DropDiagnostic
                               : DrawAction::PreserveNativeShadow;
    case PassKind::ReflectionOrProbe:
    case PassKind::UiOrPost:
    case PassKind::Unknown:
    default:
        return DrawAction::ConservativeMono;
    }
}

inline bool AppliesHeadPose(DrawAction action) {
    return action == DrawAction::StereoEye;
}

// Compact offline vector/matrix primitives.  The simulator uses column-vector
// semantics (M * p), explicitly separate from the D3D9 row/column adapter.
// This makes the geometric invariants readable without pretending to be a live
// device matrix upload.
struct Vec3 { float x{}, y{}, z{}; };
struct Vec4 { float x{}, y{}, z{}, w{}; };
struct Mat4 { float m[16]{}; };

inline Mat4 Identity() {
    Mat4 r{}; r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f; return r;
}

inline Mat4 Mul(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            for (int k = 0; k < 4; ++k)
                r.m[row * 4 + col] += a.m[row * 4 + k] * b.m[k * 4 + col];
    return r;
}

inline Vec4 Transform(const Mat4& a, Vec4 p) {
    return {
        a.m[0]*p.x + a.m[1]*p.y + a.m[2]*p.z + a.m[3]*p.w,
        a.m[4]*p.x + a.m[5]*p.y + a.m[6]*p.z + a.m[7]*p.w,
        a.m[8]*p.x + a.m[9]*p.y + a.m[10]*p.z + a.m[11]*p.w,
        a.m[12]*p.x + a.m[13]*p.y + a.m[14]*p.z + a.m[15]*p.w,
    };
}

inline Mat4 Translation(Vec3 t) {
    Mat4 r = Identity(); r.m[3] = t.x; r.m[7] = t.y; r.m[11] = t.z; return r;
}

inline bool SameMatrix(const Mat4& a, const Mat4& b, float eps = 0.0f) {
    for (int i = 0; i < 16; ++i)
        if (std::fabs(a.m[i] - b.m[i]) > eps) return false;
    return true;
}

// A deterministic directional-light projection used by the side simulator.
// It is not wired into the game: the live hook must first capture the game's
// own light view/projection and atlas layout.  The function is useful for
// proving UV/depth and stereo invariants before that capture exists.
struct ShadowProjection {
    Mat4 lightViewProjection{};
    float atlasScaleX = 1.0f;
    float atlasScaleY = 1.0f;
    float atlasBiasX = 0.0f;
    float atlasBiasY = 0.0f;
};

inline Vec3 Normalize(Vec3 v) {
    const float n = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (n <= 1e-6f) return {0.0f, 0.0f, -1.0f};
    return {v.x/n, v.y/n, v.z/n};
}

inline float Dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3 Cross(Vec3 a, Vec3 b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

inline Mat4 DirectionalLightView(Vec3 direction, Vec3 center) {
    const Vec3 forward = Normalize({-direction.x, -direction.y, -direction.z});
    Vec3 up = std::fabs(forward.y) > 0.9f ? Vec3{1,0,0} : Vec3{0,1,0};
    const Vec3 right = Normalize(Cross(up, forward));
    up = Cross(forward, right);
    Mat4 r = Identity();
    r.m[0] = right.x;   r.m[1] = right.y;   r.m[2] = right.z;
    r.m[4] = up.x;      r.m[5] = up.y;      r.m[6] = up.z;
    r.m[8] = forward.x; r.m[9] = forward.y; r.m[10] = forward.z;
    r.m[3] = -Dot(right, center);
    r.m[7] = -Dot(up, center);
    r.m[11] = -Dot(forward, center);
    return r;
}

inline Mat4 Ortho(float halfWidth, float halfHeight, float nearDepth, float farDepth) {
    const float z = std::max(farDepth - nearDepth, 1e-4f);
    Mat4 r{};
    r.m[0] = 1.0f / std::max(halfWidth, 1e-4f);
    r.m[5] = 1.0f / std::max(halfHeight, 1e-4f);
    r.m[10] = 1.0f / z;
    r.m[11] = -nearDepth / z;
    r.m[15] = 1.0f;
    return r;
}

inline ShadowProjection BuildDirectionalShadow(Vec3 direction, Vec3 center,
                                               float halfWidth, float halfHeight,
                                               float nearDepth, float farDepth,
                                               float atlasScaleX = 1.0f,
                                               float atlasScaleY = 1.0f,
                                               float atlasBiasX = 0.0f,
                                               float atlasBiasY = 0.0f) {
    ShadowProjection p{};
    p.lightViewProjection = Mul(Ortho(halfWidth, halfHeight, nearDepth, farDepth),
                                DirectionalLightView(direction, center));
    p.atlasScaleX = atlasScaleX; p.atlasScaleY = atlasScaleY;
    p.atlasBiasX = atlasBiasX; p.atlasBiasY = atlasBiasY;
    return p;
}

inline bool ProjectToAtlas(const ShadowProjection& p, Vec3 world, Vec3& uvDepth) {
    const Vec4 clip = Transform(p.lightViewProjection, {world.x, world.y, world.z, 1.0f});
    if (!finite(clip.x) || !finite(clip.y) || !finite(clip.z) || !finite(clip.w) ||
        std::fabs(clip.w) < 1e-6f) return false;
    const float invW = 1.0f / clip.w;
    uvDepth = {clip.x*invW*p.atlasScaleX + p.atlasBiasX,
               clip.y*invW*p.atlasScaleY + p.atlasBiasY,
               clip.z*invW};
    return finite(uvDepth.x) && finite(uvDepth.y) && finite(uvDepth.z);
}

} // namespace singularity::shadow
