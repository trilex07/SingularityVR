#pragma once
#include "pause_rtt_probe.h"
#include "pause_rtt_start.h"
#include "native_gamepad_enum_probe.h"
#include "ui_asset_witness.h"
// Singularity 2010 PE32: evidence in menu-*-disasm.txt and menu-live-input.json.
// This bridge runs on the original platform input thread, never the render thread.
bool PauseVrSettingsHitNormalized(LONG u, LONG v);
void OpenVrSettingsFromPause();
extern volatile LONG g_panelOpen;
void* PatchVTable(void* obj,int index,void* repl);
extern volatile LONG g_vrMenuReleaseRequested; // render thread consumes COM invalidation
extern volatile LONG g_modalSaveDialog; // 1 while RvUIGFxMovie_Dialog owns focus
extern volatile LONG g_modalSlotsActive,g_modalSlotsCapturePending,g_vrModalDialogSurface,g_vrModalDirectDialog;
extern volatile LONG g_menuAssetWitness;
extern volatile LONG g_vrUiDirectCanvas,g_vrUiNoteCanvas;

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
static uintptr_t ExactDialogSurface(uintptr_t movie,uintptr_t cls) {
 if(!movie||!cls)return 0;
 __try {
  const int targetOff=PauseRttProbe::Property(cls,"RenderTexture","ObjectProperty");
  const uintptr_t target=targetOff>=0?ReadPointer(movie+(uintptr_t)targetOff):0;
  const uintptr_t resource=target?ReadPointer(target+0xA4u):0;
  if(!target||!resource)return 0;
  return (uintptr_t)PauseRttProbe::NativeSurface(target,resource);
 } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
  return 0;
 }
}
static bool ExactDirectDialogPath(uintptr_t movie,uintptr_t cls) {
 if(!movie||!cls)return false;
 __try {
  const int targetOff=PauseRttProbe::Property(cls,"RenderTexture","ObjectProperty");
  const int modeOff=PauseRttProbe::Property(cls,"RenderTextureMode","ByteProperty");
  if(targetOff<0||modeOff<0||!Readable((void*)(movie+(uintptr_t)targetOff),4)||
     !Readable((void*)(movie+(uintptr_t)modeOff),1))return false;
  const uintptr_t target=ReadPointer(movie+(uintptr_t)targetOff);
  const unsigned char mode=*reinterpret_cast<const unsigned char*>(movie+(uintptr_t)modeOff);
  return target==0&&mode==0;
 } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
  return false;
 }
}
static void ObserveMovieAsset(uintptr_t movie,uintptr_t cls,const char* focusClass) {
 if(!Rd(g_menuAssetWitness))return;
 static unsigned remaining=8;
 if(!remaining)return;--remaining;
 __try {
  const int infoOff=PauseRttProbe::Property(cls,"MovieInfo","ObjectProperty");
  const uintptr_t info=infoOff>=0?ReadPointer(movie+infoOff):0;
  const uintptr_t infoClass=ReadPointer(info?info+0x34u:0);
  char infoName[96]{};
  const int rawOff=infoClass?PauseRttProbe::Property(infoClass,"RawData","ArrayProperty"):-1;
  if(!info || !NameOf(info,infoName,sizeof(infoName)) || rawOff<0 ||
     !Readable((void*)(info+rawOff),12)) {
   Log("ui-asset witness: focus=%s MovieInfo=%p RawData UNKNOWN (no writes)",focusClass,(void*)info);return;
  }
  const uintptr_t array=info+rawOff;
  const uintptr_t bytes=ReadPointer(array);
  const int count=*reinterpret_cast<const int*>(array+4);
  const int capacity=*reinterpret_cast<const int*>(array+8);
  if(!bytes || !uiwitness::ArrayBounds(count,capacity) || !Readable((void*)bytes,count)) {
   Log("ui-asset witness: focus=%s info=%s rawOff=%d count=%d capacity=%d UNKNOWN (no writes)",focusClass,infoName,rawOff,count,capacity);return;
  }
  const auto* data=reinterpret_cast<const unsigned char*>(bytes);
  Log("ui-asset witness: focus=%s info=%s rawOff=%d bytes=%d fnv64=%016llX magic=%02X%02X%02X%02X (source identity, not pixel proof; no writes)",
      focusClass,infoName,rawOff,count,uiwitness::Fingerprint(data,count),data[0],data[1],data[2],data[3]);
 } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
  Log("ui-asset witness: guarded read failed UNKNOWN; no writes");
 }
}
static void ObserveWeaponAssets() {
 if(!Rd(g_menuAssetWitness))return;
 static uintptr_t last=0;static unsigned remaining=6;
 const uintptr_t weapon=(uintptr_t)Rd(g_weaponObj);
 if(!remaining || !weapon || weapon==last)return;
 last=weapon;--remaining;
 __try {
  const uintptr_t cls=ReadPointer(weapon+0x34u);char name[96]{};
  if(!cls || !NameOf(weapon,name,sizeof(name)))return;
  const int fxOff=PauseRttProbe::Property(cls,"mWeaponFX","ObjectProperty");
  const uintptr_t fx=fxOff>=0?ReadPointer(weapon+fxOff):0;
  const uintptr_t fxClass=ReadPointer(fx?fx+0x34u:0);
  Log("native-fx witness: weapon=%s object=%p mWeaponFX=%p off=%d (read-only, no playback/render proof)",name,(void*)weapon,(void*)fx,fxOff);
  if(!fxClass)return;
  const char* fields[]={"mMuzzleFlash1stPerson","mMuzzleFlash3rdPerson","mMuzzleFlashIronsights","mLoopingMuzzleFXEmitter"};
  for(const char* field:fields){
   const int off=PauseRttProbe::Property(fxClass,field,"ObjectProperty");
   const uintptr_t asset=off>=0?ReadPointer(fx+off):0;
   char assetName[96]{},assetClass[96]{};
   const uintptr_t type=ReadPointer(asset?asset+0x34u:0);
   if(asset)NameOf(asset,assetName,sizeof(assetName));if(type)NameOf(type,assetClass,sizeof(assetClass));
   Log("native-fx witness: field=%s off=%d value=%p name=%s class=%s (no mutations)",field,off,(void*)asset,assetName,assetClass);
  }
 } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
  Log("native-fx witness: guarded read failed UNKNOWN; no writes");
 }
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
  const bool directCanvas=kind==4&&ExactDirectDialogPath(movie,cls);
  InterlockedExchange(&g_vrUiDirectCanvas,directCanvas?1:0);
  InterlockedExchange(&g_vrUiNoteCanvas,directCanvas&&!strcmp(className,"RvUIGFxMovie_Notepad")?1:0);
  if(directCanvas)Log("context-pointer: focus=%s reflected null RenderTexture / mode 0",className);
  static unsigned generation=0;
  InterlockedExchange64(&g_vrMenuStateToken,((LONG64)++generation<<32)|(unsigned)kind);
  // Do not carry a cursor coordinate or pressed edge into another GFx movie.
  // The first ray hit in the new movie republishes the live controller UV.
  PublishVrUiPair(500000,500000);
  InterlockedExchange(&g_vrUiValid,0);
  InterlockedExchange(&g_vrUiDown,0);
  InterlockedExchange(&g_vrUiEverHit,0);
  static char prevClass[96]{};
  const bool dialogFocus=className[0]&&!strcmp(className,"RvUIGFxMovie_Dialog");
  const bool slotsFocus=prevClass[0]&&(!strcmp(prevClass,"RvUIGFxMovie_Slots")||!strcmp(prevClass,"RvUIGFxMovie_SlotsBase"));
  const bool saveDialogTransition=modalmenu::IsSaveDialogTransition(slotsFocus,dialogFocus);
  if(saveDialogTransition) {
   InterlockedExchange(&g_modalSlotsActive,1);
   InterlockedExchange(&g_modalSlotsCapturePending,1);
   InterlockedExchange(&g_vrMenuReleaseRequested, 1);
   Log("menu modal: save-slot -> dialog focus; render-thread slots RTT invalidation requested");
  }
  if(!dialogFocus){
   InterlockedExchange(&g_modalSlotsActive,0);
   InterlockedExchange(&g_modalSlotsCapturePending,0);
   InterlockedExchange(&g_vrModalDialogSurface,0);
   InterlockedExchange(&g_vrModalDirectDialog,0);
  } else {
   const uintptr_t dialogSurface=ExactDialogSurface(movie,cls);
   const bool directDialog=ExactDirectDialogPath(movie,cls);
   InterlockedExchange(&g_vrModalDialogSurface,(LONG)dialogSurface);
   InterlockedExchange(&g_vrModalDirectDialog,directDialog?1:0);
   if(dialogFocus)Log("menu modal: Dialog exact RTT surface=%p verified=%d; capture and parent suppression require this identity",
                      (void*)dialogSurface,dialogSurface?1:0);
   if(directDialog)Log("menu modal: Dialog direct-render proof: reflected RenderTexture is null and RenderTextureMode=0");
  }
  InterlockedExchange(&g_modalSaveDialog,dialogFocus?1:0);
  if (className[0]) strcpy_s(prevClass, className);
  InterlockedExchange(&g_vrMenuClass,kind);
  Log("VR UI focus: movie=%p object=%s class=%s menu=%d",(void*)movie,objectName,className,cachedMenu?1:0);
  if(cachedMenu)ObserveMovieAsset(movie,cls,className);
  // NewGame: actual focus log proves SPSlots -> dialog. This probe establishes
  // whether each has its own RTT, shares a parent's RTT, or draws directly.
  // Four cold snapshots/session; no new hook, matrix write or resource ref.
  static unsigned modalNotes=0;
  if (Rd(g_modalUiProbe) && modalNotes<4 && movie &&
      (!strcmp(className,"RvUIGFxMovie_Dialog") || !strcmp(className,"RvUIGFxMovie_Slots"))) {
   ++modalNotes;
   __try {
    const int targetOff=PauseRttProbe::Property(cls,"RenderTexture","ObjectProperty");
    const int infoOff=PauseRttProbe::Property(cls,"MovieInfo","ObjectProperty");
    const int modeOff=PauseRttProbe::Property(cls,"RenderTextureMode","ByteProperty");
    const uintptr_t target=targetOff>=0?ReadPointer(movie+targetOff):0;
    const uintptr_t resource=target?ReadPointer(target+0xA4):0;
    const uintptr_t surface=target&&resource?(uintptr_t)PauseRttProbe::NativeSurface(target,resource):0;
    const bool contract=surface!=0;
    Log("modal-ui identity n=%u token=%llu focus=%p class=%s MovieInfo=%p RenderTexture=%p resource=%p contract=%d surface=%p modeOffset=%d mode=%d; no render changes",
        modalNotes,VrMenuStateToken(),(void*)movie,className,(void*)(infoOff>=0?ReadPointer(movie+infoOff):0),
        (void*)target,(void*)resource,contract?1:0,(void*)surface,modeOff,
        modeOff>=0 && Readable((void*)(movie+modeOff),1)?(int)*reinterpret_cast<const unsigned char*>(movie+modeOff):-1);
   } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
    Log("modal-ui UNKNOWN: guarded snapshot failed; no render changes");
   }
  }
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
    || !Readable(self,0x64))return false;
 // FViewport SizeX/SizeY (live 5264/2684); no desktop/window coordinate assumption.
 const int w=*reinterpret_cast<int*>((uintptr_t)self+0x4Cu);
 const int h=*reinterpret_cast<int*>((uintptr_t)self+0x50u);
 if(w<64 || h<64 || w>32768 || h>32768)return false;
 LONG u=500000,v=500000;
 if(!ReadVrUiPair(u,v)){u=500000;v=500000;}
 if(u<0 || u>1000000 || v<0 || v>1000000){u=500000;v=500000;}
 // Native getter 0x011CFC40 returns ScreenToClient POINT.y unchanged (top-left).
 // MouseY delta inversion in Drain is an axis convention, not an absolute-coordinate flip.
 // When the controller ray misses the panel, pin the native mouse at the last VR
 // coordinates instead of falling through to the desktop getter (which exposes
 // the game's arrow cursor at an unrelated location). UpdateVrUiPointer clears
 // the click edge on a miss, so this cannot activate an item.
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
  if(Rd(g_menuAssetWitness)) {
   static unsigned sampleCount=0;static DWORD sampleTime=0;
   const DWORD now=GetTickCount();
   if(sampleCount<24 && (sampleCount==0 || (DWORD)(now-sampleTime)>=500)) {
    ++sampleCount;sampleTime=now;
    Log("ui-coordinate witness: sample=%u token=%llu nativeXY=%d,%d viewport=%d,%d rayUV=%ld,%ld updates=%ld (dispatch timing; no cursor mutation)",
        sampleCount,VrMenuStateToken(),x,y,*reinterpret_cast<const int*>((uintptr_t)viewport+0x4Cu),
        *reinterpret_cast<const int*>((uintptr_t)viewport+0x50u),Rd(g_vrUiU),Rd(g_vrUiV),Rd(g_vrCounters.vrPointerUpdates));
   }
  }
  const float dx=lastX<0?0.f:(float)(x-lastX),dy=lastY<0?0.f:(float)(lastY-y);
  ((AxisFn)vt[6])((void*)client,viewport,0,mouseX,dx,dt,0);
  ((AxisFn)vt[6])((void*)client,viewport,0,mouseY,dy,dt,0);
  lastX=x;lastY=y;
 }else lastX=lastY=-1;
 const bool down=valid&&InterlockedCompareExchange(&g_vrUiDown,0,0)!=0;
 LONG pointerU=500000,pointerV=500000;ReadVrUiPair(pointerU,pointerV);
 const bool pauseSettingsRow=valid && PauseVrSettingsHitNormalized(pointerU,pointerV);
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
 ObserveWeaponAssets(); // at most six weapon identities, only the explicit read-only witness
 NativeGamepadEnumProbe::SetContext(VrModeOn() && InterlockedCompareExchange(&g_padEmu,0,0)!=0 &&
     InterlockedCompareExchange(&g_vrMenuClass,0,0)==0);
 BindViewport();
 {
  hotpath::scope inputScope(g_hotPathSampler,hotpath::Family::native_input_poll);
  originalPoll(self,dt); // original guarded missing-mouse path preserves keyboard/XInput/OpenXR
 }
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
 if(a==MH_OK && b==MH_OK)NativeGamepadEnumProbe::Install();
}
} // namespace NativeVrUi


