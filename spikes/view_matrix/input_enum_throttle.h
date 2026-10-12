#pragma once
// One-variable experiment: defer the proven native device scan, never input reads.
namespace inputenumthrottle {
static volatile LONG enabled=0;
inline bool Enabled(){return InterlockedCompareExchange(&enabled,0,0)!=0;}
inline bool Defer(bool on,bool eligible,bool haveSuccess,DWORD now,DWORD last) {
 return on && eligible && haveSuccess && DWORD(now-last)<500u;
}
inline bool Poll(const char* logPath) {
 static DWORD lastPoll=0;const DWORD now=GetTickCount();
 if(lastPoll && DWORD(now-lastPoll)<1000u)return false;lastPoll=now;
 char path[MAX_PATH]{};strcpy_s(path,logPath);char* end=strrchr(path,'\\');if(!end)return false;
 strcpy_s(end+1,sizeof(path)-(end+1-path),"input-enum-throttle.flag");
 WIN32_FILE_ATTRIBUTE_DATA data{};bool on=false;
 if(GetFileAttributesExA(path,GetFileExInfoStandard,&data) && !(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
  FILETIME current;GetSystemTimeAsFileTime(&current);ULARGE_INTEGER n{},w{};
  n.LowPart=current.dwLowDateTime;n.HighPart=current.dwHighDateTime;
  w.LowPart=data.ftLastWriteTime.dwLowDateTime;w.HighPart=data.ftLastWriteTime.dwHighDateTime;
  if(n.QuadPart>=w.QuadPart && n.QuadPart-w.QuadPart<20ull*60*10000000) {
   HANDLE file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
   if(file!=INVALID_HANDLE_VALUE) {
    char payload[64]{};DWORD bytes=0;const char expected[]="SingularityVR input enum throttle v1";
    on=ReadFile(file,payload,sizeof(payload)-1,&bytes,nullptr) && bytes==sizeof(expected)-1 && !memcmp(payload,expected,bytes);
    CloseHandle(file);
   }
  }
 }
 const bool changed=on!=Enabled();if(changed) {
  InterlockedExchange(&enabled,on?1:0);
  Log("INPUT_ENUM_THROTTLE enabled=%d (500ms scan interval; gameplay/emulated-pad/owner-thread only)",on?1:0);
 }
 return changed;
}
}
