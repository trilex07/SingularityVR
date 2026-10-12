#pragma once
#include "input_enum_throttle.h"
// Runtime stack/disassembly evidence: exact no-argument native helper forwards
// IDirectInput8W::EnumDevices(GAMECTRL, callback011D2180, nullptr, ATTACHEDONLY).
// OFF preserves EAX, arguments and callbacks. Opt-in throttle defers callbacks
// inside the explicitly flagged scan interval, reusing only a successful prior return value.
namespace NativeGamepadEnumProbe {
using Fn=DWORD (__cdecl*)();
static Fn original=nullptr;
static volatile LONG ownerThread=0,eligible=0,haveSuccess=0,lastScan=0,cachedResult=0;
static volatile LONG nativeCalls=0,deferredCalls=0;
static void SetContext(bool allowed) {
 const LONG tid=(LONG)GetCurrentThreadId();
 InterlockedCompareExchange(&ownerThread,tid,0);
 if(InterlockedCompareExchange(&ownerThread,0,0)==tid)InterlockedExchange(&eligible,allowed?1:0);
}
static void Poll(const char* logPath) {
 if(inputenumthrottle::Poll(logPath))InterlockedExchange(&haveSuccess,0);
 static DWORD last=0;const DWORD now=GetTickCount();if(last && DWORD(now-last)<1000u)return;last=now;
 const LONG calls=InterlockedExchange(&nativeCalls,0),deferred=InterlockedExchange(&deferredCalls,0);
 if(calls||deferred)Log("INPUT_ENUM_SCAN native=%ld deferred=%ld enabled=%d",calls,deferred,inputenumthrottle::Enabled()?1:0);
}
static DWORD __cdecl Observe() {
 const bool allowed=InterlockedCompareExchange(&eligible,0,0)!=0 &&
  InterlockedCompareExchange(&ownerThread,0,0)==(LONG)GetCurrentThreadId();
 const bool experiment=inputenumthrottle::Enabled() && allowed;
 if(experiment && inputenumthrottle::Defer(true,true,InterlockedCompareExchange(&haveSuccess,0,0)!=0,
       GetTickCount(),(DWORD)InterlockedCompareExchange(&lastScan,0,0))) {
  InterlockedIncrement(&deferredCalls);
  return (DWORD)InterlockedCompareExchange(&cachedResult,0,0);
 }
 InterlockedIncrement(&nativeCalls);
 hotpath::scope measured(g_hotPathSampler,hotpath::Family::native_gamepad_enumeration);
 const DWORD result=original();
 if(experiment) {
  if((result&0x80000000u)==0) {
   InterlockedExchange(&cachedResult,(LONG)result);
   InterlockedExchange(&lastScan,(LONG)GetTickCount());
   InterlockedExchange(&haveSuccess,1);
  } else InterlockedExchange(&haveSuccess,0); // Never cache a failed native result.
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
 const MH_STATUS created=MH_CreateHook((void*)address,(void*)&Observe,(void**)&original);
 const MH_STATUS enabled=created==MH_OK?MH_EnableHook((void*)address):created;
 if(created==MH_OK && enabled!=MH_OK){MH_RemoveHook((void*)address);original=nullptr;}
 Log("native gamepad enumeration probe: create=%d enable=%d (default OFF throttle; native call preserved when OFF)",(int)created,(int)enabled);
}
}
