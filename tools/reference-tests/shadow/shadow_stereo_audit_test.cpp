// Offline audit tests for the stereo shadow artifact diagnosis.
// Pure logic over checked-in headers only (no D3D, no game, no Steam).
// Research branch: production d3d9.cpp is not touched by this file.
//
// Build (x86 MSVC): cl /nologo /EHsc /W3
//   tools\reference-tests\shadow\shadow_stereo_audit_test.cpp
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "../../../spikes/view_matrix/shadow_mode3.h"
#include "../../../spikes/view_matrix/shadow_pass_policy.h"
#include "../../../spikes/view_matrix/shadow_policy.h"
#include "../../../spikes/view_matrix/fp_shadow.h"

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}

using singularity::shadow::DrawEvidence;
using singularity::shadow::StereoAction;
using singularity::shadow::DecideDraw;
using singularity::shadow::ShouldRemapRegister;
using singularity::shadow::ShadowRegisterCensus;
using singularity::shadow::SlotMayRemap;
using singularity::shadow::PassEvidence;
using singularity::shadow::PassKind;
using singularity::shadow::DrawAction;
using singularity::shadow::Classify;
using singularity::shadow::Decide;

static PassEvidence GameplayEyeEvidence() {
    PassEvidence e{};
    e.nearPlane = 0.1f; e.farPlane = 5000.0f;
    e.projectionW = 1.0f; e.projectionH = 1.0f;
    e.targetWidth = 2560; e.targetHeight = 1440;
    e.sceneWidth = 2560; e.sceneHeight = 1440;
    e.perspective = true; e.eyeSizedTarget = true; e.hasEyeViewport = true;
    return e;
}

int main() {
    // --- remap exclusion (shadow_mode3.h) ---
    Check(ShouldRemapRegister(true, false), "camera non-shadow slot remapped");
    Check(!ShouldRemapRegister(true, true), "light-space slot never remapped");
    Check(!ShouldRemapRegister(false, false), "non-camera slot never remapped");
    Check(!ShouldRemapRegister(false, true), "non-camera shadow slot never remapped");

    ShadowRegisterCensus census;
    Check(SlotMayRemap(census, 2, true), "empty census remaps: mode 3 degrades to split");
    census.Mark(2);
    Check(!SlotMayRemap(census, 2, true), "census-marked light slot excluded");
    Check(SlotMayRemap(census, 3, true), "unmarked camera slot still remapped");
    Check(!SlotMayRemap(census, 3, false), "non-camera slot never remapped");
    ShadowRegisterCensus empty;
    Check(!SlotMayRemap(empty, 4, true, 1u << 4), "carry-mask light slot excluded");
    Check(SlotMayRemap(empty, 5, true, 1u << 4), "non-carried slot still remapped");
    Check(!empty.IsShadow(33), "slots >= 32 never shadow");

    // --- seam routing (shadow_mode3.h DecideDraw) ---
    DrawEvidence gen{}; gen.layoutShadow = true;
    Check(DecideDraw(gen) == StereoAction::NativeShadowOnce, "generation drawn native-once");
    DrawEvidence depth{}; depth.offscreenDepthOnly = true;
    Check(DecideDraw(depth) == StereoAction::NativeShadowOnce, "depth-only drawn native-once");
    DrawEvidence off{}; off.isScene = false;
    Check(DecideDraw(off) == StereoAction::NativeShadowOnce, "offscreen drawn native-once");
    DrawEvidence eye{}; eye.knownCameraSlot = true;
    Check(DecideDraw(eye) == StereoAction::StereoEye, "known camera slot split per eye");
    DrawEvidence unknown{};
    Check(DecideDraw(unknown) == StereoAction::MonoFallback, "unknown falls back to mono");
    DrawEvidence bad{}; bad.finite = false; bad.knownCameraSlot = true;
    Check(DecideDraw(bad) == StereoAction::MonoFallback, "non-finite falls back to mono");

    // --- policy gates (shadow_pass_policy.h) ---
    PassEvidence shadow = GameplayEyeEvidence();
    shadow.shaderDeclaresShadow = true;
    Check(Classify(shadow) == PassKind::ShadowLightSpace, "shadow shader never gameplay");
    Check(Decide(shadow) == DrawAction::PreserveNativeShadow, "shadow pass preserved native");
    PassEvidence depthOnly = GameplayEyeEvidence();
    depthOnly.depthOnly = true; depthOnly.eyeSizedTarget = false;
    depthOnly.targetWidth = 512; depthOnly.targetHeight = 512;
    Check(Classify(depthOnly) == PassKind::ShadowLightSpace, "depth-only offscreen is light-space");
    PassEvidence recv = GameplayEyeEvidence();
    recv.sampledProjectedShadow = true;
    Check(Decide(recv) == DrawAction::MonoShadowReceiver, "sampled receiver drawn mono");
    PassEvidence plain = GameplayEyeEvidence();
    Check(Decide(plain) == DrawAction::StereoEye, "plain gameplay split per eye");
    PassEvidence nan = GameplayEyeEvidence();
    nan.nearPlane = std::numeric_limits<float>::quiet_NaN();
    Check(Decide(nan) == DrawAction::ConservativeMono, "non-finite evidence conservative mono");
    PassEvidence diag = depthOnly;
    Check(Decide(diag, true) == DrawAction::DropDiagnostic, "drop is explicit-only");
    Check(Decide(diag, false) == DrawAction::PreserveNativeShadow, "drop never implicit");
    PassEvidence ui{};
    Check(Classify(ui) == PassKind::UiOrPost, "empty evidence is ui/post");

    // --- map filter (shadow_policy.h) ---
    Check(IsProjectedShadowMap(114u, 512u, 512u), "512 R32F accepted");
    Check(IsProjectedShadowMap(113u, 1024u, 1024u), "1024 R16F accepted");
    Check(!IsProjectedShadowMap(114u, 512u, 256u), "non-square rejected");
    Check(!IsProjectedShadowMap(114u, 64u, 64u), "tiny rejected");
    Check(!IsProjectedShadowMap(21u, 512u, 512u), "colour format rejected by map filter");
    Check(SuppressVrShadow(true, false, true), "vr+disabled+classified suppresses");
    Check(!SuppressVrShadow(true, true, true), "enabled shadows never suppressed");
    Check(!SuppressVrShadow(false, false, true), "non-vr never suppressed");
    Check(SuppressFirstPersonShadowCaster(true, true, false, 390u, 390u, 100u),
          "FP gun-verts caster suppressed");
    Check(!SuppressFirstPersonShadowCaster(true, true, false, 777u, 390u, 100u),
          "non-FP verts never suppressed");

    // --- FP caster conjunction (fp_shadow.h) ---
    const unsigned sig[] = {390u, 14490u};
    fpshadow::DrawKey color{};
    color.verts = 390; color.first = 14490; color.prims = 674;
    color.vb = reinterpret_cast<const void*>(0x1000);
    color.ib = reinterpret_cast<const void*>(0x2000);
    color.stride = 32; color.rtIsScene = true;
    fpshadow::DrawKey caster = color;
    caster.layoutShadow = true; caster.rtIsScene = false;
    Check(fpshadow::IsFpShadowCasterDraw(caster, sig, 2, &color, 1), "FP caster conjunction matches");
    fpshadow::DrawKey sceneShadow = caster; sceneShadow.rtIsScene = true;
    Check(!fpshadow::IsFpShadowCasterDraw(sceneShadow, sig, 2, &color, 1), "scene shadow never FP caster");
    fpshadow::DrawKey colorShader = caster; colorShader.layoutShadow = false;
    Check(!fpshadow::IsFpShadowCasterDraw(colorShader, sig, 2, &color, 1), "colour shader never FP caster");
    fpshadow::DrawKey otherMesh = caster; otherMesh.vb = reinterpret_cast<const void*>(0x3000);
    Check(!fpshadow::IsFpShadowCasterDraw(otherMesh, sig, 2, &color, 1), "tuple mismatch never FP caster");
    fpshadow::DrawKey worldMesh = caster; worldMesh.verts = 777;
    Check(!fpshadow::IsFpShadowCasterDraw(worldMesh, sig, 2, &color, 1), "non-FP verts never FP caster");
    Check(!fpshadow::IsFpShadowCasterDraw(caster, sig, 2, nullptr, 0), "no colour draws never FP caster");

    if (g_fail) { std::fprintf(stderr, "shadow stereo audit: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: shadow stereo audit (remap exclusion + seam routing + policy gates + FP caster)");
    return 0;
}
