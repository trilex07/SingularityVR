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
static volatile LONG g_keepAcquiredWeapons=1,g_ownedWeaponWheel=1,g_gamePaused=0,g_vrMenuClass=0,g_vrUiMenu=0;
static volatile LONG g_inCinematic=0;
static uintptr_t g_playerPawn=0;
static uintptr_t g_camera=0;
static bool IsLive(uintptr_t p){return p!=0;}
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
static uintptr_t g_processInternal=0,g_textLo=0,g_textHi=0;
#include "../inventory_vm_hook.h"
static unsigned pickupCalls=0;
static void* pickupSelf=nullptr;
static void* pickupArchetype=nullptr;
static int pickupArgs[3]{};
static int __fastcall OriginalPickup(void* self,void*,void* archetype,int select,int instant,int keep) {
 ++pickupCalls;pickupSelf=self;pickupArchetype=archetype;
 pickupArgs[0]=select;pickupArgs[1]=instant;pickupArgs[2]=keep;return 77;
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
  unsigned char dispatch[128]{};
  const unsigned char site[]={0x8b,0x14,0x95,0x30,0x87,0xcc,0x01};
  memcpy(dispatch+20,site,sizeof(site));memcpy(dispatch+80,site,sizeof(site));
  Check(inventoryvm::DispatchTable(dispatch,sizeof(dispatch))==0x1cc8730);
  dispatch[83]^=4;Check(inventoryvm::DispatchTable(dispatch,sizeof(dispatch))==0);
  dispatch[83]^=4;dispatch[80]=0;Check(inventoryvm::DispatchTable(dispatch,sizeof(dispatch))==0);
  Check(inventoryvm::DispatchTable(nullptr,128)==0);
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
  // Exact nested VirtualFunction seam: consume only this local pickup call.
  unsigned char vmFrame[40]{};
  uintptr_t vmCode=NativeOwnedInventory::script+inventoryretention::DropAt+1;
  memcpy(vmFrame+0x10,&NativeOwnedInventory::function,4);
  memcpy(vmFrame+0x14,&manager,4);memcpy(vmFrame+0x18,&vmCode,4);
  memcpy((void*)(pawn+0x3d0),&manager,4);
  NativeOwnedInventory::throwName=*(const int*)(NativeOwnedInventory::script+201);
  NativeOwnedInventory::ObserveThrow((void*)pawn,vmFrame);
  Check(PauseRttProbe::Pointer((uintptr_t)vmFrame+0x18)==vmCode&&code==original);
  NativeOwnedInventory::ObserveThrowEntry((void*)pawn,vmFrame,NativeOwnedInventory::throwName,0x10);
  Check(PauseRttProbe::Pointer((uintptr_t)vmFrame+0x18)==vmCode&&code==original);
  Check(!NativeOwnedInventory::SkipPickupThrow((void*)manager,vmFrame));
  Check(NativeOwnedInventory::SkipPickupThrow((void*)pawn,vmFrame));
  Check(PauseRttProbe::Pointer((uintptr_t)vmFrame+0x18)==NativeOwnedInventory::script+211);
  Check(code==original); // never edits shared bytecode
  Check(!NativeOwnedInventory::SkipPickupThrow((void*)pawn,vmFrame));
  memcpy(vmFrame+0x18,&vmCode,4);g_keepAcquiredWeapons=0;
  Check(!NativeOwnedInventory::SkipPickupThrow((void*)pawn,vmFrame));g_keepAcquiredWeapons=1;
  NativeOwnedInventory::thread^=1;
  Check(!NativeOwnedInventory::SkipPickupThrow((void*)pawn,vmFrame));
  NativeOwnedInventory::thread=GetCurrentThreadId();
  uintptr_t wrongNode=0;memcpy(vmFrame+0x10,&wrongNode,4);
  Check(!NativeOwnedInventory::SkipPickupThrow((void*)pawn,vmFrame));
  memcpy(vmFrame+0x10,&NativeOwnedInventory::function,4);
  code[209]^=1;Check(!NativeOwnedInventory::SkipPickupThrow((void*)pawn,vmFrame));code=original;
  // Exercise the actual opcode table adapter and its original-call ABI.
  static unsigned char interpreter[0x200]{};
  static uintptr_t opcodeTable[256]{};
  const uint32_t tableAddress=(uint32_t)(uintptr_t)opcodeTable;
  memcpy(interpreter+5+20,site,sizeof(site));memcpy(interpreter+5+80,site,sizeof(site));
  memcpy(interpreter+5+23,&tableAddress,4);memcpy(interpreter+5+83,&tableAddress,4);
  const unsigned char frameNode[]={0x8b,0x46,0x10};
  const unsigned char frameStep[]={0x8b,0x46,0x18,0x0f,0xb6,0x10,0x8b,0x4e,0x14};
  memcpy(interpreter+110,frameNode,sizeof(frameNode));memcpy(interpreter+130,frameStep,sizeof(frameStep));
  opcodeTable[0x1b]=(uintptr_t)&Original;
  g_processInternal=(uintptr_t)interpreter;g_textLo=(uintptr_t)&Original;g_textHi=g_textLo+1;
  InventoryVmHook::Install();
  Check(opcodeTable[0x1b]==(uintptr_t)&InventoryVmHook::VirtualCall);
  unsigned callsBefore=calls;
  ((InventoryVmHook::Exec)opcodeTable[0x1b])((void*)pawn,nullptr,vmFrame,nullptr);
  Check(calls==callsBefore&&PauseRttProbe::Pointer((uintptr_t)vmFrame+0x18)==NativeOwnedInventory::script+211);
  ((InventoryVmHook::Exec)opcodeTable[0x1b])((void*)manager,nullptr,vmFrame,nullptr);
  Check(calls==callsBefore+1&&code==original);
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
  for(unsigned mask=0;mask<16;++mask) {
    long published[4]{};unsigned resultMask=0;
    unsigned n=NativeOwnedInventory::VertsFromMask(mask,published,4);
    for(unsigned i=0;i<n;++i)resultMask|=inventoryretention::WeaponBit((int)published[i]);
    Check(resultMask==mask); // labels and icon gating consume the SAME atomic snapshot
  }
  // Renderer never reads the mutable engine-thread list/count.
  NativeOwnedInventory::count=0;
  Check(NativeOwnedInventory::OwnedVerts(owned,8)==3);
  NativeOwnedInventory::count=3;
  const uintptr_t third=(uintptr_t)objects[51].data(),fourth=(uintptr_t)objects[52].data();
  memcpy((void*)(third+0x1e0),&fourth,4);
  Check(NativeOwnedInventory::Snapshot());Check(NativeOwnedInventory::Mask()==15);
  Check(NativeOwnedInventory::count==4);
  // Native acquisition has a separate keep-existing argument. Exercise the
  // actual ABI and scope: only that fourth argument may change; never ammo,
  // script, objects, return value or other callers.
  const auto beforePickup=objects;
  InventoryVmHook::originalPickup=&OriginalPickup;
  Check(InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,1,0,0)==77);
  Check(pickupCalls==1&&pickupSelf==(void*)pawn&&pickupArchetype==(void*)first);
  Check(pickupArgs[0]==1&&pickupArgs[1]==0&&pickupArgs[2]==1);
  Check(objects==beforePickup); // no chain/ammo/object mutation by callback
  g_keepAcquiredWeapons=0;
  InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,1,0,0);
  Check(pickupArgs[2]==0);g_keepAcquiredWeapons=1;
  g_gamePaused=1;InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,1,0,0);
  Check(pickupArgs[2]==0);g_gamePaused=0;
  g_inCinematic=1;InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,1,0,0);
  Check(pickupArgs[2]==0);g_inCinematic=0;
  NativeOwnedInventory::thread^=1;
  InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,1,0,0);
  Check(pickupArgs[2]==0);NativeOwnedInventory::thread=GetCurrentThreadId();
  InventoryVmHook::PickupCall((void*)manager,nullptr,(void*)first,1,0,0);
  Check(pickupArgs[2]==0);
  InventoryVmHook::PickupCall((void*)pawn,nullptr,nullptr,1,0,0);
  Check(pickupArgs[2]==0);
  InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,1,0,9);
  Check(pickupArgs[2]==9);
  InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,7,0,0);
  Check(pickupArgs[0]==7&&pickupArgs[2]==0);
  InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,1,7,0);
  Check(pickupArgs[1]==7&&pickupArgs[2]==0);
  const uintptr_t savedManager=NativeOwnedInventory::manager;
  NativeOwnedInventory::manager=0;
  InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,1,0,0);
  Check(pickupArgs[2]==0);NativeOwnedInventory::manager=savedManager;
  InventoryVmHook::PickupCall(nullptr,nullptr,(void*)first,1,0,0);
  Check(pickupSelf==nullptr&&pickupArgs[2]==0);
  InventoryVmHook::PickupCall((void*)pawn,nullptr,(void*)first,0,1,1);
  Check(pickupArgs[0]==0&&pickupArgs[1]==1&&pickupArgs[2]==1);
  Check(!inventorypickup::Retain(true,true,true,true,false,false,false,1,0,0));
  Check(!inventorypickup::Hash(nullptr,inventorypickup::Bytes));
  unsigned char wrongBody[inventorypickup::Bytes]{};
  Check(inventorypickup::Hash(wrongBody,sizeof(wrongBody))!=inventorypickup::Fingerprint);
  Check(!inventorypickup::Hash(wrongBody,sizeof(wrongBody)-1));
  Check(objects==beforePickup);
  // Current camera ownership resolves the saved inventory even before the
  // render-side incremental pawn census reaches the new level's object.
  const uintptr_t camera=(uintptr_t)objects[54].data(),cameraClass=(uintptr_t)objects[55].data();
  objectNames[cameraClass]="RvPlayerCamera";objectNames[cc]="RvPlayerController";
  memcpy((void*)(camera+OBJ_CLASS),&cameraClass,4);
  memcpy((void*)(camera+0x100),&controller,4);memcpy((void*)(pawn+0x104),&controller,4);
  offsets["PCOwner"]=0x100;offsets["Controller"]=0x104;
  g_camera=camera;g_playerPawn=0;
  Check(NativeOwnedInventory::CameraOwnedPawn()==pawn);
  Check(NativeOwnedInventory::Snapshot()&&NativeOwnedInventory::Mask()==15);
  memset((void*)(pawn+0x104),0,4);
  Check(NativeOwnedInventory::CameraOwnedPawn()==0); // reciprocal ownership mandatory
  memcpy((void*)(pawn+0x104),&controller,4);
  g_camera=0;g_playerPawn=pawn;
  NativeOwnedInventory::next=GetTickCount64()+60000;
  NativeOwnedInventory::count=0;
  NativeOwnedInventory::Refresh();NativeOwnedInventory::Poll();
  Check(NativeOwnedInventory::count==4); // open bypasses delay, not thread ownership
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
  Check(!NativeOwnedInventory::function&&!NativeOwnedInventory::script);
  std::printf("PASS %u actual-handler scope/SEH checks; fake engine only\n",checks);
}
