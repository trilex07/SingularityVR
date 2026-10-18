#include "../context_cursor_gate.h"
#include "../menu_hud_scale.h"
#include <cassert>
#include <cstdio>

int main() {
    using contextcursor::MatchesObservedDraw;
    using contextcursor::ShouldSuppress;
    constexpr unsigned triangleList=4;

    assert(MatchesObservedDraw(true,true,true,true,true,triangleList,4,8,72));
    assert(!ShouldSuppress(false,true,true,true,true,true,triangleList,4,8,72));
    assert(!ShouldSuppress(true,true,true,true,true,true,triangleList,4,8,72));
    assert(ContextMenuDistance(4,false,true,.95f,1800)==1.8f);
    assert(ContextMenuDistance(4,false,false,.95f,1800)==.95f);
    assert(ContextMenuDistance(4,true,true,.95f,1800)==.95f);
    assert(ContextMenuDistance(3,false,true,.95f,1800)==.95f);
    assert(ContextMenuDistance(4,false,true,.95f,0)==.95f);
    assert(!ShouldSuppress(true,false,true,true,true,true,triangleList,4,8,72));
    assert(!ShouldSuppress(true,true,false,true,true,true,triangleList,4,8,72));
    assert(!ShouldSuppress(true,true,true,false,true,true,triangleList,4,8,72));
    assert(!ShouldSuppress(true,true,true,true,false,true,triangleList,4,8,72));
    assert(!ShouldSuppress(true,true,true,true,true,false,triangleList,4,8,72));
    assert(!ShouldSuppress(true,true,true,true,true,true,triangleList,2,4,36));
    assert(!ShouldSuppress(true,true,true,true,true,true,triangleList,4,8,68));
    std::puts("context cursor: falsified suppression inert; shared contextual comfort depth L1");
    return 0;
}
