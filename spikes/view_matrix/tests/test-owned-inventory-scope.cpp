// Own-process fake engine only: exercises the actual scoped runtime handler,
// including SEH unwind restoration. Never opens/attaches to Singularity.
//
// 2026-10-19: the owned-inventory snapshot now reads the possession chain
// through the AUDITED native layout (pawn+0x3D0 -> manager+0x1E0 ->
// item+0x1E0, weapon name at item+0x2E8) and validates every object by its
// class name. The wheel falls back to the accepted mesh-observed set when the
// snapshot is not ready, and the bytecode write is withdrawn.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
static const int OBJ_NAME=0x2c,OBJ_CLASS=0x34,OBJ_OUTER=0x28;
static int g_upChildren=0x4c,g_upNext=0x40;
static volatile LONG g_keepAcquiredWeapons=1,g_ownedWeaponWheel=1,g_gamePaused=0,g_vrMenuClass=0;
static volatile LONG g_inCinematic=0;
static uintptr_t g_playerPawn=0;
static LONG Rd(volatile LONG& n){return InterlockedCompareExchange(&n,0,0);}
static std::unordered_map<uintptr_t,std::string> objectNames;
static std::unordered_map<int,std::string> globalNames;
static std::unordered_map<std::string,int> offsets;
static uintptr_t controller=0;
static bool NameOf(uintptr_t p,char* out,size_t cap) {
  const auto it=objectNames.find(p);if(it==objectNames.end())return false;
  strncpy_s(out,cap,it->second.c_str(),_TRUNCATE);return true;
}
static bool NameFromIndex(int i,char* out,size_t cap) {
  const auto it=globalNames.find(i);if(it==globalNames.end())return false;
  strncpy_s(out,cap,it->second.c_str(),_TRUNCATE);return true;
}
static uintptr_t FindController(){return controller;}
static int PropOffsetInChain(uintptr_t,const char* name,int* depthOut=nullptr){
  const auto it=offsets.find(name);
  if(it==offsets.end())return -1;
  if(depthOut)*depthOut=1;
  return it->second;
}
static void Log(const char*,...){}
namespace PauseRttProbe {
static bool SafeRange(uintptr_t p,size_t n) {
  MEMORY_BASIC_INFORMATION m{};
  return p&&VirtualQuery((void*)p,&m,sizeof(m))&&m.State==MEM_COMMIT&&
  !(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&
  p+n>=p&&p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;
}
static uintptr_t Pointer(uintptr_t p){return SafeRange(p,4)?*(const uintptr_t*)p:0;}
}
#include "../native_owned_inventory.h"
static unsigned checks=0,calls=0;
static bool raiseFault=false;
static unsigned selections=0;
static void* selectedPawn=nullptr;
static NativeOwnedInventory::Name selectedName{};
static void __fastcall SelectWeapon(void* self,void*,NativeOwnedInventory::Name name) {
  ++selections;selectedPawn=self;selectedName=name;
}
static unsigned char expectedDrop[11]{};
static void Check(bool pass){++checks;if(!pass){std::fprintf(stderr,"FAIL scope check %u\n",checks);std::exit(1);}}
// The original interpreter entry. The production call site runs it whenever
// the scoped handler did not take the call, so the mock mirrors that.
static void __fastcall Original(void*,void*,void*,void*) {
  ++calls;
  // The audited script bytes must be untouched: the drop expression is NOT
  // neutralised any more, so the original opcode stays 0x1B.
  for(unsigned i=0;i<inventoryretention::DropBytes;++i)
   Check(*(const unsigned char*)(NativeOwnedInventory::script+inventoryretention::DropAt+i)==expectedDrop[i]);
  if(raiseFault)RaiseException(0xE0000011,0,0,nullptr);
}
static bool Scoped(void* self,void* stack,int name) {
  const bool handled=NativeOwnedInventory::InvokeRetained(self,nullptr,stack,nullptr,name,8,Original);
  if(!handled)Original(self,nullptr,stack,nullptr);
  return handled;
}
static bool CatchFault(void* self,void* stack) {
  __try { Scoped(self,stack,42); }
  __except(GetExceptionCode()==0xE0000011?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
   return true;
  }
  return false;
}
int main(int argc,char** argv) {
  g_keepAcquiredWeapons=g_ownedWeaponWheel=0;
  NativeOwnedInventory::Poll();NativeOwnedInventory::Queue(0);Check(NativeOwnedInventory::Mask()==0);
  g_keepAcquiredWeapons=g_ownedWeaponWheel=1;
  Check(argc==2);
  std::ifstream f(argv[1],std::ios::binary);Check(bool(f));f.seekg(4079214+44);
  std::vector<unsigned char> code(284);f.read((char*)code.data(),code.size());Check(f.gcount()==284);
  std::vector<std::vector<unsigned char>> objects(64,std::vector<unsigned char>(0x400));
  unsigned cursor=0;
  for(auto r:inventoryretention::Objects) {
   const uintptr_t o=(uintptr_t)objects[cursor++].data();objectNames[o]=r.name;
   memcpy(code.data()+r.at,&o,4);
  }
  int index=100;
  for(auto r:inventoryretention::Names) {
   globalNames[index]=r.name;memcpy(code.data()+r.at,&index,4);
   memset(code.data()+r.at+4,0,4);++index;
  }
  const uintptr_t fn=(uintptr_t)objects[40].data(),cls=(uintptr_t)objects[41].data();
  const uintptr_t outer=(uintptr_t)objects[42].data(),pawn=(uintptr_t)objects[43].data();
  const uintptr_t manager=(uintptr_t)objects[44].data();
  objectNames[cls]="Function";objectNames[outer]="RvInventoryManager";
  memcpy((void*)(fn+OBJ_CLASS),&cls,4);memcpy((void*)(fn+OBJ_OUTER),&outer,4);
  memcpy((void*)(pawn+0x3d0),&manager,4);
  uintptr_t stack[8]{};stack[2]=fn;
  NativeOwnedInventory::function=fn;NativeOwnedInventory::functionName=42;
  NativeOwnedInventory::script=(uintptr_t)code.data();NativeOwnedInventory::pawn=pawn;
  NativeOwnedInventory::manager=manager;NativeOwnedInventory::thread=GetCurrentThreadId();
  const auto original=code;
  memcpy(expectedDrop,code.data()+inventoryretention::DropAt,sizeof(expectedDrop));
  // The scope checks stay, but the bytecode write is withdrawn: the script
  // bytes must be IDENTICAL after a scoped local pickup.
  Check(!Scoped((void*)manager,stack,42));
  Check(calls==1&&code==original&&true);
  Check(!Scoped((void*)pawn,stack,42));
  Check(!Scoped((void*)manager,stack,43));
  NativeOwnedInventory::thread^=1;
  Check(!Scoped((void*)manager,stack,42));
  NativeOwnedInventory::thread=GetCurrentThreadId();
  g_keepAcquiredWeapons=0;
  Check(!Scoped((void*)manager,stack,42));
  g_keepAcquiredWeapons=1;
  code[33]^=1;
  Check(!Scoped((void*)manager,stack,42));
  Check(code!=original); // the mutated script is not the audited shape
  code=original;raiseFault=true;
  Check(CatchFault((void*)manager,stack));
  Check(code==original&&true);
  raiseFault=false;
  // ---- the actual snapshot, with reflected fake fields: saved guns appear
  // before any mesh observation; no inventory/ammo write is permitted. ----
  controller=(uintptr_t)objects[45].data();
  const uintptr_t cc=(uintptr_t)objects[46].data(),pc=(uintptr_t)objects[47].data();
  const uintptr_t mc=(uintptr_t)objects[48].data(),wc=(uintptr_t)objects[53].data();
  objectNames[pc]="RvPlayerPawnSP";objectNames[mc]="RvInventoryManager";objectNames[wc]="RvWeaponShared";
  // The class chain: SuperField sits at Next-4, exactly as the engine layout has it.
  const uintptr_t super=0;
  memcpy((void*)(pc+g_upNext-4),&super,4);
  memcpy((void*)(mc+g_upNext-4),&super,4);
  memcpy((void*)(wc+g_upNext-4),&super,4);
  memcpy((void*)(controller+OBJ_CLASS),&cc,4);memcpy((void*)(controller+0x100),&pawn,4);
  memcpy((void*)(pawn+OBJ_CLASS),&pc,4);memcpy((void*)(manager+OBJ_CLASS),&mc,4);
  // The pawn's vtable: +0xB14 is the selector, +0xB18 the audited HasWeapon.
  static unsigned char vtable[0xb20];
  const uintptr_t hasWeapon=inventoryretention::HasWeaponImpl;
  const uintptr_t selector=(uintptr_t)&SelectWeapon;
  memcpy(vtable+0xb14,&selector,4);memcpy(vtable+0xb18,&hasWeapon,4);
  const uintptr_t vt=(uintptr_t)vtable;
  memcpy((void*)pawn,&vt,4);
  offsets={{"Pawn",0x100},{"InvManager",0x3d0},{"InventoryChain",0x1e0},
          {"Inventory",0x1e0},{"mWeaponName",0x2e8}};
  g_playerPawn=pawn;
  const uintptr_t first=(uintptr_t)objects[49].data();memcpy((void*)(manager+0x1e0),&first,4);
  const char* gunNames[]={"ARPistol","ARMachineGun","ARShotgun","ARSniperRifle"};
  for(unsigned i=0;i<4;++i){
   const uintptr_t it=(uintptr_t)objects[49+i].data();objectNames[it]=gunNames[i];
   globalNames[200+(int)i]=gunNames[i];
   memcpy((void*)(it+OBJ_CLASS),&wc,4);
   const uintptr_t link=i<2?(uintptr_t)objects[50+i].data():0;
   memcpy((void*)(it+0x1e0),&link,4);
   NativeOwnedInventory::Name n{200+(int)i,0};memcpy((void*)(it+0x2e8),&n,8);
  }
  const auto before=objects;
  Check(NativeOwnedInventory::Snapshot());
  Check(NativeOwnedInventory::Mask()==7&&NativeOwnedInventory::count==3&&objects==before);
  Check(NativeOwnedInventory::Ready());
  long owned[8]{};
  Check(NativeOwnedInventory::OwnedVerts(owned,8)==3);
  Check(owned[0]==5799&&owned[1]==11387&&owned[2]==8297);
  const uintptr_t third=(uintptr_t)objects[51].data(),fourth=(uintptr_t)objects[52].data();
  memcpy((void*)(third+0x1e0),&fourth,4);
  Check(NativeOwnedInventory::Snapshot());Check(NativeOwnedInventory::Mask()==15);
  Check(NativeOwnedInventory::count==4);
  // Exercise the actual virtual call, not only snapshot readiness. UObject[0]
  // is the vtable pointer; dereferencing it twice reads the first METHOD.
  NativeOwnedInventory::Queue(11387);NativeOwnedInventory::Poll();
  Check(selections==1&&selectedPawn==(void*)pawn);
  Check(selectedName.index==201&&selectedName.number==0);
  g_gamePaused=1;NativeOwnedInventory::Queue(5799);NativeOwnedInventory::Poll();
  Check(selections==1);g_gamePaused=0;
  const uintptr_t wrongTarget=0;
  memcpy(vtable+0xb18,&wrongTarget,4);
  NativeOwnedInventory::Queue(5799);NativeOwnedInventory::Poll();Check(selections==1);
  memcpy(vtable+0xb18,&hasWeapon,4);
  memcpy((void*)(fourth+0x1e0),&first,4); // cycle must fail closed
  NativeOwnedInventory::next=0;NativeOwnedInventory::Poll();
  Check(NativeOwnedInventory::Mask()==0);
  Check(!NativeOwnedInventory::Ready());
  memset((void*)(fourth+0x1e0),0,4);
  NativeOwnedInventory::next=0;NativeOwnedInventory::Poll();
  Check(NativeOwnedInventory::Mask()==15);
  // A pawn whose class is not the local player pawn must be refused.
  objectNames[pc]="RvSomeOtherPawn";
  NativeOwnedInventory::next=0;NativeOwnedInventory::Poll();
  Check(NativeOwnedInventory::Mask()==0);
  objectNames[pc]="RvPlayerPawnSP";
  g_playerPawn=0;controller=0;
  NativeOwnedInventory::next=0;NativeOwnedInventory::Poll();
  Check(NativeOwnedInventory::Mask()==0);
  Check(!NativeOwnedInventory::Ready());
  std::printf("PASS %u actual-handler scope/SEH checks; fake engine only\n",checks);
}
