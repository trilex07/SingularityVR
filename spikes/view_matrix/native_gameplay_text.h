#pragma once
#include "gameplay_text_safe_area.h"

// Input-thread only. The original HUD AVM1 ShowInformation() calls
// locationTxt.show(); TextDisplay attaches its named holder below the root
// txtMsgContainer. Do not touch NPC, objective, prompt, HP/ammo or subtitles.
namespace NativeGameplayText {
using String=NativeContextCursor::String;
using GetNumber=float (__thiscall*)(void*,const String*);
using SetNumber=void (__thiscall*)(void*,const String*,float);
static uintptr_t owner=0,record=0;
static GetNumber get=nullptr;
static SetNumber set=nullptr;
static DWORD next=0;
static unsigned reports=0;
static void Update() {
    if(!Rd(g_vrGameplayTextSafeArea)||!VrModeOn()||!InGameplay()||
       Rd(g_vrMenuClass)||Rd(g_vrUiMenu)||Rd(g_panelOpen)||
       Rd(g_gamePaused)||Rd(g_inCinematic))return;
    const DWORD now=GetTickCount();
    if((LONG)(now-next)<0)return;
    next=now+200;
    __try {
        const uintptr_t controller=FindController();
        const uintptr_t cc=controller?PauseRttProbe::Pointer(controller+OBJ_CLASS):0;
        static uintptr_t cachedCC=0,cachedHC=0;
        static int hudOff=-1,movieOff=-1;
        if(cc!=cachedCC){cachedCC=cc;hudOff=cc?PauseRttProbe::Property(cc,"myHUD","ObjectProperty"):-1;}
        const uintptr_t hud=hudOff>=0?PauseRttProbe::Pointer(controller+hudOff):0;
        const uintptr_t hc=hud?PauseRttProbe::Pointer(hud+OBJ_CLASS):0;
        if(hc!=cachedHC){cachedHC=hc;movieOff=hc?PauseRttProbe::Property(hc,"mGFxHUD","ObjectProperty"):-1;}
        const uintptr_t movie=movieOff>=0?PauseRttProbe::Pointer(hud+movieOff):0;
        if(!movie){owner=record=0;get=nullptr;set=nullptr;return;}
        const uintptr_t currentRecord=PauseRttProbe::Pointer(movie+0x48);
        if(movie!=owner||currentRecord!=record||!get||!set) {
            owner=record=0;get=nullptr;set=nullptr;next=now+2000;
            const uintptr_t cls=PauseRttProbe::Pointer(movie+OBJ_CLASS);
            if(PauseRttProbe::Property(cls,"pMovie","StructProperty")!=0x48)return;
            const int infoOff=PauseRttProbe::Property(cls,"MovieInfo","ObjectProperty");
            const uintptr_t info=infoOff>=0?PauseRttProbe::Pointer(movie+infoOff):0;
            const uintptr_t ic=info?PauseRttProbe::Pointer(info+OBJ_CLASS):0;
            const int raw=ic?PauseRttProbe::Property(ic,"RawData","ArrayProperty"):-1;
            if(raw<0||!PauseRttProbe::SafeRange(info+raw,12))return;
            const uintptr_t bytes=PauseRttProbe::Pointer(info+raw);
            const int count=*(const int*)(info+raw+4),capacity=*(const int*)(info+raw+8);
            if(count!=187541||!uiwitness::ArrayBounds(count,capacity)||
               !PauseRttProbe::SafeRange(bytes,count)||
               uiwitness::Fingerprint((const unsigned char*)bytes,count)!=0x21ACFC3AD63D0F40ull)return;
            const uintptr_t vt=PauseRttProbe::Pointer(movie);
            const uintptr_t g=PauseRttProbe::Pointer(vt+0x14c),s=PauseRttProbe::Pointer(vt+0x15c);
            if(!currentRecord||!NativeContextCursor::Method(g,213,0x123BF764C6F43D2Aull)||
               !NativeContextCursor::Method(s,183,0x51D61F73642B3837ull))return;
            owner=movie;record=currentRecord;get=(GetNumber)g;set=(SetNumber)s;
            next=now+200;
        }
        // The class's NAME is LocationTextDisplay and TextDisplay's ctor
        // attaches that movie to txtMsgContainer. Undefined paths have zero
        // dimensions and cannot pass Place().
        auto x=NativeContextCursor::Path(L"_root.txtMsgContainer.LocationTextDisplay._x");
        auto y=NativeContextCursor::Path(L"_root.txtMsgContainer.LocationTextDisplay._y");
        auto w=NativeContextCursor::Path(L"_root.txtMsgContainer.LocationTextDisplay._width");
        auto h=NativeContextCursor::Path(L"_root.txtMsgContainer.LocationTextDisplay._height");
        const float oldX=get((void*)owner,&x),oldY=get((void*)owner,&y);
        gameplaytextsafe::Placement target{};
        if(!gameplaytextsafe::AuthoredPosition(oldX,oldY)||
           !gameplaytextsafe::Place(get((void*)owner,&w),get((void*)owner,&h),target))return;
        if(std::fabs(oldX-target.x)<.5f&&std::fabs(oldY-target.y)<.5f)return;
        set((void*)owner,&x,target.x);set((void*)owner,&y,target.y);
        if(reports++<12)Log("gameplay location text safe area: holder=(%.1f,%.1f)->(%.1f,%.1f), readback=(%.1f,%.1f) [not pixel proof]",
            oldX,oldY,target.x,target.y,get((void*)owner,&x),get((void*)owner,&y));
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        owner=record=0;get=nullptr;set=nullptr;next=now+2000;
    }
}
}
