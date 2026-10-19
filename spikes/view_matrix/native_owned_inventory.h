#pragma once
#include "inventory_retention_policy.h"
// Only the existing engine input/script threads touch engine objects. The XR
// and renderer sides exchange a mask/selection request, never UObject pointers.
namespace NativeOwnedInventory {
struct Name {int index,number;};
struct Item {int mesh;Name name;};
static Item items[4]{};
static unsigned count=0;
static uintptr_t pawn=0,manager=0,function=0,script=0;
static int functionName=-1;
static DWORD thread=0;
static ULONGLONG next=0;
static unsigned reports=0;
static const char* stage="not sampled";
static ULONGLONG resolveNext=0;
static volatile LONG owned=0,request=0;
static bool patchActive=false;
static uintptr_t P(uintptr_t p){return PauseRttProbe::Pointer(p);}
static bool Named(uintptr_t p,const char* expected){char n[96]{};return p&&NameOf(p,n,sizeof(n))&&!strcmp(n,expected);}
static bool Derives(uintptr_t cls,const char* name) {
 if(!g_upNext)return false;
 for(unsigned d=0;cls&&d<32;++d,cls=P(cls+g_upNext-4))if(Named(cls,name))return true;
 return false;
}
static uintptr_t Field(uintptr_t cls,const char* name) {
 if(!g_upChildren||!g_upNext)return 0;
 for(unsigned d=0;cls&&d<32;++d,cls=P(cls+g_upNext-4)) {
  uintptr_t f=P(cls+g_upChildren);
  for(unsigned i=0;f&&i<400;++i,f=P(f+g_upNext))if(Named(f,name))return f;
 }
 return 0;
}
static bool ScriptIdentity(uintptr_t code) {
 if(!PauseRttProbe::SafeRange(code,inventoryretention::ScriptBytes)||
    !inventoryretention::OriginalShape((const unsigned char*)code,inventoryretention::ScriptBytes))return false;
 for(const auto& r:inventoryretention::Objects)if(!Named(P(code+r.at),r.name))return false;
 for(const auto& r:inventoryretention::Names) {
  char n[96]{};
  if(!NameFromIndex(*(const int*)(code+r.at),n,sizeof(n))||strcmp(n,r.name)||
     *(const int*)(code+r.at+4)!=0)return false;
 }
 MEMORY_BASIC_INFORMATION mem{};
 if(!VirtualQuery((void*)code,&mem,sizeof(mem)))return false;
 const DWORD prot=mem.Protect&0xff;
 return prot==PAGE_READWRITE||prot==PAGE_WRITECOPY;
}
static void ResolveScript(uintptr_t cls) {
 function=script=0;functionName=-1;
 const uintptr_t f=Field(cls,"SwitchWeaponByArchetype");
 if(!f||!Named(P(f+OBJ_CLASS),"Function")||!Named(P(f+OBJ_OUTER),"RvInventoryManager"))return;
 unsigned matches=0;uintptr_t candidate=0;
 // Discover the unique script TArray by content; no assumed UStruct layout.
 for(unsigned off=0x40;off<=0x90;off+=4) {
  if(!PauseRttProbe::SafeRange(f+off,12))continue;
  const int n=*(const int*)(f+off+4),cap=*(const int*)(f+off+8);
  const uintptr_t code=P(f+off);
  if(n==284&&cap>=n&&cap<=4096&&ScriptIdentity(code)){++matches;candidate=code;}
 }
 if(matches!=1)return;
 function=f;script=candidate;functionName=*(const int*)(f+OBJ_NAME);
 if(reports++<16)Log("owned inventory: SwitchWeaponByArchetype script/ref identities verified, scoped drop guard ready (not runtime acceptance)");
}
static bool Snapshot() {
 stage="controller/pawn property";
 const uintptr_t ctl=FindController(),cc=ctl?P(ctl+OBJ_CLASS):0;
 const int po=cc?PauseRttProbe::Property(cc,"Pawn","ObjectProperty"):-1;
 const uintptr_t p=po>=0?P(ctl+po):0,pc=p?P(p+OBJ_CLASS):0;
 stage="local SP pawn class";
 if(!Derives(pc,"RvPlayerPawnSP"))return false;
 const int mo=PauseRttProbe::Property(pc,"InvManager","ObjectProperty");
 stage="pawn inventory layout";
 if(mo!=0x3d0)return false; // independently audited native HasWeapon and selector
 const uintptr_t m=P(p+mo),mc=m?P(m+OBJ_CLASS):0;
 stage="inventory manager class";
 if(!Derives(mc,"RvInventoryManager"))return false;
 const int head=PauseRttProbe::Property(mc,"InventoryChain","ObjectProperty");
 stage="inventory chain layout";
 if(head!=0x1e0)return false;
 uintptr_t seen[128]{};unsigned visits=0;Item found[4]{};unsigned foundCount=0,mask=0;
 for(uintptr_t it=P(m+head);it;){
  stage="inventory chain bounds/cycle";
  if(visits>=128)return false;
  for(unsigned k=0;k<visits;++k)if(seen[k]==it)return false;
  seen[visits++]=it;
  const uintptr_t ic=P(it+OBJ_CLASS);
  const int link=PauseRttProbe::Property(ic,"Inventory","ObjectProperty");
  const int im=PauseRttProbe::Property(ic,"InvManager","ObjectProperty");
  stage="inventory item link/owner";
  if(link!=0x1e0||im<0||P(it+im)!=m)return false;
  if(Derives(ic,"RvWeaponShared")) {
   const int nameOff=PauseRttProbe::Property(ic,"mWeaponName","NameProperty");
   stage="weapon name layout";
   if(nameOff!=0x2e8||!PauseRttProbe::SafeRange(it+nameOff,8))return false;
   char n[96]{};NameOf(it,n,sizeof(n));
   const int mesh=inventoryretention::WeaponMesh(n);
   const unsigned bit=inventoryretention::WeaponBit(mesh);
   if(bit&&!(mask&bit)&&foundCount<4){found[foundCount++]={mesh,*(const Name*)(it+nameOff)};mask|=bit;}
  }
  it=P(it+link);
 }
 pawn=p;manager=m;count=foundCount;
 memcpy(items,found,sizeof(items));InterlockedExchange(&owned,(LONG)mask);
 if(Rd(g_keepAcquiredWeapons)&&(!function||!script)&&GetTickCount64()>=resolveNext) {
  resolveNext=GetTickCount64()+2000;ResolveScript(mc);
 }
 stage=Rd(g_keepAcquiredWeapons)&&!script?"ownership ready; retention script refused":"ready";
 return true;
}
static void Report() {
 static const char* previous=nullptr;static LONG previousMask=-1;
 const LONG mask=Rd(owned);
 if((previous!=stage||previousMask!=mask)&&reports<32) {
  ++reports;previous=stage;previousMask=mask;
  Log("owned inventory: stage=%s mask=%X count=%u pawn=%p manager=%p (identity only)",
      stage,(unsigned)mask,count,(void*)pawn,(void*)manager);
 }
}
static void Poll() {
 if(!Rd(g_keepAcquiredWeapons)&&!Rd(g_ownedWeaponWheel))return;
 thread=GetCurrentThreadId();
 const ULONGLONG now=GetTickCount64();
 const LONG selected=InterlockedExchange(&request,0);
 if(!selected&&now<next)return;
 next=now+500;
 __try {
  if(!Snapshot()) {count=0;pawn=manager=0;InterlockedExchange(&owned,0);Report();return;}
  Report();
  if(!selected||!pawn||!manager||Rd(g_gamePaused)||Rd(g_inCinematic)||Rd(g_vrMenuClass)!=0)return;
  for(unsigned i=0;i<count;++i)if(items[i].mesh==selected) {
   const uintptr_t target=P(P(pawn)+0xb14);
   if(!NativeContextCursor::Method(target,168,0xCB797F59504492C3ull))return;
   using Select=void (__thiscall*)(void*,Name);
   ((Select)target)((void*)pawn,items[i].name);
   if(reports++<24)Log("owned inventory: selected existing weapon mesh=%ld via audited pawn selector",selected);
   return;
  }
 } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
  count=0;pawn=manager=function=script=0;functionName=-1;InterlockedExchange(&owned,0);
 }
}
static unsigned Mask(){return (unsigned)Rd(owned);}
static void Queue(int mesh){InterlockedExchange(&request,mesh);}
static bool CanRetain(void* stack,int nodeOffset) {
 __try {
  return P((uintptr_t)stack+nodeOffset)==function&&
   Named(P(function+OBJ_CLASS),"Function")&&Named(P(function+OBJ_OUTER),"RvInventoryManager")&&
   pawn&&P(pawn+0x3d0)==manager&&ScriptIdentity(script);
 } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
  return false;
 }
}
template<class Invoke> static bool InvokeRetained(void* self,void* edx,void* stack,void* result,
 int name,int nodeOffset,Invoke original) {
 if(!Rd(g_keepAcquiredWeapons)||name!=functionName||!script||!function||patchActive||
    (uintptr_t)self!=manager||!manager||thread!=GetCurrentThreadId()||nodeOffset<0)return false;
 // A native-to-script entry gives a bounded synchronous scope. Never patch the
 // shared class globally: NPCs and other inventory managers keep their drop.
 if(!CanRetain(stack,nodeOffset))return false;
 unsigned char saved[inventoryretention::DropBytes];
 memcpy(saved,(void*)(script+inventoryretention::DropAt),sizeof(saved));
 patchActive=true;
 __try {
  memset((void*)(script+inventoryretention::DropAt),0x0b,sizeof(saved)); // EX_Nothing, same length
  if(reports++<32)Log("owned inventory: local pickup drop suppressed within original SwitchWeaponByArchetype; no grant/ammo writes");
  original(self,edx,stack,result);
 } __finally {
  memcpy((void*)(script+inventoryretention::DropAt),saved,sizeof(saved));patchActive=false;
 }
 return true;
}
}
