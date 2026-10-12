#pragma once
#include "input_enum_throttle.h"
#include "input_enum_cache.h"
#include <cfgmgr32.h>
// Runtime stack/disassembly evidence: exact no-argument native helper forwards
// IDirectInput8W::EnumDevices(GAMECTRL, callback011D2180, nullptr, ATTACHEDONLY).
// OFF preserves EAX, arguments and callbacks. Opt-in topology cache replays
// native callbacks; only Windows discovery is omitted on a cache hit.
namespace NativeGamepadEnumProbe {
using Fn=DWORD (__cdecl*)();
static Fn original=nullptr;
static volatile LONG ownerThread=0,eligible=0;
static volatile LONG capturedCallbacks=0,replayedCallbacks=0,rejectedSnapshots=0;
using Callback=BOOL (CALLBACK*)(const DIDEVICEINSTANCEW*,void*);
static Callback originalCallback=nullptr;
static InputEnumerationCache cache;
static HCMNOTIFICATION notification=nullptr;
static bool busy=false,recording=false;
static DWORD ReadIdentity(){
 const void* address=(void*)0x01D1FC7Cu;
 return Readable((void*)address,sizeof(DWORD))?*(const DWORD*)address:0;
}
static DWORD (*interfaceIdentity)()=&ReadIdentity;
static DWORD CALLBACK TopologyChanged(HCMNOTIFICATION,PVOID,CM_NOTIFY_ACTION,PCM_NOTIFY_EVENT_DATA,DWORD){
 cache.Invalidate();return ERROR_SUCCESS;
}
static BOOL CALLBACK ObserveCallback(const DIDEVICEINSTANCEW* instance,void* user){
 const bool owner=(LONG)GetCurrentThreadId()==InterlockedCompareExchange(&ownerThread,0,0);
 if(owner && recording){InterlockedIncrement(&capturedCallbacks);cache.Append(instance);}else cache.Invalidate();
 const BOOL result=originalCallback(instance,user);
 if(owner && recording && !result)cache.Abort(); // A partial enumeration is not a complete snapshot.
 return result;
}
static volatile LONG nativeCalls=0,deferredCalls=0;
static void SetContext(bool allowed) {
 const LONG tid=(LONG)GetCurrentThreadId();
 InterlockedCompareExchange(&ownerThread,tid,0);
 if(InterlockedCompareExchange(&ownerThread,0,0)==tid){
  if(InterlockedExchange(&eligible,allowed?1:0)!=(allowed?1:0))cache.Invalidate();
 }
}
static void Poll(const char* logPath) {
 if(inputenumthrottle::Poll(logPath))cache.Invalidate();
 static DWORD last=0;const DWORD now=GetTickCount();if(last && DWORD(now-last)<1000u)return;last=now;
 const LONG calls=InterlockedExchange(&nativeCalls,0),deferred=InterlockedExchange(&deferredCalls,0);
 const LONG captured=InterlockedExchange(&capturedCallbacks,0),replayed=InterlockedExchange(&replayedCallbacks,0),rejected=InterlockedExchange(&rejectedSnapshots,0);
 if(calls||deferred||captured||replayed||rejected)Log("INPUT_ENUM_SCAN native=%ld deferred=%ld enabled=%d callback_capture=%ld callback_replay=%ld rejected=%ld",calls,deferred,inputenumthrottle::Enabled()?1:0,captured,replayed,rejected);
}
static DWORD __cdecl Observe() {
 const bool allowed=InterlockedCompareExchange(&eligible,0,0)!=0 &&
  InterlockedCompareExchange(&ownerThread,0,0)==(LONG)GetCurrentThreadId();
 const bool experiment=inputenumthrottle::Enabled() && allowed && originalCallback && notification;
 const DWORD identity=experiment?interfaceIdentity():0;
 if(experiment && !busy && cache.Ready(identity)) {
  InterlockedIncrement(&deferredCalls);
  busy=true;
  const DWORD result=cache.Replay([](const DIDEVICEINSTANCEW* instance,void* user){
   InterlockedIncrement(&replayedCallbacks);return originalCallback(instance,user);
  });
  busy=false;
  return result;
 }
 InterlockedIncrement(&nativeCalls);
 hotpath::scope measured(g_hotPathSampler,hotpath::Family::native_gamepad_enumeration);
 if(!allowed){cache.Invalidate();return original();} // No owner-thread snapshot writes on another thread.
 if(busy){recording=false;cache.Abort();return original();} // Reentrant COM/message pump: fail open.
 cache.Invalidate(); // OFF/native path may change device setup; never reuse its older snapshot.
 if(experiment && identity){cache.Begin(identity);recording=true;}
 busy=true;
 const DWORD result=original();
 busy=false;recording=false;
 if(experiment && identity){
  if(interfaceIdentity()!=identity)cache.Abort();
  cache.Finish(result);
  if(!cache.Ready(identity))InterlockedIncrement(&rejectedSnapshots);
 }
 return result;
}
static void Install() {
 static bool tried=false;if(tried)return;tried=true;
 if((uintptr_t)GetModuleHandleW(nullptr)!=0x00400000u)return;
 const uintptr_t address=0x011D2FC0u;
 const unsigned char signature[]={0xa1,0x7c,0xfc,0xd1,0x01,0x85,0xc0,0x74,0x13,
  0x8b,0x08,0x8b,0x51,0x10,0x6a,0x01,0x6a,0x00,0x68,0x80,0x21,0x1d,0x01,
  0x6a,0x04,0x50,0xff,0xd2,0xc3};
 if(!Readable((void*)address,sizeof(signature))||memcmp((void*)address,signature,sizeof(signature))) {
  Log("native gamepad enumeration probe: exact loaded-code signature rejected");return;
 }
 // Full callback fingerprint from the same owned live-text snapshot; COFF listing
 // addresses are offsets into .text, not VAs (base 00402000).
 const uintptr_t callbackAddress=0x011D2180u;
 if(!Readable((void*)callbackAddress,0x313u))return;
 unsigned fingerprint=2166136261u;
 for(unsigned i=0;i<0x313u;++i)fingerprint=(fingerprint^((const unsigned char*)callbackAddress)[i])*16777619u;
 if(fingerprint!=0x8735998Eu){Log("input enum cache: callback signature rejected");return;}
 HMODULE cm=GetModuleHandleW(L"cfgmgr32.dll");if(!cm)cm=LoadLibraryW(L"cfgmgr32.dll");
 using Register=CONFIGRET(WINAPI*)(PCM_NOTIFY_FILTER,PVOID,PCM_NOTIFY_CALLBACK,PHCMNOTIFICATION);
 using Unregister=CONFIGRET(WINAPI*)(HCMNOTIFICATION);
 auto reg=cm?(Register)GetProcAddress(cm,"CM_Register_Notification"):nullptr;
 auto unreg=cm?(Unregister)GetProcAddress(cm,"CM_Unregister_Notification"):nullptr;
 if(!reg||!unreg)return;
 CM_NOTIFY_FILTER filter{};filter.cbSize=sizeof(filter);filter.Flags=CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
 filter.FilterType=CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
 if(reg(&filter,nullptr,TopologyChanged,&notification)!=CR_SUCCESS)return;
 const MH_STATUS cbCreated=MH_CreateHook((void*)callbackAddress,(void*)&ObserveCallback,(void**)&originalCallback);
 const MH_STATUS cbEnabled=cbCreated==MH_OK?MH_EnableHook((void*)callbackAddress):cbCreated;
 if(cbEnabled!=MH_OK){
  if(cbCreated==MH_OK)MH_RemoveHook((void*)callbackAddress);
  originalCallback=nullptr;unreg(notification);notification=nullptr;return;
 }
 const MH_STATUS created=MH_CreateHook((void*)address,(void*)&Observe,(void**)&original);
 const MH_STATUS enabled=created==MH_OK?MH_EnableHook((void*)address):created;
 if(created==MH_OK && enabled!=MH_OK){MH_RemoveHook((void*)address);original=nullptr;}
 if(enabled!=MH_OK){
  MH_DisableHook((void*)callbackAddress);MH_RemoveHook((void*)callbackAddress);
  originalCallback=nullptr;unreg(notification);notification=nullptr;
 }
 Log("native gamepad enumeration probe: create=%d enable=%d (default OFF throttle; native call preserved when OFF)",(int)created,(int)enabled);
}
}
