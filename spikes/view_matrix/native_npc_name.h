#pragma once
#include "npc_name_policy.h"
// Input-thread only. Original HUD AVM1 attaches npcTextField to txtMsgContainer.
// Numeric ABI and complete method fingerprints were audited independently.
namespace NativeNpcName {
using String=NativeContextCursor::String;
using GetNumber=float (__thiscall*)(void*,const String*);
using SetNumber=void (__thiscall*)(void*,const String*,float);
static uintptr_t owner=0,record=0;
static GetNumber get=nullptr;
static SetNumber set=nullptr;
static DWORD next=0;
static unsigned reports=0;
static void Update() {
    const int percent=(int)Rd(g_npcNameScalePct);
    if(percent>=100)return;
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
        auto x=NativeContextCursor::Path(L"_root.txtMsgContainer.npcTextField.nameTxt._xscale");
        auto y=NativeContextCursor::Path(L"_root.txtMsgContainer.npcTextField.nameTxt._yscale");
        const float sx=get((void*)owner,&x),sy=get((void*)owner,&y);
        if(npcname::ShouldScale(sx,sy,percent)) {
            set((void*)owner,&x,(float)percent);set((void*)owner,&y,(float)percent);
            if(reports++<12)Log("NPC name scale: requested=%d readback=%.1f,%.1f (not pixel proof)",
                percent,get((void*)owner,&x),get((void*)owner,&y));
        }
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        owner=record=0;get=nullptr;set=nullptr;next=now+2000;
    }
}
}
