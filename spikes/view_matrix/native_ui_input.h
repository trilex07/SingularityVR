#pragma once
#include "pause_rtt_probe.h"
#include "pause_rtt_start.h"
// Singularity 2010 PE32: evidence in menu-*-disasm.txt and menu-live-input.json.
// This bridge runs on the original platform input thread, never the render thread.
bool PauseVrSettingsHitNormalized(LONG u, LONG v);
void OpenVrSettingsFromPause();
extern volatile LONG g_panelOpen;
void* PatchVTable(void* obj,int index,void* repl);
namespace NativeVrUi {
struct Name { unsigned index, number; };
typedef void (__thiscall *PollFn)(void*,float);
typedef int (__thiscall *CursorFn)(void*);
typedef void (__thiscall *CursorPosFn)(void*,POINT*);
typedef int (__thiscall *KeyFn)(void*,void*,int,Name,int,float,int);
typedef void (__thiscall *MoveFn)(void*,void*,int,int);
typedef int (__thiscall *AxisFn)(void*,void*,int,Name,float,float,int);
static PollFn originalPoll=nullptr;
static CursorFn originalX=nullptr, originalY=nullptr;
static CursorPosFn originalPos=nullptr;
static void* viewport=nullptr;
static uintptr_t cachedMovie=0;
static bool cachedMenu=false, held=false;
static int lastX=-1,lastY=-1;
static LONG installs=0, ticks=0;
static uintptr_t ReadPointer(uintptr_t p) {
 return p && Readable((void*)p,4) ? *reinterpret_cast<uintptr_t*>(p) : 0;
}
static bool MenuClass(const char* n) {
 static const char* names[]={"RvUIGFxMovie_Main","RvUIGFxMovie_IIS","RvUIGFxMovie_Pause",
 "RvUIGFxMovie_Dialog","RvUIGFxMovie_Slots","RvUIGFxMovie_SlotsBase","RvUIGFxMovie_LoadGame",
 "RvUIGFxMovie_Review","RvUIGFxMovie_Credits","RvUIGFxMovie_Notepad","RvUIGFxMovie_Stats",
 "RvUIGFxMovie_UseStation","RvUIGFxMovie_BarisovStation","RvUIGFxMovie_WeaponLocker"};
 if(!strncmp(n,"RvUIGFxMovie_Options",strlen("RvUIGFxMovie_Options")))return true;
 for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);++i)if(!strcmp(n,names[i]))return true;
 return false;
}
static bool UpdateMenu() {
 // Exact native GetFocusMovie body at 0x00D7C2C0; read only, no native call needed.
 const uintptr_t engine=ReadPointer(0x01CF8778u);
 const uintptr_t record=ReadPointer(engine ? engine+0x148u:0);
 const uintptr_t movie=ReadPointer(record ? record+0x50u:0);
 if(movie!=cachedMovie) {
  ReportVrRuntimeCounters();
  cachedMovie=movie;cachedMenu=false;
  char objectName[96]={}, className[96]={};
  const uintptr_t cls=ReadPointer(movie ? movie+0x34u:0);
  if(movie && NameOf(movie,objectName,sizeof(objectName)) && strncmp(objectName,"Default__",9)
     && cls && NameOf(cls,className,sizeof(className)))cachedMenu=MenuClass(className);
  const LONG kind=!cachedMenu?0:!strcmp(className,"RvUIGFxMovie_IIS")?1:!strcmp(className,"RvUIGFxMovie_Main")?2:!strcmp(className,"RvUIGFxMovie_Pause")?3:4;
  static unsigned generation=0;
  InterlockedExchange64(&g_vrMenuStateToken,((LONG64)++generation<<32)|(unsigned)kind);
  InterlockedExchange(&g_vrMenuClass,kind);
  Log("VR UI focus: movie=%p object=%s class=%s menu=%d",(void*)movie,objectName,className,cachedMenu?1:0);
  // Fix-4/menus: a movie outside the table is the prime suspect for the nested-views
  // second menu - its draws miss the menu path and fall through to StereoPair. Name it
  // loudly, once per focus change (never per frame/draw), so a test run identifies it.
  if(movie && !cachedMenu)
      Log("VR UI focus UNCLASSIFIED: object=%s class=%s - not in the menu table; open this"
          " menu in the test run, then check which render path its draws take",objectName,className);
 }
 PauseRttProbe::Observe(movie,cachedMenu &&
     (InterlockedCompareExchange(&g_vrMenuClass,0,0)==2 ||
      InterlockedCompareExchange(&g_vrMenuClass,0,0)==3));
 const bool menu=cachedMenu && InterlockedCompareExchange(&g_vrUiEnabled,0,0)!=0;
 InterlockedExchange(&g_vrUiMenu,menu?1:0);
 if(menu)InterlockedAnd(&g_padBtns,0x0000FCFF); // Immediate select/fire separation before this input tick.
 return menu;
}
static bool Coordinates(void* self,int& x,int& y) {
 if(self!=viewport || !InterlockedCompareExchange(&g_vrUiMenu,0,0)
    || !InterlockedCompareExchange(&g_vrUiValid,0,0) || !Readable(self,0x64))return false;
 // FViewport SizeX/SizeY (live 5264/2684); no desktop/window coordinate assumption.
 const int w=*reinterpret_cast<int*>((uintptr_t)self+0x4Cu);
 const int h=*reinterpret_cast<int*>((uintptr_t)self+0x50u);
 if(w<64 || h<64 || w>32768 || h>32768)return false;
 LONG u=InterlockedCompareExchange(&g_vrUiU,0,0),v=InterlockedCompareExchange(&g_vrUiV,0,0);
 if(u<0 || u>1000000 || v<0 || v>1000000)return false;
 // Native getter 0x011CFC40 returns ScreenToClient POINT.y unchanged (top-left).
 // MouseY delta inversion in Drain is an axis convention, not an absolute-coordinate flip.
 return uipointer::MapCanvas(u/1000000.0f,v/1000000.0f,(unsigned)w,(unsigned)h,g_vrMenuPanel.canvas,x,y);
}
static int __fastcall GetX(void* self,void*){int x,y;return Coordinates(self,x,y)?x:originalX(self);}
static int __fastcall GetY(void* self,void*){int x,y;return Coordinates(self,x,y)?y:originalY(self);}
static void __fastcall GetPos(void* self,void*,POINT* out){int x,y;if(out&&Coordinates(self,x,y)){out->x=x;out->y=y;}else originalPos(self,out);}
static bool BindViewport() {
 if(!Readable((void*)0x01D1FCC4u,8))return false;
 const uintptr_t data=ReadPointer(0x01D1FCC4u);
 const int count=*reinterpret_cast<int*>(0x01D1FCC8u);
 if(count<1||count>8||!data||!Readable((void*)data,4))return false;
 const uintptr_t platform=ReadPointer(data);
 if(!platform||!Readable((void*)platform,0x70))return false;
 void* selected=(void*)(platform+4u);
 if(viewport==selected)return true;
 void** vt=*reinterpret_cast<void***>(selected);
 if(!Readable(vt,19*4))return false;
 // Fail closed for an unknown executable/viewport class. Do not patch unrelated interfaces.
 if(vt[15]!=(void*)0x011CFC10u && vt[15]!=(void*)&GetX)return false;
 if(!originalX) {
  originalX=(CursorFn)vt[15];originalY=(CursorFn)vt[16];originalPos=(CursorPosFn)vt[17];
  if(vt[16]!=(void*)0x011CFC40u||vt[17]!=(void*)0x011CFC70u)return false;
  if(!PatchVTable(selected,15,(void*)&GetX)||!PatchVTable(selected,16,(void*)&GetY)||!PatchVTable(selected,17,(void*)&GetPos)) {
   PatchVTable(selected,15,(void*)originalX);PatchVTable(selected,16,(void*)originalY);PatchVTable(selected,17,(void*)originalPos);
   originalX=originalY=nullptr;originalPos=nullptr;return false;
  }
 }
 viewport=selected;lastX=lastY=-1;
 Log("VR UI native viewport bound: %p cursor getters patched; desktop cursor untouched",viewport);
 InterlockedExchange(&g_vrUiNativeReady,1);
 return true;
}
static void Drain(float dt) {
 if(!BindViewport())return;
 const uintptr_t client=ReadPointer((uintptr_t)viewport+0x1Cu); // platform+0x20
 if(!client||!Readable((void*)client,4))return;
 void** vt=*reinterpret_cast<void***>(client);
 if(!Readable(vt,15*4)||!Readable((void*)0x01CF566Cu,0x30))return;
 const bool menu=InterlockedCompareExchange(&g_vrUiMenu,0,0)!=0;
 if(InterlockedCompareExchange(&g_panelOpen,0,0)) {
  // The VR settings panel is owned by the mod-rendered hit-test path. Do not
  // leak its R2 edge into the paused GFx movie behind it.
  lastX=lastY=-1;
  return;
 }
 int x,y;const bool valid=menu&&Coordinates(viewport,x,y);
 const Name mouseX=*reinterpret_cast<Name*>(0x01CF566Cu),mouseY=*reinterpret_cast<Name*>(0x01CF5674u);
 const Name left=*reinterpret_cast<Name*>(0x01CF568Cu);
 // Original platform code uses these same InputAxis/InputKey interfaces, controller0/gamepad0.
 if(valid) {
  ((MoveFn)vt[14])((void*)client,viewport,x,y); // verified WM_MOUSEMOVE dispatch vtable+0x38
  const float dx=lastX<0?0.f:(float)(x-lastX),dy=lastY<0?0.f:(float)(lastY-y);
  ((AxisFn)vt[6])((void*)client,viewport,0,mouseX,dx,dt,0);
  ((AxisFn)vt[6])((void*)client,viewport,0,mouseY,dy,dt,0);
  lastX=x;lastY=y;
 }else lastX=lastY=-1;
 const bool down=valid&&InterlockedCompareExchange(&g_vrUiDown,0,0)!=0;
 const bool pauseSettingsRow=valid && PauseVrSettingsHitNormalized(
     InterlockedCompareExchange(&g_vrUiU,0,0), InterlockedCompareExchange(&g_vrUiV,0,0));
 if(down!=held) {
  if (pauseSettingsRow) {
   // This row is supplied by the mod, so it has no native GFx widget to receive
   // the click. Consume the edge before the original movie sees it and open the
   // existing settings panel on the game's input thread.
   if (down) {
    OpenVrSettingsFromPause();
    held=false; // no synthetic mouse-up should leak after the panel takes ownership
   }
  } else {
   ((KeyFn)vt[4])((void*)client,viewport,0,left,down?0:1,1.f,0);
   static unsigned logged=0;if(logged++<16)Log("VR UI native click: %s viewport=(%d,%d) normalized=(%ld,%ld)",down?"down":"up",lastX,lastY,InterlockedCompareExchange(&g_vrUiU,0,0),InterlockedCompareExchange(&g_vrUiV,0,0));
   held=down;
  }
 }
 InterlockedIncrement(&ticks);
}
static void __fastcall Poll(void* self,void*,float dt) {
 ReadableForget();
 UpdateMenu(); // update mode before the game tick/XInput routes the trigger
 BindViewport();
 originalPoll(self,dt); // original guarded missing-mouse path preserves keyboard/XInput/OpenXR
 Drain(dt); // engine UI calls stay on the game's original input thread
}
static void Install() {
 if(InterlockedCompareExchange(&installs,1,0)!=0)return;
 if((uintptr_t)GetModuleHandleW(nullptr)!=0x00400000u){Log("VR UI native input hook rejected: unsupported image base");return;}
 const uintptr_t p=0x011D31C0u;
 const unsigned char prologue[]={0x55,0x8b,0xec,0x6a,0xff,0x68,0x8a,0x76,0x70,0x01};
 if(!Readable((void*)p,sizeof(prologue))||memcmp((void*)p,prologue,sizeof(prologue))){Log("VR UI native input hook rejected: exact prologue");return;}
 const MH_STATUS a=MH_CreateHook((void*)p,(void*)&Poll,(void**)&originalPoll);
 const MH_STATUS b=a==MH_OK?MH_EnableHook((void*)p):a;
 Log("VR UI native input hook: create=%d enable=%d",(int)a,(int)b);
}
} // namespace NativeVrUi


