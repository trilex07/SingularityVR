// Own-process fake engine only: exercises the actual scoped runtime handler,
// including SEH unwind restoration. Never opens/attaches to Singularity.
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
static int g_upChildren=0,g_upNext=0;
static volatile LONG g_keepAcquiredWeapons=1,g_ownedWeaponWheel=1,g_gamePaused=0,g_vrMenuClass=0;
static volatile LONG g_inCinematic=0;
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
static void Log(const char*,...){}
namespace PauseRttProbe {
static bool SafeRange(uintptr_t p,size_t n) {
 MEMORY_BASIC_INFORMATION m{};
 return p&&VirtualQuery((void*)p,&m,sizeof(m))&&m.State==MEM_COMMIT&&
 !(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&
 p+n>=p&&p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;
}
static uintptr_t Pointer(uintptr_t p){return SafeRange(p,4)?*(const uintptr_t*)p:0;}
static int Property(uintptr_t cls,const char* name,const char*){
 const auto type=objectNames.find(cls);
 if(!strcmp(name,"InvManager")&&type!=objectNames.end()&&type->second=="RvPlayerPawnSP")return 0x3d0;
 const auto it=offsets.find(name);return it==offsets.end()?-1:it->second;
}
}
namespace NativeContextCursor {static bool Method(uintptr_t,size_t,unsigned long long){return false;}}
#include "../native_owned_inventory.h"
static unsigned checks=0,calls=0;
static bool raiseFault=false;
static bool nested=false;
static bool nestedFault=false;
static unsigned char expectedDrop[11]{};
static void Check(bool pass){++checks;if(!pass){std::fprintf(stderr,"FAIL scope check %u\n",checks);std::exit(1);}}
static void __fastcall OtherInventory(void*,void*,void*,void*) {
 Check(NativeOwnedInventory::patchSuspended);
 Check(!memcmp((void*)(NativeOwnedInventory::script+inventoryretention::DropAt),expectedDrop,sizeof(expectedDrop)));
 if(nestedFault)RaiseException(0xE0000011,0,0,nullptr);
}
static void __fastcall Original(void*,void*,void*,void*) {
 ++calls;
 for(unsigned i=0;i<inventoryretention::DropBytes;++i)
  Check(*(const unsigned char*)(NativeOwnedInventory::script+inventoryretention::DropAt+i)==0x0b);
 if(nested) {
  uintptr_t stack[8]{};stack[2]=NativeOwnedInventory::function;
  Check(NativeOwnedInventory::InvokeRetained((void*)0x1234,nullptr,stack,nullptr,42,8,OtherInventory));
  Check(!NativeOwnedInventory::patchSuspended);
  for(unsigned i=0;i<inventoryretention::DropBytes;++i)
   Check(*(const unsigned char*)(NativeOwnedInventory::script+inventoryretention::DropAt+i)==0x0b);
 }
 if(raiseFault)RaiseException(0xE0000011,0,0,nullptr);
}
static bool CatchFault(void* self,void* stack) {
 __try {
  NativeOwnedInventory::InvokeRetained(self,nullptr,stack,nullptr,42,8,Original);
 } __except(GetExceptionCode()==0xE0000011?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
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
 Check(NativeOwnedInventory::ScriptIdentity((uintptr_t)code.data()));
 Check(NativeOwnedInventory::InvokeRetained((void*)manager,nullptr,stack,nullptr,42,8,Original));
 Check(calls==1&&code==original&&!NativeOwnedInventory::patchActive);
 Check(!NativeOwnedInventory::InvokeRetained((void*)pawn,nullptr,stack,nullptr,42,8,Original));
 Check(!NativeOwnedInventory::InvokeRetained((void*)manager,nullptr,stack,nullptr,43,8,Original));
 NativeOwnedInventory::thread^=1;
 Check(!NativeOwnedInventory::InvokeRetained((void*)manager,nullptr,stack,nullptr,42,8,Original));
 NativeOwnedInventory::thread=GetCurrentThreadId();
 g_keepAcquiredWeapons=0;
 Check(!NativeOwnedInventory::InvokeRetained((void*)manager,nullptr,stack,nullptr,42,8,Original));
 g_keepAcquiredWeapons=1;
 code[33]^=1;
 Check(!NativeOwnedInventory::InvokeRetained((void*)manager,nullptr,stack,nullptr,42,8,Original));
 code=original;raiseFault=true;
 Check(CatchFault((void*)manager,stack));
 Check(calls==2&&code==original&&!NativeOwnedInventory::patchActive);
 raiseFault=false;nested=true;
 Check(NativeOwnedInventory::InvokeRetained((void*)manager,nullptr,stack,nullptr,42,8,Original));
 Check(calls==3&&code==original&&!NativeOwnedInventory::patchActive&&!NativeOwnedInventory::patchSuspended);
 nestedFault=true;
 Check(CatchFault((void*)manager,stack));
 Check(calls==4&&code==original&&!NativeOwnedInventory::patchActive&&!NativeOwnedInventory::patchSuspended);
 nestedFault=false;
 nested=false;
 // Actual snapshot implementation, with reflected fake fields: saved guns
 // appear before any mesh observation; no inventory/ammo write is permitted.
 g_keepAcquiredWeapons=0;g_upNext=0x40;
 controller=(uintptr_t)objects[45].data();
 const uintptr_t cc=(uintptr_t)objects[46].data(),pc=(uintptr_t)objects[47].data();
 const uintptr_t mc=(uintptr_t)objects[48].data(),wc=(uintptr_t)objects[53].data();
 objectNames[pc]="RvPlayerPawnSP";objectNames[mc]="RvInventoryManager";objectNames[wc]="RvWeaponShared";
 memcpy((void*)(controller+OBJ_CLASS),&cc,4);memcpy((void*)(controller+0x100),&pawn,4);
 memcpy((void*)(pawn+OBJ_CLASS),&pc,4);memcpy((void*)(manager+OBJ_CLASS),&mc,4);
 offsets={{"Pawn",0x100},{"InvManager",0x1e4},{"InventoryChain",0x1e0},
          {"Inventory",0x1e0},{"mWeaponName",0x2e8}};
 // Pawn.InvManager is a different reflected property than Inventory.InvManager.
 // The mock dispatch below accounts for that class-specific offset.
 const uintptr_t first=(uintptr_t)objects[49].data();memcpy((void*)(manager+0x1e0),&first,4);
 const char* gunNames[]={"ARPistol","ARMachineGun","ARShotgun","ARSniperRifle"};
 for(unsigned i=0;i<4;++i){
  const uintptr_t it=(uintptr_t)objects[49+i].data();objectNames[it]=gunNames[i];
  memcpy((void*)(it+OBJ_CLASS),&wc,4);memcpy((void*)(it+0x1e4),&manager,4);
  const uintptr_t link=i<2?(uintptr_t)objects[50+i].data():0;
  memcpy((void*)(it+0x1e0),&link,4);
  NativeOwnedInventory::Name n{(int)(200+i),0};memcpy((void*)(it+0x2e8),&n,8);
 }
 const auto before=objects;
 Check(NativeOwnedInventory::Snapshot());
 Check(NativeOwnedInventory::Mask()==7&&NativeOwnedInventory::count==3&&objects==before);
 const uintptr_t third=(uintptr_t)objects[51].data(),fourth=(uintptr_t)objects[52].data();
 memcpy((void*)(third+0x1e0),&fourth,4);
 Check(NativeOwnedInventory::Snapshot());Check(NativeOwnedInventory::Mask()==15);
 memcpy((void*)(fourth+0x1e0),&first,4); // cycle must fail closed
 NativeOwnedInventory::next=0;NativeOwnedInventory::Poll();Check(NativeOwnedInventory::Mask()==0);
 memset((void*)(fourth+0x1e0),0,4);memset((void*)(third+0x1e4),0,4);
 NativeOwnedInventory::next=0;NativeOwnedInventory::Poll();Check(NativeOwnedInventory::Mask()==0);
 controller=0;NativeOwnedInventory::next=0;NativeOwnedInventory::Poll();Check(NativeOwnedInventory::Mask()==0);
 std::printf("PASS %u actual-handler scope/SEH checks; fake engine only\n",checks);
}
