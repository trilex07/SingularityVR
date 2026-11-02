#include "../tmd_gravity_grip.h"
#include <cassert>
int main(){
 tmdgravity::Grip g;
 assert(g.Update(true,true,true,false));
 assert(g.Update(true,true,true,false));
 assert(!g.Update(true,false,true,false));
 assert(!g.Update(true,true,true,true)); // support owns edge
 assert(!g.Update(true,true,true,false)); // cannot become power mid squeeze
 g.Update(true,false,true,false);
 assert(!g.Update(true,true,false,false)); // no TMD/tracking/menu
 assert(!g.Update(true,true,true,false)); // reacquire needs new edge
 g.Update(true,false,true,false);
 assert(g.Update(true,true,true,false));
 assert(!g.Update(true,true,false,false)); // focus loss releases power
 assert(!g.Update(true,true,true,false));
 g.Update(true,false,true,false);
 assert(!g.Update(false,true,true,false));
 assert(!g.Update(true,true,true,false)); // enabled while held
 g.Update(true,false,true,false);
 assert(g.Update(true,true,true,false));
 assert(!g.Update(true,true,true,true)); // reload interruption releases
 assert(!g.Update(true,true,true,false));
}
