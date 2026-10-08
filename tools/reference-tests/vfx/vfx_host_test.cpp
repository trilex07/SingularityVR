// Desktop test for the frozen VFX plugin ABI and the host-side loader/seams.
// Proves: the ABI layouts are stable, the host compiles, and every seam is a
// safe no-op when SingularityVfx.dll is absent (the kill-switch).
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <type_traits>

#include "../../../spikes/view_matrix/vfx_plugin.h"
#include "../../../spikes/view_matrix/vfx_host.h"
#include "../../../spikes/view_matrix/vfx_draw.h"
#include "../../../spikes/view_matrix/vfx_atlas_io.h"

using namespace singularity_vfx;

static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); std::exit(1); }
}

static_assert(VFX_PLUGIN_ABI_VERSION == 1, "ABI version is frozen at 1");

// DrawItem layout is part of the contract: both sides assume these offsets.
static_assert(sizeof(VfxDrawItem) == 40, "VfxDrawItem must stay 40 bytes");
static_assert(offsetof(VfxDrawItem, normalizedAge) == 8, "normalizedAge offset");
static_assert(offsetof(VfxDrawItem, position) == 12, "position offset");
static_assert(offsetof(VfxDrawItem, direction) == 24, "direction offset");
static_assert(offsetof(VfxDrawItem, size) == 36, "size offset");
static_assert(std::is_trivially_copyable<VfxDrawItem>::value, "DrawItem is POD");
static_assert(std::is_trivially_copyable<VfxFireEvent>::value, "FireEvent is POD");
static_assert(std::is_trivially_copyable<VfxHitEvent>::value, "HitEvent is POD");
static_assert(sizeof(VfxVec3) == 12, "Vec3 is three floats");

int main() {
    host::Plugin plugin;
    Check(!plugin.loaded(), "no plugin is loaded before Load()");

    // Seams must not touch anything when the plugin is absent.
    VfxFireEvent fire{};
    fire.muzzle = {1.0f, 2.0f, 3.0f};
    fire.forward = {0.0f, 0.0f, 1.0f};
    fire.weapon = VfxWeaponClass::Rifle;
    fire.sequence = 1;
    fire.valid = true;
    plugin.OnFire(fire);

    VfxHitEvent hit{};
    hit.point = {4.0f, 5.0f, 6.0f};
    hit.normal = {0.0f, 1.0f, 0.0f};
    hit.material = VfxHitMaterial::Flesh;
    hit.valid = true;
    plugin.OnHit(hit);

    plugin.Tick(0.016f);

    constexpr int kDrawCap = 32;
    VfxDrawItem items[kDrawCap] = {};
    Check(plugin.Collect(VfxRenderPass::Gameplay, items, kDrawCap) == 0,
          "Collect returns zero with no plugin");
    Check(plugin.Collect(VfxRenderPass::Menu, items, kDrawCap) == 0,
          "menu pass also returns zero");
    Check(plugin.Collect(VfxRenderPass::Shadow, items, kDrawCap) == 0,
          "shadow pass also returns zero");

    // No SingularityVfx.dll beside the test binary -> Load is a clean false.
    Check(plugin.Load() == false, "Load returns false when the plugin is absent");
    Check(!plugin.loaded(), "failure leaves the host unloaded");

    // The drawer compiles against d3d9.h; no device is created here.
    host::VfxDrawer drawer;
    Check(drawer.Init(nullptr) == false, "drawer Init(nullptr) is a safe false");

    std::puts("PASS: VFX ABI layout stable, host seams no-op without plugin, kill-switch clean");
    return 0;
}
