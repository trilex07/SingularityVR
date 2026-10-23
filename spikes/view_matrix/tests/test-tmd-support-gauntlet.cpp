#include "../tmd_support_gauntlet.h"
#include "../tmd_sleeve_palette.h"
#include "../dual_weapon_tmd_policy.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <initializer_list>

static void Identity(float m[4][4]) {
    std::memset(m,0,16*sizeof(float));for(int i=0;i<4;++i)m[i][i]=1;
}
static void Near(const float* a,const float* b,unsigned n) {
    for(unsigned i=0;i<n;++i)assert(std::fabs(a[i]-b[i])<.0001f);
}
int main() {
    for(unsigned bits=0;bits<256;++bits) {
        const bool on=tmdsupport::Enabled(bits&1,bits&2,bits&4,bits&8,bits&16,2,bits&32,bits&64);
        assert(on==((bits&127)==127));
        assert(!tmdsupport::Enabled(true,true,true,true,true,1,true,true));
        for(int support=0;support<2;++support)for(int palette=0;palette<2;++palette)
            for(int tracked=0;tracked<2;++tracked)
                assert(tmdsupport::HideHardware(support,on,palette,tracked)==
                    (!palette || !tracked || (support&&!on)));
    }
    unsigned cases=0;
    for(long gun:{5799L,8297L,11387L,11389L})for(int hands:{1,2})
    for(bool support:{false,true})for(int degrees:{-70,55}) {
        float local[4][4],base[4][4],primary[4][4],off[4][4],weapon[4][4];
        Identity(local);Identity(base);Identity(primary);Identity(off);Identity(weapon);
        local[3][0]=17;local[3][1]=-6; // nontrivial bone -> camera-space conjugation
        float angle=degrees*3.14159265359f/180;
        weapon[0][0]=weapon[1][1]=std::cos(angle);
        weapon[0][1]=std::sin(angle);weapon[1][0]=-std::sin(angle);
        weapon[3][0]=11;weapon[3][1]=24;
        primary[3][0]=8;off[3][1]=-23;
        const bool authored=dualtmd::UseWeaponAuthoredPair(true,true,hands,gun);
        float target[4][4],attachment[4][4],delta[4][4],right[4][4];
        const bool fore=tmdsupport::Target(target,primary,off,weapon,support,authored,gun);
        assert(fore==(support&&!authored));
        tmdsupport::Target(attachment,primary,off,weapon,support,authored,gun);
        Near(&target[0][0],&attachment[0][0],16); // same target in both eye rides
        assert(skeletal::LocalDeltaPair(delta,right,local,base,target,primary));
        float original[45][4]{},hardware[45][4],staged[45][4],glove[3][4];
        for(unsigned b=0;b<15;++b) {
            original[b*3][0]=original[b*3+1][1]=original[b*3+2][2]=1;
            original[b*3][3]=float(b)*.1f; // preserve distinct original bone animation
            skeletal::TransformBone(&hardware[b*3],&original[b*3],delta);
        }
        assert(tmdsleeve::Stage(tmdsleeve::kMap,15,original,hardware,delta,staged));
        Near(&staged[0][0],&hardware[0][0],180); // sleeve not double transformed
        skeletal::TransformBone(glove,&original[0],delta);
        Near(&glove[0][0],&staged[0][0],12); // same authored point, same rigid owner
        float chain[4][4],expected[4][4];
        skeletal::Multiply(chain,delta,local);skeletal::Multiply(expected,local,attachment);
        Near(&chain[0][0],&expected[0][0],16); // bone delta equals socket camera ride
        ++cases;
    }
    float identity[4][4],invalid[4][4],out[4][4];Identity(identity);Identity(invalid);
    invalid[0][0]=std::numeric_limits<float>::quiet_NaN();
    assert(!tmdsupport::Target(out,identity,identity,invalid,true,false,8297));
    Near(&out[0][0],&identity[0][0],16); // no invalid attachment target on failure
    dualtmd::PowerLatch impulse;
    assert(!impulse.Update(true,true,true)); // visible device is NOT permission to Impulse while supporting
    assert(!impulse.Update(true,false,true));
    assert(!impulse.Update(true,false,false));
    assert(impulse.Update(true,false,true));
    std::printf("PASS: %u support/weapon/rotation palettes; bounded admission, shared socket target, input rearm\n",cases);
}
