// Tests for shot-fired trigger resistance (no hardware, no game).
// Covers: gun fingerprint mapping, per-weapon kick ranges, kick window
// timing/expiry, ammo/menu gating via the base Select, kick-vs-wall dedupe,
// and the gatling spin-up stages.
#include <cstdio>
#include <cstdlib>

#include "../../../spikes/view_matrix/trigger_shot.h"

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}

using triggershot::KickFor;
using triggershot::ShotRunner;
using psvr2trigger::Effect;
using psvr2trigger::Hand;
using psvr2trigger::Weapon;

int main() {
    // Fingerprint mapping (same table as GunAlign).
    Check(triggershot::WeaponFromGunVerts(5799) == Weapon::Pistol, "5799 pistol");
    Check(triggershot::WeaponFromGunVerts(8297) == Weapon::Shotgun, "8297 shotgun");
    Check(triggershot::WeaponFromGunVerts(11387) == Weapon::Rifle, "11387 rifle");
    Check(triggershot::WeaponFromGunVerts(11389) == Weapon::Sniper, "11389 sniper");
    Check(triggershot::WeaponFromGunVerts(11390) == Weapon::Sniper, "11390 railgun heavy");
    Check(triggershot::WeaponFromGunVerts(0) == Weapon::Other, "unknown Other");
    Check(triggershot::WeaponFromGunVerts(99999) == Weapon::Other, "gatling pending census");

    // Kick profiles are significant and inside validated ranges.
    const Weapon kickWeapons[] = {Weapon::Pistol, Weapon::Shotgun, Weapon::Rifle, Weapon::Sniper};
    for (int i = 0; i < 4; ++i) {
        Weapon w = kickWeapons[i];
        const triggershot::Kick k = KickFor(w);
        Check(k.amplitude >= 6 && k.amplitude <= 8, "kick amplitude significant");
        Check(k.frequency >= 1 && k.durationMs > 0, "kick timed");
        Check(triggershot::ValidKickCommand(triggershot::KickCommand(Hand::Right, k)),
              "kick command valid");
    }
    Check(KickFor(Weapon::Tmd).durationMs == 0, "TMD has no kick");
    Check(KickFor(Weapon::Other).durationMs == 0, "Other has no kick");
    // Shotgun hits hardest, pistol is the lightest kick.
    Check(KickFor(Weapon::Shotgun).amplitude >= KickFor(Weapon::Pistol).amplitude,
          "shotgun >= pistol");
    Check(KickFor(Weapon::Shotgun).durationMs > KickFor(Weapon::Pistol).durationMs,
          "shotgun kick lasts longer");

    // Shot state machine: kick inside the window, wall outside it.
    ShotRunner r;
    psvr2trigger::Command wall = psvr2trigger::ProfileFor(Weapon::Shotgun, false);
    r.OnShotFired(1000, Weapon::Shotgun);
    Check(r.InKick(1000), "kick at fire instant");
    Check(r.InKick(1000 + 220), "kick at window edge");
    Check(!r.InKick(1000 + 221), "kick expires");
    psvr2trigger::Command during = r.Select(wall, 1100);
    Check(during.effect == Effect::Vibration && during.amplitude == 8, "kick overrides wall");
    psvr2trigger::Command after = r.Select(wall, 2000);
    Check(after.effect == wall.effect && after.strength == wall.strength, "wall returns");
    // Dedupe: kick and wall pack differently (backend sends on change only).
    Check(psvr2trigger::Pack(during) != psvr2trigger::Pack(after), "kick vs wall differ");

    // No shot yet -> wall passes through untouched.
    ShotRunner fresh;
    Check(psvr2trigger::Pack(fresh.Select(wall, 5000)) == psvr2trigger::Pack(wall),
          "no shot means wall");

    // Gatling spin-up stages rise monotonically (KHARVOX chaingun mirror).
    auto s1 = triggershot::SpinUpKick(0), s2 = triggershot::SpinUpKick(300),
         s3 = triggershot::SpinUpKick(600), s4 = triggershot::SpinUpKick(5000);
    Check(s1.amplitude <= s2.amplitude && s2.amplitude <= s4.amplitude, "spin-up rises");
    Check(s4.amplitude == 8 && s4.frequency == 55, "full-speed 8/55");

    if (g_fail) { std::fprintf(stderr, "trigger-shot tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: shot-fired trigger resistance policy");
    return 0;
}
