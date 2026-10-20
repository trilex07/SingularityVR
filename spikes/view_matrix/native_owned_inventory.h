#pragma once
#include "inventory_retention_policy.h"
// Only the existing engine input/script threads touch engine objects. The XR
// and renderer sides exchange a mask/selection request, never UObject pointers.
//
// ---- 2026-10-19 (L3 feedback: "I cannot switch weapons, the wheel is empty") ----
// The first owned-inventory implementation read the possession chain through
// `FindController()` + `PauseRttProbe::Property(cls,"Pawn","ObjectProperty")`.
// At runtime that path produced `stage=local SP pawn class mask=0 count=0`:
// the controller lookup or the typed property lookup failed, the snapshot
// refused, the ownership mask stayed 0 and the wheel rendered NO icons - a
// visible REGRESSION against the accepted baseline, which at least showed the
// mesh-observed guns.
//
// Three changes, all evidence driven:
//  1. The pawn comes from the engine's own cached player pawn (the same object
//     the ammo/reload observers already use and the log already proves:
//     `player pawn: 0x1BD76400 (RvPlayerPawnSP)`), with the controller route
//     kept only as a fallback.
//  2. The inventory chain is read through the AUDITED native layout
//     (pawn+0x3D0 -> manager+0x1E0 -> item+0x1E0, weapon name at item+0x2E8)
//     and every object is validated by its CLASS NAME, so a wrong offset
//     refuses instead of walking garbage. Reflection is used only as a
//     cross-check, never as the only source of an address.
//  3. The wheel keeps the accepted mesh-observed naming as a fallback, so an
//     unresolved ownership snapshot can never empty the wheel again.
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
static volatile LONG owned=0,request=0,ready=0;
static uintptr_t P(uintptr_t p){return PauseRttProbe::Pointer(p);}
static bool Named(uintptr_t p,const char* expected){char n[96]{};return p&&NameOf(p,n,sizeof(n))&&!strcmp(n,expected);}
// Class-chain containment. The audited classes are RvPlayerPawnSP,
// RvInventoryManager and RvWeaponShared (+ its _Shotgun/_SniperRifle
// derivatives), so a CONTAINS test is the correct shape; the exact-name test
// used before could not match a derived weapon class at all.
static bool ClassHas(uintptr_t cls,const char* needle) {
  if(!g_upNext||!cls)return false;
  char n[96]{};
  for(unsigned d=0;cls&&d<32;++d,cls=P(cls+g_upNext-4)) {
   if(!NameOf(cls,n,sizeof(n)))continue;
   if(strstr(n,needle))return true;
  }
  return false;
}
// Find a reflected member by NAME in the class chain (a UField pointer).
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
  if(reports++<16)Log("owned inventory: SwitchWeaponByArchetype script/ref identities verified (scoped drop guard available, not runtime acceptance)");
}
// The pawn's inventory manager is at the audited native offset. The reflected
// name lookup is a CROSS-CHECK only: it is reported, never trusted as the
// only source, because the typed property lookup failed at runtime.
static const uintptr_t kPawnInvManager=0x3d0;
static const uintptr_t kManagerChain=0x1e0;
static const uintptr_t kItemNext=0x1e0;
static const uintptr_t kItemWeaponName=0x2e8;
static int reflectedInvManager=-1;
static int reflectedChain=-1;
static bool Snapshot() {
  // ---- 1. the local pawn ----
  stage="pawn";
  uintptr_t p=(uintptr_t)InterlockedCompareExchange((volatile LONG*)&g_playerPawn,0,0);
  uintptr_t pc=p?P(p+OBJ_CLASS):0;
  if(!p||!pc) {
   // Fallback: the reflected controller route. Kept because the pawn cache is
   // cleared on a level change and the snapshot must recover on its own.
   stage="controller/pawn property";
   const uintptr_t ctl=FindController(),cc=ctl?P(ctl+OBJ_CLASS):0;
   const int po=cc?PropOffsetInChain(cc,"Pawn"):-1;
   if(po>=0){p=P(ctl+po);pc=p?P(p+OBJ_CLASS):0;}
  }
  if(!p||!pc){stage="no local pawn";return false;}
  stage="local SP pawn class";
  if(!ClassHas(pc,"PlayerPawn")){stage="pawn class mismatch";return false;}
  // ---- 2. the inventory manager ----
  stage="inventory manager";
  const uintptr_t m=P(p+kPawnInvManager);
  if(!m){stage="no inventory manager at pawn+0x3D0";return false;}
  const uintptr_t mc=P(m+OBJ_CLASS);
  if(!ClassHas(mc,"InventoryManager")){stage="inventory manager class mismatch";return false;}
  reflectedInvManager=-1;
  reflectedChain=-1;
  static uintptr_t lastPawnClass=0,lastManagerClass=0;
  if(pc!=lastPawnClass){lastPawnClass=pc;reflectedInvManager=PropOffsetInChain(pc,"InvManager");}
  if(mc!=lastManagerClass){lastManagerClass=mc;reflectedChain=PropOffsetInChain(mc,"InventoryChain");}  // ---- 3. the possession chain ----
  stage="inventory chain";
  uintptr_t seen[128]{};unsigned visits=0;Item found[4]{};unsigned foundCount=0,mask=0;
  for(uintptr_t it=P(m+kManagerChain);it;){
   stage="inventory chain bounds/cycle";
   if(visits>=128)return false;
   for(unsigned k=0;k<visits;++k)if(seen[k]==it)return false;
   seen[visits++]=it;
   const uintptr_t ic=P(it+OBJ_CLASS);
   if(ClassHas(ic,"RvWeaponShared")) {
    stage="weapon name layout";
    if(!PauseRttProbe::SafeRange(it+kItemWeaponName,8))return false;
    // The weapon identity is the audited mWeaponName FName; the item's own
    // object name is the fallback when mWeaponName carries no text. Both
    // raw values are reported once so a wrong alias table is visible in the
    // log instead of silently producing an empty wheel.
    char own[96]{};NameOf(it,own,sizeof(own));
    const Name weapon=*(const Name*)(it+kItemWeaponName);
    char wn[96]{};NameFromIndex(weapon.index,wn,sizeof(wn));
    int mesh=inventoryretention::WeaponMesh(wn);
    if(!mesh)mesh=inventoryretention::WeaponMesh(own);
    if(!mesh&&reports<24)Log("owned inventory: unmapped weapon item name=%s mWeaponName=%s",own,wn);
    const unsigned bit=inventoryretention::WeaponBit(mesh);
    if(bit&&!(mask&bit)&&foundCount<4){
     // The selector needs the FName the ENGINE uses for this weapon.
     Name ident=weapon;
     if(!wn[0])ident=*(const Name*)(it+OBJ_NAME);
     found[foundCount++]={mesh,ident};
     mask|=bit;
    }
   }
   const uintptr_t nxt=P(it+kItemNext);
   if(!nxt)break;
   it=nxt;
  }
  pawn=p;manager=m;count=foundCount;
  memcpy(items,found,sizeof(items));
  InterlockedExchange(&owned,(LONG)mask);
  InterlockedExchange(&ready,(LONG)(foundCount?1:0));
  if(Rd(g_keepAcquiredWeapons)&&(!function||!script)&&GetTickCount64()>=resolveNext) {
   resolveNext=GetTickCount64()+2000;ResolveScript(mc);
  }
  stage=foundCount?"ready":"ownership empty";
  return true;
}
static void Report() {
  static const char* previous=nullptr;static LONG previousMask=-1;
  const LONG mask=Rd(owned);
  if((previous!=stage||previousMask!=mask)&&reports<32) {
   ++reports;previous=stage;previousMask=mask;
   Log("owned inventory: stage=%s mask=%X count=%u pawn=%p manager=%p invMgrOff=%d chainOff=%d (identity only)",
       stage,(unsigned)mask,count,(void*)pawn,(void*)manager,reflectedInvManager,reflectedChain);
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
   if(!Snapshot()) {
    count=0;pawn=manager=0;InterlockedExchange(&owned,0);InterlockedExchange(&ready,0);Report();return;
   }
   Report();
   if(!selected||!pawn||!manager||Rd(g_gamePaused)||Rd(g_inCinematic)||Rd(g_vrMenuClass)!=0)return;
   for(unsigned i=0;i<count;++i)if(items[i].mesh==selected) {
    // The audited selector: RvPlayerPawnSP vtable+0xB14 takes the weapon FName.
    // The identity check is the vtable ITSELF: slot +0xB18 of the same vtable
    // must point at the independently audited HasWeapon implementation
    // (0x102F700, verified against the owned EXE vtable at 0x19D0788). That
    // pins the vtable far more strongly than a body hash that cannot be
    // recomputed from the installed (packed) image.
    const uintptr_t vt=P(pawn);
    const uintptr_t cross=P(vt+0xb18);
    if(cross!=inventoryretention::HasWeaponImpl){if(reports++<24)Log("owned inventory: pawn vtable identity refused (cross=%p expected=%p)",(void*)cross,(void*)inventoryretention::HasWeaponImpl);return;}
    const uintptr_t target=P(vt+0xb14);
    if(!target)return;
    using Select=void (__thiscall*)(void*,Name);
    ((Select)target)((void*)pawn,items[i].name);
    if(reports++<24)Log("owned inventory: selected existing weapon mesh=%ld via audited pawn selector",selected);
    return;
   }
  } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
   count=0;pawn=manager=function=script=0;functionName=-1;InterlockedExchange(&owned,0);InterlockedExchange(&ready,0);
  }
}
static unsigned Mask(){return (unsigned)Rd(owned);}
// True only when a snapshot actually produced a possession list. The wheel
// uses this to decide between the owned mask and the accepted mesh-observed
// fallback, so a refused snapshot can never empty the wheel again.
static bool Ready(){return Rd(ready)!=0;}
// The possessed mesh ids, for the wheel's label ownership test.
static unsigned OwnedVerts(long* out,int max){
  unsigned n=0;
  if(!out||max<=0)return 0;
  for(unsigned i=0;i<count&&n<(unsigned)max;++i)out[n++]=items[i].mesh;
  return n;
}
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
  (void)edx;(void)result;(void)original;
  if(!Rd(g_keepAcquiredWeapons)||name!=functionName||!script||!function||
     (uintptr_t)self!=manager||!manager||thread!=GetCurrentThreadId()||nodeOffset<0)return false;
  if(!CanRetain(stack,nodeOffset))return false;
  // ---- 2026-10-19: the bytecode write is WITHDRAWN ----
  // The offline token analysis (tools/reload/decode_switch_weapon_script.py and
  // solve_script_tokens.py, against the original package) shows the
  // ThrowActiveWeapon call is a NINE byte record: opcode 0x1B at offset 200
  // followed by the 8 byte FName at 201..208 - the 11 byte range this code
  // used to overwrite also destroyed the NEXT record (0x4A,0x16 at 209/210).
  // The record layout of the surrounding stream is not yet established, so a
  // length preserving neutralisation cannot be proven safe, and writing into
  // the shared script is exactly the failure mode that crashed the game with
  // the rejected NPC package. The scope checks stay, the write does not.
  if(reports++<24)Log("owned inventory: local pickup reached SwitchWeaponByArchetype (scoped, no bytecode write)");
  return false;
}
}
