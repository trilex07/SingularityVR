#include "../wrist_equipment_policy.h"
#include <cassert>
#include <limits>
int main(){
    using namespace wristhud;
    assert(Cells(100,100)==10&&Cells(50,100)==5&&Cells(1,100)==1&&Cells(0,100)==0);
    assert(Cells(std::numeric_limits<float>::quiet_NaN(),100)==-1);
    assert(!ValidEnergy(25,0)&&ValidEnergy(25,100));
    assert(ValidCount(0)&&!ValidCount(-1)&&!ValidCount(1000001));
    assert(Current(100,249)&&!Current(100,251)&&!Current(0,0));
    assert(Visibility(false,.9f,.35f,.2f,true,true,false,false,false,false));
    assert(!Visibility(false,.7f,.35f,.2f,true,true,false,false,false,false));
    assert(Visibility(true,.7f,.35f,.2f,true,true,false,false,false,false));
    assert(!Visibility(true,.9f,.35f,.2f,true,true,false,false,false,true));
    assert(!Visibility(true,.9f,.35f,.2f,false,true,false,false,false,false));
    // Both sides, edge-on, and a full wrist turn. No tracking/pose offsets.
    bool shown=false;
    for(int degrees=180;degrees>=0;--degrees){
        const float facing=std::cos(degrees*3.14159265358979323846f/180.f);
        shown=Visibility(shown,facing,.35f,.2f,true,true,false,false,false,false);
        assert(shown==(degrees<=36));
    }
    for(int degrees=0;degrees<=180;++degrees){
        const float facing=std::cos(degrees*3.14159265358979323846f/180.f);
        shown=Visibility(shown,facing,.35f,.2f,true,true,false,false,false,false);
        assert(shown==(degrees<=49));
    }
    assert(!SuppressLegacy(true,true,false,true));
    assert(SuppressLegacy(true,true,true,true));
}
