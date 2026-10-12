#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <cassert>
#include "../../../third_party/minhook/include/MinHook.h"
#include "../hotpath_sampler.h"
static hotpath::sampler g_hotPathSampler;
static unsigned nativeCalls=0,hookCalls=0;
static DWORD __cdecl FakeNative(){++nativeCalls;return 0x88780078u;}
static bool Readable(void*,size_t){return false;}
static void Log(const char*,...){}
MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*){++hookCalls;return MH_ERROR_UNSUPPORTED_FUNCTION;}
MH_STATUS WINAPI MH_EnableHook(LPVOID){++hookCalls;return MH_ERROR_UNSUPPORTED_FUNCTION;}
MH_STATUS WINAPI MH_RemoveHook(LPVOID){++hookCalls;return MH_ERROR_UNSUPPORTED_FUNCTION;}
#include "../native_gamepad_enum_probe.h"
int main(){
 NativeGamepadEnumProbe::original=&FakeNative;
 g_hotPathSampler.configure(false);
 assert(NativeGamepadEnumProbe::Observe()==0x88780078u && nativeCalls==1);
 assert(g_hotPathSampler.read(hotpath::Family::native_gamepad_enumeration).calls==0);
 g_hotPathSampler.configure(true,1);
 assert(NativeGamepadEnumProbe::Observe()==0x88780078u && nativeCalls==2);
 assert(g_hotPathSampler.read(hotpath::Family::native_gamepad_enumeration).calls==1);
 assert(g_hotPathSampler.advance_frame());
 NativeGamepadEnumProbe::Install();NativeGamepadEnumProbe::Install();
 assert(hookCalls==0); // Unsupported/unreadable executable must remain untouched.
}
