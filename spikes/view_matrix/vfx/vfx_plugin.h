#pragma once

// =============================================================================
// SingularityVR VFX plugin ABI  (FROZEN by the HOST / main agent)
//
// Variant A architecture:
//   HOST   = production d3d9.dll. Owns D3D9, per-eye matrices, the muzzle
//            transform, the FireAmmunition / SetFlashLocation event seams and
//            the actual quad drawing (inside the existing per-eye path).
//   PLUGIN = separate SingularityVfx.dll, loaded by the HOST. Owns effect
//            lifetime, the bounded pool, clip selection and the DrawItem list.
//            It contains NO D3D9 and NO engine hooks.
//
// This header is the ONLY contract between the two. Do not change it without
// bumping VFX_PLUGIN_ABI_VERSION; both sides include it verbatim.
// =============================================================================

#include <cstdint>

#define VFX_PLUGIN_ABI_VERSION 1u

namespace singularity_vfx {

// ---- value types shared across the boundary (plain, fixed width) ----------

struct VfxVec3 {
    float x, y, z;
};

enum class VfxKind : uint8_t {
    MuzzleFlash = 0,
    MuzzleSmoke,
    BloodImpact,
};

enum class VfxBlend : uint8_t {
    Additive = 0,   // ONE / ONE
    Alpha,          // SRCALPHA / INVSRCALPHA
};

enum class VfxRenderPass : uint8_t {
    Gameplay = 0,
    Menu,
    Shadow,
    DepthOnly,
    FlatCinema,
};

enum class VfxWeaponClass : uint8_t {
    Pistol = 0,
    Rifle,
    Shotgun,
    Gatling,
    Energy,
    Unknown,
};

enum class VfxHitMaterial : uint8_t {
    Flesh = 0,
    Metal,
    Concrete,
    Wood,
    Other,
};

// Stable clip IDs. The HOST maps these to atlas UV rectangles (manifest);
// the PLUGIN only selects a clip. Add new clips at the END and bump the ABI.
enum class VfxClip : uint16_t {
    MuzzleFireSmall = 0,
    MuzzleFireLarge,
    MuzzleFireHeavy,
    MuzzleFireEnergy,
    SmokeWhite,
    SmokeBlack,
    BloodSplash,
    BloodHitAlt,
    Count,
};

// ---- events: HOST -> PLUGIN ----------------------------------------------

struct VfxFireEvent {
    VfxVec3       muzzle;              // world-space muzzle position
    VfxVec3       forward;             // world-space shot direction (unit)
    VfxWeaponClass weapon;
    uint32_t      sequence;            // monotonic shot counter (host-owned)
    bool          valid;
    uint8_t       _pad[3];
};

struct VfxHitEvent {
    VfxVec3        point;              // world-space trace hit
    VfxVec3        normal;             // surface normal at the hit
    VfxHitMaterial material;
    uint32_t       sequence;
    bool           valid;
    uint8_t        _pad[3];
};

// ---- draw description: PLUGIN -> HOST -------------------------------------

// The host renders these as camera-facing quads, once per eye, inside the
// existing stereo path. The plugin never sees a device.
struct VfxDrawItem {
    uint8_t  kind;            // VfxKind
    uint8_t  blend;           // VfxBlend
    uint16_t clipId;          // VfxClip
    uint16_t frame;           // atlas frame index
    uint16_t _pad;
    float    normalizedAge;   // 0..1
    VfxVec3  position;        // world-space quad centre
    VfxVec3  direction;       // world-space facing hint (e.g. shot / hit normal)
    float    size;            // half-extent in world units (host may scale by FOV)
};

// ---- plugin exports (extern "C", resolved by the host) --------------------

typedef uint32_t (*VfxAbiVersionFn)();
typedef bool     (*VfxInitFn)();
typedef void     (*VfxShutdownFn)();
typedef void     (*VfxOnFireFn)(const VfxFireEvent*);
typedef void     (*VfxOnHitFn)(const VfxHitEvent*);
typedef void     (*VfxTickFn)(float dtSeconds);
typedef int      (*VfxCollectFn)(uint8_t renderPass, VfxDrawItem* out, int capacity);

#define VFX_EXPORT_ABI      "VfxPluginAbiVersion"
#define VFX_EXPORT_INIT     "VfxPluginInit"
#define VFX_EXPORT_SHUTDOWN "VfxPluginShutdown"
#define VFX_EXPORT_FIRE     "VfxPluginOnFire"
#define VFX_EXPORT_HIT      "VfxPluginOnHit"
#define VFX_EXPORT_TICK     "VfxPluginTick"
#define VFX_EXPORT_COLLECT  "VfxPluginCollect"

#define VFX_PLUGIN_FILENAME "SingularityVfx.dll"

} // namespace singularity_vfx
