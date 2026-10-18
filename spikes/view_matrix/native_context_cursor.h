#pragma once
#include "context_cursor_policy.h"

// Input-thread only. Offline native registration + vtable audit proves
// UGFxMovie::GetVariableBool and SetVariableBool, NOT ProcessEvent.
namespace NativeContextCursor {
struct String { const wchar_t* data; int count,capacity; };
using GetBool=int (__thiscall*)(void*,const String*);
using SetBool=void (__thiscall*)(void*,const String*,int);
static GetBool getBool=nullptr;
static SetBool setBool=nullptr;
static uintptr_t owner=0,record=0;
static unsigned reports=0;
static bool sessionTouched=false;
static bool Method(uintptr_t p,size_t size,unsigned long long hash){
    MEMORY_BASIC_INFORMATION m{};
    if(!p||!VirtualQuery((void*)p,&m,sizeof(m))||m.AllocationBase!=GetModuleHandleW(nullptr)||
       !PauseRttProbe::SafeRange(p,size))return false;
    const DWORD protect=m.Protect&0xff;
    if(protect!=PAGE_EXECUTE_READ&&protect!=PAGE_EXECUTE_READWRITE&&protect!=PAGE_EXECUTE_WRITECOPY)return false;
    return uiwitness::Fingerprint((const unsigned char*)p,size)==hash;
}
static void Configure(uintptr_t movie,uintptr_t cls,int kind){
    owner=record=0;getBool=nullptr;setBool=nullptr;
    if(!Rd(g_contextCursorNative)&&!sessionTouched)return;
    if(kind!=4||!movie||!cls)return;
    __try {
        const int movieOff=PauseRttProbe::Property(cls,"pMovie","StructProperty");
        const int infoOff=PauseRttProbe::Property(cls,"MovieInfo","ObjectProperty");
        // Both audited native methods dereference the reflected pMovie at +48.
        if(movieOff!=0x48||infoOff<0)return;
        const uintptr_t info=PauseRttProbe::Pointer(movie+infoOff);
        const uintptr_t type=info?PauseRttProbe::Pointer(info+OBJ_CLASS):0;
        const int raw=type?PauseRttProbe::Property(type,"RawData","ArrayProperty"):-1;
        if(raw<0||!PauseRttProbe::SafeRange(info+raw,12))return;
        const auto bytes=PauseRttProbe::Pointer(info+raw);
        const int count=*(const int*)(info+raw+4),capacity=*(const int*)(info+raw+8);
        if(!uiwitness::ArrayBounds(count,capacity)||!PauseRttProbe::SafeRange(bytes,count))return;
        const auto hash=uiwitness::Fingerprint((const unsigned char*)bytes,count);
        // Both audited AVM1 movies attach Mouse as _root.mouse, depth 65535.
        if(hash!=0x28B71A137B55BB7Aull&&hash!=0xD2E964C936CCE902ull)return;
        const uintptr_t vt=PauseRttProbe::Pointer(movie);
        const uintptr_t get=PauseRttProbe::Pointer(vt+0x148),set=PauseRttProbe::Pointer(vt+0x158);
        if(!Method(get,214,0x9D912591BFBC040Eull)||!Method(set,187,0xEF5A6D846CDD5E7Bull))return;
        record=PauseRttProbe::Pointer(movie+movieOff);
        if(!record)return;
        owner=movie;getBool=(GetBool)get;setBool=(SetBool)set;
        if(reports++<16)Log("context-cursor native: movie=%p asset=%016llX bool ABI verified; no package edits",(void*)movie,hash);
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){
        owner=record=0;getBool=nullptr;setBool=nullptr;
    }
}
template<size_t N> static String Path(const wchar_t(&s)[N]){return {s,(int)N,(int)N};}
struct Backend {
    void* movie;
    bool Visible(){auto p=Path(L"_root.mouse._visible");return getBool(movie,&p)!=0;}
    bool Owned(){auto p=Path(L"_root.mouse.__singularityVrHidden");return getBool(movie,&p)!=0;}
    void SetVisible(bool v){auto p=Path(L"_root.mouse._visible");setBool(movie,&p,v?1:0);}
    void SetOwned(bool v){auto p=Path(L"_root.mouse.__singularityVrHidden");setBool(movie,&p,v?1:0);}
};
static void Update(uintptr_t focused,bool hide){
    if(!hide&&!sessionTouched)return;
    if(!owner||focused!=owner||!getBool||!setBool)return;
    __try {
        // Never call an old focus/GC object. Inactive movies restore only on
        // refocus, before they can be presented without the VR replacement.
        if(PauseRttProbe::Pointer(owner+0x48)!=record)return;
        Backend b{(void*)owner};
        const auto r=cursorvisibility::UpdateVisibility(b,hide);
        if(r==cursorvisibility::VisibilityResult::Hidden)sessionTouched=true;
        if(r!=cursorvisibility::VisibilityResult::Unchanged&&reports++<32)
            Log("context-cursor native: movie=%p result=%d hide=%d visibility readback (not pixel proof)",
                (void*)owner,(int)r,hide?1:0);
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){
        owner=record=0;getBool=nullptr;setBool=nullptr;
        if(reports++<32)Log("context-cursor native: guarded failure; disabled for this focus");
    }
}
}
