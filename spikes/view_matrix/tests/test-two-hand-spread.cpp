#include "../two_hand_spread.h"
#include <cassert>
#include <cstdio>
int main(){
    int shot[]{1100,2100,42};
    assert(!twohandspread::Apply(shot,1000,2000,50,true,false));
    assert(shot[0]==1100&&shot[1]==2100&&shot[2]==42);
    assert(!twohandspread::Apply(shot,1000,2000,50,false,true));
    assert(twohandspread::Apply(shot,1000,2000,50,true,true));
    assert(shot[0]==1050&&shot[1]==2050&&shot[2]==42);
    assert(twohandspread::Reduced(65530,14,50)==4);
    assert(twohandspread::Reduced(6,65522,50)==65532);
    for(int centre=0;centre<65536;centre+=257)for(int delta=-1000;delta<=1000;delta+=25){
        int native=(centre+delta)&65535;
        assert(twohandspread::Reduced(centre,native,100)==native);
        assert(twohandspread::Reduced(centre,centre,50)==centre);
        assert(twohandspread::Reduced(centre,native,0)==centre);
    }
    puts("PASS two-hand native spread delta, wrap, central recoil, roll and local/support gates");
}
