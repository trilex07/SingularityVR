#include "../native_restraint_hands.h"
#include <cstdio>
#include <initializer_list>
int main() {
    unsigned checks=0;
    const char* identity[]{"SkeletalMesh","ch_inview_noTMD_rope_mesh","Package","Skeletal","Package","wp_rope"};
    auto evaluate=[&](unsigned bits,const char* const* id) {
        return restraint::OwnsHands(bits&1,bits&2,bits&4,id[0],id[1],id[2],id[3],id[4],id[5],bits&8);
    };
    for(unsigned bits=0;bits<16;++bits) {
        if(evaluate(bits,identity)!=(bits==15))return 1;
        ++checks;
    }
    for(unsigned i=0;i<6;++i) {
        const char* saved=identity[i];
        for(const char* bad:{"", "other", "ch_inview_noTMD", "wp_rope_extra"}) {
            identity[i]=bad;if(evaluate(15,identity))return 2;++checks;
        }
        identity[i]=nullptr;if(evaluate(15,identity))return 3;++checks;
        identity[i]=saved;
    }
    // Release is immediate; no persistent/latching state belongs in policy.
    if(!evaluate(15,identity)||evaluate(14,identity)||evaluate(13,identity)||!evaluate(15,identity))return 4;
    ++checks;
    std::printf("PASS %u native restraint identity/ownership gates (CPU only)\n",checks);
}
