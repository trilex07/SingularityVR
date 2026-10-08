// End-to-end test of the Variant A split: load the reference plugin stub through
// the host loader and verify the seams round-trip. Requires SingularityVfx.dll
// to sit next to this test executable (build.ps1 produces it; copy it here as
// SingularityVfx.dll before running).
#include <cstdio>
#include <cstdlib>

#include "../../../spikes/view_matrix/vfx_host.h"

using namespace singularity_vfx;

static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); std::exit(1); }
}

int main() {
    host::Plugin plugin;
    if (!plugin.Load()) {
        std::fprintf(stderr, "FAIL: SingularityVfx.dll not found next to this test\n");
        return 2;
    }
    Check(plugin.loaded(), "plugin loads and negotiates ABI");

    VfxDrawItem items[32] = {};

    VfxFireEvent fire{};
    fire.muzzle = {10.0f, 20.0f, 30.0f};
    fire.forward = {0.0f, 0.0f, 1.0f};
    fire.weapon = VfxWeaponClass::Rifle;
    fire.sequence = 4;
    fire.valid = true;
    plugin.OnFire(fire);

    int n = plugin.Collect(VfxRenderPass::Gameplay, items, 32);
    Check(n == 3, "one flash + two smoke puffs collected in gameplay");
    Check(plugin.Collect(VfxRenderPass::Menu, items, 32) == 0, "menu pass isolated");
    Check(plugin.Collect(VfxRenderPass::Shadow, items, 32) == 0, "shadow pass isolated");
    Check(items[0].kind == static_cast<uint8_t>(VfxKind::MuzzleFlash), "first item is the flash");
    Check(items[0].blend == static_cast<uint8_t>(VfxBlend::Additive), "flash is additive");
    Check(items[1].blend == static_cast<uint8_t>(VfxBlend::Alpha), "smoke is alpha");
    Check(items[0].clipId == static_cast<uint16_t>(VfxClip::MuzzleFireSmall), "rifle alt frame picks small flash");

    plugin.Tick(1.0f);
    Check(plugin.Collect(VfxRenderPass::Gameplay, items, 32) == 0, "all effects expire within 1s");

    VfxHitEvent flesh{};
    flesh.point = {1.0f, 2.0f, 3.0f};
    flesh.normal = {0.0f, 1.0f, 0.0f};
    flesh.material = VfxHitMaterial::Flesh;
    flesh.sequence = 1;
    flesh.valid = true;
    plugin.OnHit(flesh);
    Check(plugin.Collect(VfxRenderPass::Gameplay, items, 32) == 1, "flesh hit spawns one blood effect");

    VfxHitEvent metal = flesh;
    metal.material = VfxHitMaterial::Metal;
    plugin.OnHit(metal);
    Check(plugin.Collect(VfxRenderPass::Gameplay, items, 32) == 1, "metal hit adds no blood");

    plugin.Unload();
    Check(!plugin.loaded(), "unload clears the host handle");

    std::puts("PASS: host<->plugin round-trip (flash/smoke/blood, gating, expiry, kill-switch)");
    return 0;
}
