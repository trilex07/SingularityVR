#include <cassert>
#include "../glove_animation.h"
#include "../weapon_shot_haptics.h"
#include "../tmd_haptics.h"
#include "../free_hand_pose_policy.h"

int main() {
    using namespace weaponhaptics;
    assert(FromVertices(5799)==Weapon::Pistol);
    assert(FromVertices(11387)==Weapon::Rifle);
    assert(FromVertices(8297)==Weapon::Shotgun);
    assert(FromVertices(11389)==Weapon::Sniper);
    assert(FromVertices(11390)==Weapon::Sniper);
    assert(FromVertices(0)==Weapon::Other);
    Engine e;
    assert(!e.OnNativeReturn(0,0,Weapon::Pistol,100)); // dry fire
    assert(e.Sample(true,0,100)==0);
    assert(!e.OnNativeReturn(-1,1,Weapon::Pistol,100)); // no native witness
    assert(!e.OnNativeReturn(1,3,Weapon::Pistol,100)); // unrelated state change
    assert(!e.OnNativeReturn(1,2,Weapon::Other,100));  // unknown weapon
    assert(e.OnNativeReturn(1,2,Weapon::Pistol,100));
    assert(e.Sample(true,0,100)==For(Weapon::Pistol).amplitude);
    assert(e.Sample(true,.4f,101)==0);                 // native rumble wins
    assert(e.Sample(true,0,102)==0);                   // fallback stays cancelled
    assert(e.Sample(true,0,170)==0);                   // bounded pulse
    assert(e.OnNativeReturn(2,3,Weapon::Rifle,200));   // automatic repeat
    assert(e.OnNativeReturn(3,4,Weapon::Rifle,225));
    assert(e.Sample(true,0,250)==For(Weapon::Rifle).amplitude);
    assert(e.Sample(false,0,251)==0);                  // focus/pause clears
    assert(e.Sample(true,0,252)==0);
    assert(e.OnNativeReturn(4,5,Weapon::Shotgun,300));
    assert(e.Sample(true,0,300)>For(Weapon::Pistol).amplitude);

    // The real native-glove renderer consumes this blend. Release and tracking
    // loss cannot preserve a stale left-hand fist when the right holds a gun.
    float left=0, right=0;
    for(int n=0;n<8;++n)left=gloves::SmoothGrip(left,1,true,true);
    assert(left>.9f && right==0);                     // independent hands
    left=gloves::SmoothGrip(left,0,true,true);
    assert(left>0 && left<.9f);                       // smooth valid release
    for(int n=0;n<12;++n)left=gloves::SmoothGrip(left,0,true,true);
    assert(left<.002f);
    left=gloves::SmoothGrip(.8f,1,false,true);assert(left==0);
    left=gloves::SmoothGrip(.8f,1,true,false);assert(left==0);
    assert(gloves::SmoothGrip(0,1,true,true)>.4f);
    freehandpose::State free{};
    free.wristAnchor=free.weaponHeld=free.nativeReady=free.gameplay=
        free.focused=free.tracked=free.stereo=true;
    assert(freehandpose::OffhandFingers(free)); // right gun, independent left
    free.support=true;assert(!freehandpose::OffhandFingers(free)); // support owns fingers
    free.support=false;assert(freehandpose::OffhandFingers(free)); // release
    free.magazine=true;assert(!freehandpose::OffhandFingers(free));
    free.magazine=false;free.knife=true;assert(!freehandpose::OffhandFingers(free));
    free.knife=false;free.cinematic=true;assert(!freehandpose::OffhandFingers(free));
    free.cinematic=false;free.paused=true;assert(!freehandpose::OffhandFingers(free));
    free.paused=false;free.tracked=false;assert(!freehandpose::OffhandFingers(free));
    free.tracked=true;assert(freehandpose::OffhandFingers(free));
    assert(freehandpose::NativeCuffDraw(true,true,false,false,true,true));
    assert(!freehandpose::NativeCuffDraw(true,false,false,false,true,true));
    assert(!freehandpose::NativeCuffDraw(true,true,false,false,true,false));
    assert(!freehandpose::NativeCuffDraw(false,true,false,false,true,true));
    assert(!freehandpose::NativeCuffDraw(true,true,false,false,false,true));
    assert(tmdhaptics::ProfileFor(tmdhaptics::Power::Impulse).startAmp>
           tmdhaptics::ProfileFor(tmdhaptics::Power::Age).startAmp);
    return 0;
}
