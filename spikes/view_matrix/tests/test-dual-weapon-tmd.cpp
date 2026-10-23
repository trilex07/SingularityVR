#include "../dual_weapon_tmd_policy.h"
#include "../two_hand_grip.h"
#include "../support_grip.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
    using namespace dualtmd;
    assert(Context(true,true,true,true,false,false,false,true));
    for(unsigned bits=0;bits<256;++bits){
        const bool expected=(bits&15)==15 && !(bits&112) && (bits&128);
        assert(Context(bits&1,bits&2,bits&4,bits&8,bits&16,bits&32,bits&64,bits&128)==expected);
    }
    for(int hands=-1;hands<=3;++hands)for(int context=0;context<2;++context)
        for(int equipped=0;equipped<2;++equipped){
            assert(Weapon(hands==2,context,equipped,hands)==
                (hands==2||(context&&equipped&&hands==1)));
        }
    assert(!MeleeWindow(true,false));assert(MeleeWindow(true,true));
    assert(MeleeWindow(false,false));assert(MeleeWindow(false,true));
    // Reproduce the integration collision: a TMD material may be submitted
    // while HANDS_WEAPON is active, but its tracked palette owns the pose.
    for(const auto& section: {std::initializer_list<unsigned>{7444,0,4402},
                              {2626,0,4402},{4818,13206,7704},
                              {4781,13206,7704},{7444,13206,7704},
                              {37,36318,17},{7444,36318,17}}) {
        auto it=section.begin();const unsigned verts=*it++,first=*it++,triangles=*it;
        assert(NativeTmdSection(verts,first,triangles));
        assert(IndependentTmdOwnsPose(true,true,true,false,true,verts,first,triangles));
        assert(!IndependentTmdOwnsPose(false,true,true,false,true,verts,first,triangles));
        assert(!IndependentTmdOwnsPose(true,true,false,false,true,verts,first,triangles));
        assert(!IndependentTmdOwnsPose(true,true,true,true,true,verts,first,triangles));
        assert(!IndependentTmdOwnsPose(true,true,true,false,false,verts,first,triangles));
    }
    assert(!NativeTmdSection(8297,13206,7704)); // gun
    assert(!NativeTmdSection(4818,0,7704)); // wrong material/range
    PowerLatch impulse,age;
    assert(impulse.Update(true,false,true)); // normal native Impulse
    assert(!impulse.Update(true,true,true)); // support priority
    assert(!impulse.Update(true,false,true)); // don't fire on support release
    assert(!impulse.Update(true,false,false));
    assert(impulse.Update(true,false,true));
    assert(!age.Update(true,true,true));
    assert(!age.Update(true,false,true));
    assert(age.Update(false,false,true)); // default OFF is a strict pass-through
    assert(!age.blocked);
    // Use the existing acquisition predicate and support transforms, not a
    // new dual-TMD grip implementation. Release restores independent powers.
    const float fore[3]={1,0,0},near[3]={.25f,0,0},far[3]={1,0,0};
    assert(TwoHandGripAllowed(near,fore,true,true,false));
    assert(!TwoHandGripAllowed(far,fore,true,true,false));
    assert(!TwoHandGripAllowed(near,fore,false,true,true));
    assert(!TwoHandGripAllowed(near,fore,true,false,true));
    for(int degrees=-180;degrees<=180;degrees+=15){
        const float a=degrees*3.14159265f/180.f;
        const float p[4][4]={{std::cos(a),std::sin(a),0,0},
            {-std::sin(a),std::cos(a),0,0},{0,0,1,0},{13,-7,4,1}};
        for(long weapon: {5799L,8297L,11387L,11389L}){
            float o[4][4]{};assert(ApplySupportGripTarget(o,p,weapon));
            const auto s=SupportGripFor(weapon);
            for(int j=0;j<3;++j){
                const float offset=s.mode==SupportGripMode::AuthoredPair?0:
                    s.fwd*p[0][j]+s.right*p[1][j]+s.up*p[2][j];
                assert(std::fabs(o[3][j]-p[3][j]-offset)<.0001f);
            }
        }
    }
    Census c;
    c.Add(1,false,false,false,true);c.Add(1,false,true,true,false);
    assert(c.rows[1][0].raw==1&&c.rows[1][0].passed==1&&c.rows[1][0].gunRide==1&&c.rows[1][0].nativeHands==1);
    c.Add(-1,false,false,false,false);c.Add(3,false,false,false,false);
    c.frames=Census::FrameBudget;c.Add(1,false,false,false,false);assert(c.rows[1][0].raw==1);
    c.Clear();assert(c.rows[1][0].raw==0);
    c.frames=0;c.notes=Census::NoteBudget;assert(!c.Active());
    puts("dual weapon/TMD policy PASS (256 contexts, ownership, rearm, bounded census)");
}
