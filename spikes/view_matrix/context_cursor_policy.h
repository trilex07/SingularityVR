#pragma once
namespace cursorvisibility {
enum class VisibilityResult { Unchanged, Hidden, Restored, Refused };
// Ownership lives on the cursor MovieClip, not a retained UObject pointer.
// Never revive a cursor that was already hidden by the game.
template<class Backend> VisibilityResult UpdateVisibility(Backend& b,bool hide) {
    const bool owned=b.Owned(),visible=b.Visible();
    if(hide){
        if(!visible)return VisibilityResult::Unchanged;
        b.SetOwned(true);
        if(!b.Owned())return VisibilityResult::Refused;
        b.SetVisible(false);
        if(b.Visible()){b.SetOwned(owned);return VisibilityResult::Refused;}
        return VisibilityResult::Hidden;
    }
    if(!owned)return VisibilityResult::Unchanged;
    b.SetVisible(true);
    if(!b.Visible())return VisibilityResult::Refused;
    b.SetOwned(false);
    return VisibilityResult::Restored;
}
}
