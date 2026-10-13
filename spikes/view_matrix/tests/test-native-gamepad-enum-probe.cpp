#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <cassert>
#include "../../../third_party/minhook/include/MinHook.h"
#include "../hotpath_sampler.h"
static hotpath::sampler g_hotPathSampler;
static unsigned nativeCalls=0,hookCalls=0;
static DWORD fakeResult=0x88780078u;
static bool emitDevices=false,changeDuringScan=false;
static DWORD __cdecl FakeNative();
static DWORD __cdecl FakeIdentity(){return 42;}
static bool Readable(void*,size_t){return false;}
static void Log(const char*,...){}
MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*){++hookCalls;return MH_ERROR_UNSUPPORTED_FUNCTION;}
MH_STATUS WINAPI MH_EnableHook(LPVOID){++hookCalls;return MH_ERROR_UNSUPPORTED_FUNCTION;}
MH_STATUS WINAPI MH_RemoveHook(LPVOID){++hookCalls;return MH_ERROR_UNSUPPORTED_FUNCTION;}
MH_STATUS WINAPI MH_DisableHook(LPVOID){++hookCalls;return MH_ERROR_UNSUPPORTED_FUNCTION;}
#include "../native_gamepad_enum_probe.h"
static DWORD __cdecl FakeNative(){
 ++nativeCalls;
 if(emitDevices){
  DIDEVICEINSTANCEW d{};d.dwSize=sizeof(d);d.guidInstance.Data1=1;
  NativeGamepadEnumProbe::ObserveCallback(&d,nullptr);
  d.guidInstance.Data1=2;NativeGamepadEnumProbe::ObserveCallback(&d,nullptr);
 }
 if(changeDuringScan)NativeGamepadEnumProbe::cache.Invalidate();
 return fakeResult;
}
int main(){
 NativeGamepadEnumProbe::original=&FakeNative;
 g_hotPathSampler.configure(false);
 assert(NativeGamepadEnumProbe::Observe()==0x88780078u && nativeCalls==1);
 assert(g_hotPathSampler.read(hotpath::Family::native_gamepad_enumeration).calls==0);
 g_hotPathSampler.configure(true,1);
 assert(NativeGamepadEnumProbe::Observe()==0x88780078u && nativeCalls==2);
 assert(g_hotPathSampler.read(hotpath::Family::native_gamepad_enumeration).calls==1);
 assert(g_hotPathSampler.advance_frame());
 NativeGamepadEnumProbe::SetContext(true);
 InterlockedExchange(&inputenumthrottle::enabled,1);
 NativeGamepadEnumProbe::originalCallback=[](const DIDEVICEINSTANCEW*,void*)->BOOL{return TRUE;};
 NativeGamepadEnumProbe::notification=(HCMNOTIFICATION)1;
 NativeGamepadEnumProbe::interfaceIdentity=&FakeIdentity;
 g_hotPathSampler.configure(false);
 assert(NativeGamepadEnumProbe::Observe()==fakeResult && nativeCalls==3);
 assert(NativeGamepadEnumProbe::Observe()==fakeResult && nativeCalls==4); // failure not cached
 fakeResult=0;
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==5);
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==5); // only enum call deferred
 NativeGamepadEnumProbe::cache.Invalidate();
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==6); // device topology forces scan
 NativeGamepadEnumProbe::SetContext(false);
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==7);
 NativeGamepadEnumProbe::SetContext(true);
 InterlockedExchange(&NativeGamepadEnumProbe::ownerThread,GetCurrentThreadId()+1);
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==8); // other thread passes through
 InterlockedExchange(&inputenumthrottle::enabled,0);
 NativeGamepadEnumProbe::Install();NativeGamepadEnumProbe::Install();
 assert(hookCalls==0); // Unsupported/unreadable executable must remain untouched.
 InputEnumerationCache snapshot;
 DIDEVICEINSTANCEW instance{};instance.dwSize=sizeof(instance);instance.guidInstance.Data1=123;
 snapshot.Begin(42);snapshot.Append(&instance);snapshot.Finish(0);
 assert(snapshot.Ready(42) && !snapshot.Ready(43));
 unsigned callbacks=0;
 assert(snapshot.Replay([&](const DIDEVICEINSTANCEW* d,void* user){assert(!user && d->guidInstance.Data1==123);++callbacks;return TRUE;})==0);
 assert(callbacks==1);snapshot.Invalidate();assert(!snapshot.Ready(42));
 snapshot.Begin(42);snapshot.Append(&instance);snapshot.Invalidate();snapshot.Finish(0);assert(!snapshot.Ready(42));
 snapshot.Begin(42);snapshot.Append(&instance);snapshot.Finish(0x80004005u);assert(!snapshot.Ready(42));
 snapshot.Begin(42);for(unsigned i=0;i<65;++i)snapshot.Append(&instance);snapshot.Finish(0);assert(!snapshot.Ready(42));
 snapshot.Begin(42);instance.dwSize=1;snapshot.Append(&instance);snapshot.Finish(0);assert(!snapshot.Ready(42));
 snapshot.Begin(42);snapshot.Abort();snapshot.Finish(0);assert(!snapshot.Ready(42));
 InterlockedExchange(&NativeGamepadEnumProbe::ownerThread,0);
 NativeGamepadEnumProbe::SetContext(true);
 InterlockedExchange(&inputenumthrottle::enabled,1);
 NativeGamepadEnumProbe::cache.Invalidate();emitDevices=true;
 unsigned callbacksBefore=(unsigned)NativeGamepadEnumProbe::capturedCallbacks;
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==9);
 assert((unsigned)NativeGamepadEnumProbe::capturedCallbacks==callbacksBefore+2);
 unsigned replaysBefore=(unsigned)NativeGamepadEnumProbe::replayedCallbacks;
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==9);
 assert((unsigned)NativeGamepadEnumProbe::replayedCallbacks==replaysBefore+2);
 NativeGamepadEnumProbe::TopologyChanged(nullptr,nullptr,CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL,nullptr,0);
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==10);
 NativeGamepadEnumProbe::cache.Invalidate();changeDuringScan=true;
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==11);
 assert(!NativeGamepadEnumProbe::cache.Ready(42));changeDuringScan=false;
 NativeGamepadEnumProbe::busy=true;
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==12);
 assert(!NativeGamepadEnumProbe::cache.Ready(42));NativeGamepadEnumProbe::busy=false;
 NativeGamepadEnumProbe::notification=nullptr; // Failed notification setup means passthrough.
 assert(NativeGamepadEnumProbe::Observe()==0 && nativeCalls==13);
}
