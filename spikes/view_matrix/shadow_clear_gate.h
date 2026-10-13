#pragma once
// Validated foreground boundary fix. Native Clear arguments/state untouched.
namespace shadowcleargate {
static volatile LONG enabled=0,ignored=0,allowed=0;
static volatile LONG configured=0;
inline void Configure(bool on){InterlockedExchange(&configured,on?1:0);}
inline bool Requested(bool lease){return lease||InterlockedCompareExchange(&configured,0,0)!=0;}
inline bool Enabled(){return InterlockedCompareExchange(&enabled,0,0)!=0;}
inline bool OpenForeground(DWORD flags,LONG drawIndex,bool measuredOffscreenR32) {
    if(!(flags&D3DCLEAR_ZBUFFER)||drawIndex<=50)return false;
    if(Enabled()&&measuredOffscreenR32){InterlockedIncrement(&ignored);return false;}
    if(Enabled())InterlockedIncrement(&allowed);
    return true;
}
inline void Poll(const char* logPath) {
    static DWORD last=0;const DWORD now=GetTickCount();if(last&&now-last<1000)return;last=now;
    char path[MAX_PATH]{};strcpy_s(path,logPath);char* end=strrchr(path,'\\');if(!end)return;
    strcpy_s(end+1,sizeof(path)-(end+1-path),"shadow-clear-gate.flag");
    WIN32_FILE_ATTRIBUTE_DATA data{};bool on=false;
    if(GetFileAttributesExA(path,GetFileExInfoStandard,&data)&&!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
        FILETIME time;GetSystemTimeAsFileTime(&time);ULARGE_INTEGER n{},w{};
        n.LowPart=time.dwLowDateTime;n.HighPart=time.dwHighDateTime;
        w.LowPart=data.ftLastWriteTime.dwLowDateTime;w.HighPart=data.ftLastWriteTime.dwHighDateTime;
        on=n.QuadPart>=w.QuadPart&&n.QuadPart-w.QuadPart<20ull*60*10000000;
    }
    on=Requested(on);
    if(on!=Enabled()){InterlockedExchange(&enabled,on?1:0);Log("SHADOW_CLEAR_GATE enabled=%d (native Clear unchanged)",on?1:0);}
    const LONG skipped=InterlockedExchange(&ignored,0),passed=InterlockedExchange(&allowed,0);
    if(skipped||passed)Log("SHADOW_CLEAR_GATE ignoredOffscreenR32=%ld allowed=%ld",skipped,passed);
}
}
