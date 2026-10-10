// Regression for the staged two-phase PSVR2 trigger profile.
// This exercises pure policy only; hardware feedback remains an integration gate.
#include "../trigger_shot.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace psvr2trigger;
    using namespace triggershot;
    assert(WeaponFromGunVerts(5799) == Weapon::Pistol);
    assert(WeaponFromGunVerts(8297) == Weapon::Shotgun);
    assert(WeaponFromGunVerts(11387) == Weapon::Rifle);
    assert(WeaponFromGunVerts(11389) == Weapon::Sniper);
    assert(WeaponFromGunVerts(11390) == Weapon::Sniper);
    assert(WeaponFromGunVerts(99999) == Weapon::Other);

    const Kick pistol = KickFor(Weapon::Pistol);
    const Kick shotgun = KickFor(Weapon::Shotgun);
    assert(pistol.amplitude == 6 && shotgun.amplitude == 8);
    assert(ValidKickCommand(KickCommand(Hand::Right, pistol)));
    assert(ValidKickCommand(KickCommand(Hand::Right, shotgun)));
    assert(KickFor(Weapon::Other).durationMs == 0);
    assert(SpinUpKick(0).amplitude == 6 && SpinUpKick(1000).frequency == 55);

    ShotRunner runner;
    runner.SetHand(Hand::Right);
    const Command wall = ProfileFor(Weapon::Shotgun, false);
    runner.OnShotFired(1000, Weapon::Shotgun);
    const Command kick = runner.Select(wall, 1100);
    assert(kick.effect == Effect::Vibration && kick.amplitude == 8);
    assert(Pack(kick) != Pack(wall));
    assert(Pack(runner.Select(wall, 1220)) == Pack(kick));
    assert(Pack(runner.Select(wall, 1221)) == Pack(wall));

    runner.SetHand(Hand::Left);
    runner.OnShotFired(2000, Weapon::Pistol);
    const Command leftKick = runner.Select(ProfileFor(Weapon::Pistol, false), 2050);
    assert(leftKick.hand == Hand::Left);

    std::puts("PASS: shot-fired trigger resistance policy");
    return 0;
}
