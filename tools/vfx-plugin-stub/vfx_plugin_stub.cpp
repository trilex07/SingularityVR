// =============================================================================
// SingularityVfx - reference PLUGIN stub for the VFX ABI (Variant A).
//
// This is a working, self-contained implementation of the frozen ABI in
// vfx_plugin.h: a bounded pool that turns fire/hit events into a DrawItem list.
// It contains NO D3D9 and NO engine hooks, exactly as the plugin contract
// requires. The real VFX agent replaces this logic and the atlas; the exports
// and the ABI stay the same.
//
// Build: tools/vfx-plugin-stub/build.ps1  ->  SingularityVfx.dll (x86, /MD, C++17)
// =============================================================================

#include "../../spikes/view_matrix/vfx_plugin.h"

#include <cstdint>

using namespace singularity_vfx;

#define VFX_API extern "C" __declspec(dllexport)

namespace {

constexpr int kMaxActive = 32;

// Per-clip presentation. Mirrors vfx_atlas_manifest.json (frames/fps/blend) and
// adds the world-space size hint the host consumes.
struct ClipParams {
    uint16_t frames;
    float    fps;
    float    baseSize;
    float    growth;     // added over normalized age
    VfxBlend blend;
};

ClipParams ParamsFor(VfxClip id) {
    switch (id) {
    case VfxClip::MuzzleFireSmall:  return {3, 30.0f, 0.045f, 0.0f,  VfxBlend::Additive};
    case VfxClip::MuzzleFireLarge:  return {3, 30.0f, 0.060f, 0.0f,  VfxBlend::Additive};
    case VfxClip::MuzzleFireHeavy:  return {3, 24.0f, 0.075f, 0.0f,  VfxBlend::Additive};
    case VfxClip::MuzzleFireEnergy: return {3, 30.0f, 0.055f, 0.0f,  VfxBlend::Additive};
    case VfxClip::SmokeWhite:       return {16, 30.0f, 0.070f, 0.050f, VfxBlend::Alpha};
    case VfxClip::SmokeBlack:       return {16, 30.0f, 0.085f, 0.060f, VfxBlend::Alpha};
    case VfxClip::BloodSplash:      return {6, 30.0f, 0.040f, 0.015f, VfxBlend::Alpha};
    case VfxClip::BloodHitAlt:      return {14, 30.0f, 0.045f, 0.015f, VfxBlend::Alpha};
    default:                        return {1, 30.0f, 0.050f, 0.0f,  VfxBlend::Alpha};
    }
}

struct Slot {
    bool     active = false;
    VfxKind  kind = VfxKind::MuzzleFlash;
    VfxClip  clip = VfxClip::MuzzleFireSmall;
    VfxVec3  position{};
    VfxVec3  direction{0.0f, 0.0f, 1.0f};
    float    age = 0.0f;
    float    life = 0.0f;
};

Slot g_slots[kMaxActive];
int  g_count = 0;

void Add(VfxKind kind, VfxClip clip, VfxVec3 pos, VfxVec3 dir, float life) {
    for (int i = 0; i < kMaxActive; ++i) {
        Slot& s = g_slots[i];
        if (s.active) continue;
        s.active = true;
        s.kind = kind;
        s.clip = clip;
        s.position = pos;
        s.direction = dir;
        s.age = 0.0f;
        s.life = life;
        ++g_count;
        return;
    }
    // Pool exhaustion drops the newest effect; existing effects are never displaced.
}

VfxVec3 Add3(VfxVec3 a, VfxVec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
VfxVec3 Mul3(VfxVec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }

VfxClip FlashClip(VfxWeaponClass w, uint32_t seq) {
    switch (w) {
    case VfxWeaponClass::Shotgun: return VfxClip::MuzzleFireLarge;
    case VfxWeaponClass::Gatling: return VfxClip::MuzzleFireHeavy;
    case VfxWeaponClass::Energy:  return VfxClip::MuzzleFireEnergy;
    default:
        return (seq & 1u) ? VfxClip::MuzzleFireLarge : VfxClip::MuzzleFireSmall;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// ABI exports
// ---------------------------------------------------------------------------
VFX_API uint32_t VfxPluginAbiVersion() { return VFX_PLUGIN_ABI_VERSION; }

VFX_API bool VfxPluginInit() {
    for (int i = 0; i < kMaxActive; ++i) g_slots[i].active = false;
    g_count = 0;
    return true;
}

VFX_API void VfxPluginShutdown() {
    for (int i = 0; i < kMaxActive; ++i) g_slots[i].active = false;
    g_count = 0;
}

VFX_API void VfxPluginOnFire(const VfxFireEvent* e) {
    if (!e || !e->valid) return;
    Add(VfxKind::MuzzleFlash, FlashClip(e->weapon, e->sequence), e->muzzle, e->forward, 0.105f);

    const VfxClip smoke = (e->weapon == VfxWeaponClass::Gatling)
                        ? VfxClip::SmokeBlack : VfxClip::SmokeWhite;
    Add(VfxKind::MuzzleSmoke, smoke, Add3(e->muzzle, Mul3(e->forward, 0.025f)), e->forward, 0.38f);
    Add(VfxKind::MuzzleSmoke, smoke, Add3(e->muzzle, Mul3(e->forward, 0.060f)), e->forward, 0.52f);
}

VFX_API void VfxPluginOnHit(const VfxHitEvent* e) {
    if (!e || !e->valid || e->material != VfxHitMaterial::Flesh) return;
    const VfxClip clip = (e->sequence & 1u) ? VfxClip::BloodHitAlt : VfxClip::BloodSplash;
    Add(VfxKind::BloodImpact, clip, Add3(e->point, Mul3(e->normal, 0.002f)), e->normal,
        clip == VfxClip::BloodHitAlt ? 0.28f : 0.20f);
}

VFX_API void VfxPluginTick(float dt) {
    if (dt <= 0.0f) return;
    for (int i = 0; i < kMaxActive; ++i) {
        Slot& s = g_slots[i];
        if (!s.active) continue;
        s.age += dt;
        if (s.age >= s.life) { s.active = false; if (g_count > 0) --g_count; }
    }
}

VFX_API int VfxPluginCollect(uint8_t renderPass, VfxDrawItem* out, int capacity) {
    if (renderPass != static_cast<uint8_t>(VfxRenderPass::Gameplay) || !out || capacity <= 0)
        return 0;
    int written = 0;
    for (int i = 0; i < kMaxActive && written < capacity; ++i) {
        const Slot& s = g_slots[i];
        if (!s.active) continue;
        const ClipParams p = ParamsFor(s.clip);
        const float norm = s.life > 0.0f ? s.age / s.life : 1.0f;
        int frame = static_cast<int>(s.age * p.fps);
        if (frame < 0) frame = 0;
        if (frame >= p.frames) frame = p.frames - 1;

        VfxDrawItem& d = out[written++];
        d.kind = static_cast<uint8_t>(s.kind);
        d.blend = static_cast<uint8_t>(p.blend);
        d.clipId = static_cast<uint16_t>(s.clip);
        d.frame = static_cast<uint16_t>(frame);
        d._pad = 0;
        d.normalizedAge = norm;
        d.position = s.position;
        d.direction = s.direction;
        d.size = p.baseSize + p.growth * norm;
    }
    return written;
}
