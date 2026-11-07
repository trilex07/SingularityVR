#include "../tmd_visual_audit.h"
#include <cassert>
using namespace tmdvisualaudit;
int main(){
    Schedule s;
    for(unsigned i=0;i<100;++i)assert(s.Update(false,true,false,0,0,false,i)==Scene::None);
    assert(s.captured==0);
    for(unsigned i=0;i<29;++i)assert(s.Update(true,true,false,0,0,false,100+i)==Scene::None);
    assert(s.Update(true,true,false,0,0,false,130)==Scene::Neutral);
    for(unsigned i=0;i<100;++i)assert(s.Update(true,true,false,1,1,false,200+i)==Scene::None);
    for(unsigned i=0;i<29;++i)assert(s.Update(true,true,false,1,1,false,5000+i)==Scene::None);
    assert(s.Update(true,true,false,1,1,false,5030)==Scene::Sky);
    // Tracking loss and another trace reset stability; no partial capture.
    for(unsigned i=0;i<20;++i)s.Update(true,true,false,2,0,true,10000+i);
    assert(s.Update(true,false,false,2,0,true,10021)==Scene::None);
    for(unsigned i=0;i<29;++i)assert(s.Update(true,true,false,2,0,true,10100+i)==Scene::None);
    assert(s.Update(true,true,false,2,0,true,10130)==Scene::GravityInput);
    for(unsigned i=0;i<100;++i)assert(s.Update(true,true,false,3,0,true,16000+i)==Scene::None);
    Schedule busy;
    for(unsigned i=0;i<40;++i)assert(busy.Update(true,true,true,0,1,false,i)==Scene::None);
    for(unsigned i=0;i<40;++i)assert(busy.Update(true,true,false,4,1,false,i)==Scene::None);
    assert(busy.captured==0);
}
