#include "../context_cursor_gate.h"
#include "../menu_hud_scale.h"
#include "../context_cursor_policy.h"
#include <cassert>
#include <cstdio>

int main() {
    struct Backend {
        bool visible=true,owned=false,refuse=false;
        bool Visible(){return visible;} bool Owned(){return owned;}
        void SetVisible(bool v){if(!refuse)visible=v;}
        void SetOwned(bool v){if(!refuse)owned=v;}
    } b;
    using cursorvisibility::UpdateVisibility;
    using cursorvisibility::VisibilityResult;
    assert(UpdateVisibility(b,false)==VisibilityResult::Unchanged&&b.visible);
    assert(UpdateVisibility(b,true)==VisibilityResult::Hidden&&!b.visible&&b.owned);
    assert(UpdateVisibility(b,true)==VisibilityResult::Unchanged);
    assert(UpdateVisibility(b,false)==VisibilityResult::Restored&&b.visible&&!b.owned);
    b.visible=false;
    assert(UpdateVisibility(b,true)==VisibilityResult::Unchanged&&!b.owned);
    assert(UpdateVisibility(b,false)==VisibilityResult::Unchanged&&!b.visible);
    b.visible=true;b.refuse=true;
    assert(UpdateVisibility(b,true)==VisibilityResult::Refused&&b.visible);
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
