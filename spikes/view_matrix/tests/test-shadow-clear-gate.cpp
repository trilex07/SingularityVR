#include <windows.h>
#include <d3d9.h>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <initializer_list>
void Log(const char*,...){}
#include "../shadow_clear_gate.h"
int main(){
    using namespace shadowcleargate;
    for(DWORD flags:{0u,1u,2u,3u,4u,6u})for(LONG draw:{0L,50L,51L,1100L})for(bool r32:{false,true}) {
        enabled=false;assert(OpenForeground(flags,draw,r32)==((flags&D3DCLEAR_ZBUFFER)!=0&&draw>50));
        enabled=true;assert(OpenForeground(flags,draw,r32)==((flags&D3DCLEAR_ZBUFFER)!=0&&draw>50&&!r32));
    }
    assert(ignored>0&&allowed>0);
    puts("PASS: OFF equals accepted policy; ON excludes only late measured shadow clears");
}
