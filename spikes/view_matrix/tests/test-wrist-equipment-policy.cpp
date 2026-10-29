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
    assert(Visibility(false,.4f,.35f,.2f,true,true,false,false,false,false));
    assert(!Visibility(false,.2f,.35f,.2f,true,true,false,false,false,false));
    assert(Visibility(true,.2f,.35f,.2f,true,true,false,false,false,false));
    assert(!Visibility(true,.4f,.35f,.2f,true,true,false,false,false,true));
    assert(!Visibility(true,.4f,.35f,.2f,false,true,false,false,false,false));
    assert(!SuppressLegacy(true,true,false,true));
    assert(SuppressLegacy(true,true,true,true));
}
