#pragma once
// Include after Windows, MinHook and the mod's Log declaration. x86 only.
// These guards bypass only verified mouse-specific blocks when the real device is absent.
namespace missing_mouse {
static void** mouseSlot=nullptr;
static void* originalInit=nullptr,*originalPoll=nullptr,*originalCoop=nullptr;
static void* continueInit=nullptr,*continuePoll=nullptr,*continueCoop=nullptr;
typedef int (__cdecl *PollHelper)();
static PollHelper originalHelper=nullptr;
__declspec(naked) static void GuardInit(){__asm {
 pushfd
 push eax
 mov eax, mouseSlot
 cmp dword ptr [eax],0
 je absent
 pop eax
 popfd
 jmp dword ptr [originalInit]
 absent:
 pop eax
 popfd
 jmp dword ptr [continueInit]
}}
__declspec(naked) static void GuardPoll(){__asm {
 pushfd
 push eax
 mov eax, mouseSlot
 cmp dword ptr [eax],0
 je absent
 pop eax
 popfd
 jmp dword ptr [originalPoll]
 absent:
 pop eax
 popfd
 jmp dword ptr [continuePoll]
}}
__declspec(naked) static void GuardCoop(){__asm {
 pushfd
 push eax
 mov eax, mouseSlot
 cmp dword ptr [eax],0
 je absent
 pop eax
 popfd
 jmp dword ptr [originalCoop]
 absent:
 pop eax
 popfd
 jmp dword ptr [continueCoop]
}}
static int __cdecl GuardHelper(){return mouseSlot && *mouseSlot && originalHelper ? originalHelper() : 0;}
struct Fingerprint{unsigned rva,length,hash;const unsigned short* reloc;unsigned relocCount;};
static bool Matches(const unsigned char* image,const Fingerprint& f){
 unsigned h=2166136261u;const unsigned delta=(unsigned)(uintptr_t)image-0x400000u;unsigned at=0,r=0;
 while(at<f.length){
  if(r<f.relocCount && at==f.reloc[r]){
   if(at+4>f.length)return false;unsigned value;memcpy(&value,image+f.rva+at,4);value-=delta;
   for(unsigned k=0;k<4;++k){h=(h^((value>>(k*8))&255u))*16777619u;}at+=4;++r;
  }else{h=(h^image[f.rva+at])*16777619u;++at;}
 }
 return h==f.hash && r==f.relocCount;
}
static const unsigned short reloc0[]={1,11,36,65};
static const unsigned short reloc1[]={1,18,31,44,50,61,74,98,111,130,226,272,284,317,326,357,369,402,411,441,471,484,536,566,607,638,669,727,836,882,894,927,936,967,979,1012,1021,1051,1270,1373,1379,1384,1404,1516,1522,1527,1547};
static const unsigned short reloc2[]={1,14};
static const unsigned short reloc3[]={1,18,32,42,48,60,73};
static const Fingerprint fingerprints[]={
{0xDD7439u,90u,0x72189A6Fu,reloc0,4},
{0xDD325Bu,1629u,0x7D8B63DBu,reloc1,47},
{0xDD5F2Bu,32u,0xC02CC0C4u,reloc2,2},
{0xDCF3E0u,96u,0x807D0696u,reloc3,7},
};
static bool Install(){
 static LONG state=0;const LONG previous=InterlockedCompareExchange(&state,1,0);
 if(previous)return previous==2;
 unsigned char* image=(unsigned char*)GetModuleHandleW(nullptr);bool valid=false;
 __try {
  const IMAGE_DOS_HEADER* dos=(const IMAGE_DOS_HEADER*)image;
  const IMAGE_NT_HEADERS32* nt=(const IMAGE_NT_HEADERS32*)(image+dos->e_lfanew);
  valid=dos->e_magic==IMAGE_DOS_SIGNATURE && nt->Signature==IMAGE_NT_SIGNATURE &&
        nt->FileHeader.Machine==IMAGE_FILE_MACHINE_I386 && nt->OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
        nt->OptionalHeader.SizeOfImage>0x191FC80u;
  if(valid)for(unsigned i=0;i<4;++i)if(!Matches(image,fingerprints[i])){valid=false;break;}
 } __except(EXCEPTION_EXECUTE_HANDLER){valid=false;}
 if(!valid){Log("missing-mouse compatibility: exact loaded-code fingerprints rejected; no game guards installed");InterlockedExchange(&state,3);return false;}
 mouseSlot=(void**)(image+0x191FC78u);
 continueInit=image+0xDD7493u;continuePoll=image+0xDD38B8u;continueCoop=image+0xDD5F4Bu;
 const MH_STATUS init=MH_Initialize();
 if(init!=MH_OK && init!=MH_ERROR_ALREADY_INITIALIZED){Log("missing-mouse compatibility: MinHook init=%d",(int)init);InterlockedExchange(&state,3);return false;}
 void* sites[]={image+0xDD7439u,image+0xDD325Bu,image+0xDD5F2Bu,image+0xDCF3E0u};
 void* detours[]={(void*)&GuardInit,(void*)&GuardPoll,(void*)&GuardCoop,(void*)&GuardHelper};
 void** originals[]={&originalInit,&originalPoll,&originalCoop,(void**)&originalHelper};
 unsigned created=0;MH_STATUS hr=MH_OK;
 for(;created<4;++created){hr=MH_CreateHook(sites[created],detours[created],originals[created]);if(hr!=MH_OK)break;}
 if(created==4){for(unsigned i=0;i<4;++i){hr=MH_QueueEnableHook(sites[i]);if(hr!=MH_OK)break;}if(hr==MH_OK)hr=MH_ApplyQueued();}
 if(created!=4 || hr!=MH_OK){
  for(unsigned i=0;i<created;++i){MH_DisableHook(sites[i]);MH_RemoveHook(sites[i]);}
  Log("missing-mouse compatibility: hook setup failed status=%d; removed guards",(int)hr);InterlockedExchange(&state,3);return false;
 }
 InterlockedExchange(&state,2);
 Log("missing-mouse compatibility: four exact-signature guards enabled; only null real mouse skips initialization/poll/cooperative blocks; controller/keyboard paths preserved");
 return true;
}
}
static bool InstallMissingMouseCompatibility(){return missing_mouse::Install();}
