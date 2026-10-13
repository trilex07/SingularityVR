#include "../cinema_exit_profile.h"
#include <cassert>
#include <cstdio>
int main() {
    using namespace cinemaexitprofile;
    Gate g;
    assert(!g.Ready(true,true,false,false,false)); // menu/load at boot is not a post-cinema window
    g.Observe(true,false);assert(!g.Ready(true,true,false,false,false)); // intro flag alone
    g.Observe(true,true);
    assert(!g.Ready(true,false,false,false,false)); // scene mode has not exited yet
    assert(!g.Ready(true,true,true,false,false)); // second native ON still active
    assert(!g.Ready(false,true,false,false,false));
    assert(!g.Ready(true,true,false,true,false));
    assert(!g.Ready(true,true,false,false,true));
    assert(g.Ready(true,true,false,false,false));
    assert(IsRequest(request,sizeof(request)-1));
    assert(!IsRequest(nullptr,sizeof(request)-1));
    assert(!IsRequest(request,sizeof(request)));
    const char generic[64]="SingularityVR gameplay hotpath v1";
    assert(!IsRequest(generic,sizeof(request)-1));
    assert(!Expired(0,999999));assert(!Expired(100,45099));assert(Expired(100,45100));
    assert(!Expired(100,99));
    std::puts("PASS: cinema exit probe qualification / second ON / menu+pause+focus / exact request / 45s bound");
}
