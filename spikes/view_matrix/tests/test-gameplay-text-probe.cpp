#include "../gameplay_text_probe.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace gameplaytextprobe;
    const float corner[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, -.72f,-.68f,0,1};
    float middle[16]; for (int i=0;i<16;++i) middle[i]=corner[i]; middle[12]=0;
    float world[16]; for (int i=0;i<16;++i) world[i]=corner[i]; world[11]=1;
    assert(Candidate(true,true,false,true,20,30,corner));
    assert(!Candidate(false,true,false,true,20,30,corner));
    assert(!Candidate(true,false,false,true,20,30,corner));
    assert(!Candidate(true,true,true,true,20,30,corner));
    assert(!Candidate(true,true,false,true,20,30,middle));
    assert(!Candidate(true,true,false,true,20,30,world));
    assert(!Candidate(true,true,false,true,20,301,corner));
    UniqueBudget budget;
    const auto first=Key(2,20,30,-72,-68,0x1234);
    assert(budget.Admit(first) && !budget.Admit(first));
    assert(budget.Admit(Key(1,20,30,-72,-68,0x1234))); // route changes identity
    for (unsigned i=2;i<UniqueBudget::Limit;++i)
        assert(budget.Admit(Key(2,20,30,-72,-68,i+0x10000)));
    assert(!budget.Admit(Key(2,20,30,-72,-68,0xffffffff)));
    std::puts("gameplay text probe: scope, 2D geometry, route identity and 96-entry bound PASS");
}
