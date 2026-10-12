#pragma once
// Runtime stack/disassembly evidence: exact no-argument native helper forwards
// IDirectInput8W::EnumDevices(GAMECTRL, callback011D2180, nullptr, ATTACHEDONLY).
// Preserve its EAX return, all native arguments and callback execution.
namespace NativeGamepadEnumProbe {
using Fn=DWORD (__cdecl*)();
static Fn original=nullptr;
static DWORD __cdecl Observe() {
 hotpath::scope measured(g_hotPathSampler,hotpath::Family::native_gamepad_enumeration);
 return original();
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
 Log("native gamepad enumeration probe: create=%d enable=%d (timing only, native call preserved)",(int)created,(int)enabled);
}
}
