#pragma once
// Pure policy + transaction ordering; no engine state, hooks or allocations.
namespace frontmenu {
inline bool OwnsPresentation(bool enabled,int menuClass,bool focus,bool cinematic,bool paused){
    return enabled && menuClass==2 && focus && !(cinematic&&paused);
}
inline bool CurrentReceipt(bool composed,unsigned long long drawn,unsigned long long current){
    return composed && drawn==current;
}
struct Signals {
    bool enabled=false, vr=false, duplicated=false, focused=false;
    bool provider=false, panel=false, modal=false, sourceCurrent=false;
    bool planeValid=false, menuMode=false, video=false;
    int menuClass=0;
};
inline bool Eligible(const Signals& s) {
    return s.enabled && s.vr && s.duplicated && s.focused && s.provider &&
        !s.panel && !s.modal && s.sourceCurrent && s.planeValid &&
        s.menuMode && !s.video && s.menuClass==2;
}
// A successful per-eye composition MUST NOT use the older identical-left-crop
// path: that would discard right-eye panel geometry and its intended depth.
inline bool UseLegacyCrop(bool legacy,bool composed,int menuClass) {
    return legacy && !(composed && menuClass==2);
}
struct Result {bool composed=false, stateRestored=true, pixelsRestored=true;};
template<class Pass> Result Render(Pass& p) {
    Result r;
    if(!p.Capture())return r;
    const bool saved=p.SavePixels();
    bool good=saved && p.Setup() && p.Clear();
    bool began=false;
    if(good){began=p.BeginScene();good=began;}
    if(good)good=p.DrawEye(0) && p.DrawEye(1);
    if(began){const bool ended=p.EndScene();good=good&&ended;}
    r.stateRestored=p.Restore();
    good=good&&r.stateRestored;
    if(saved&&!good)r.pixelsRestored=p.RestorePixels();
    r.composed=good;
    return r;
}
} // namespace frontmenu
