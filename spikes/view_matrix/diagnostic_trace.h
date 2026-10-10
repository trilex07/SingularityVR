// Diagnostic-only frame trace. Fixed storage; the render and XInput paths never write a file.
#pragma once

enum DiagPhase { DP_WAIT, DP_BEGIN, DP_SYNC, DP_ACTIONS, DP_POSES, DP_END,
                 DP_BRIDGE, DP_ACQUIRE, DP_SWAPWAIT, DP_RELEASE, DP_HAPTIC, DP_COUNT };
struct DiagFrame {
    uint32_t id, thread, mode, eye, eyeAge[2], buttons, previous, rising, falling, publishedButtons;
    uint32_t triggerL, triggerR, rumbleL, rumbleR, hapticCalls, hapticApplyCalls, hapticStopCalls, hapticCallUs, hapticAmplitudeMilli;
    uint32_t xiGetCalls, xiSetCalls, xiGetTotalUs, xiSetTotalUs, xiGetMaxUs, xiSetMaxUs;
    uint32_t phaseUs[DP_COUNT], presentUs, frameUs, resetCount, swapchainCount;
    uint8_t phaseCalls[DP_COUNT];
    uint32_t recEyeW, recEyeH, autoW, autoH, backW, backH, sceneW, sceneH;
    uint32_t sharedW, sharedH, bridgeW, bridgeH, swapW, swapH, rectW, rectH, clientW, clientH;
    int64_t qpcStart, qpcEnd;
};
static constexpr uint32_t kDiagRing = 512;
static DiagFrame g_diagRing[kDiagRing]{};
static DiagFrame g_diagCurrent{};
static CRITICAL_SECTION g_diagLock;
static bool g_diagLockReady = false;
static HANDLE g_diagThread = nullptr, g_diagStop = nullptr;
static volatile LONG g_diagEnabled = 0, g_diagMode = 0, g_diagPollOff = 0;
static volatile LONG g_diagXiGetCalls = 0, g_diagXiSetCalls = 0;
static volatile LONG g_diagXiGetTotal = 0, g_diagXiSetTotal = 0;
static volatile LONG g_diagXiGetMax = 0, g_diagXiSetMax = 0;
static volatile LONG g_diagResetCount = 0, g_diagSwapchainCount = 0;
static volatile LONG g_diagResetRequestW = 0, g_diagResetRequestH = 0;
static volatile LONG g_diagSceneW = 0, g_diagSceneH = 0;
static uint32_t g_diagNextId = 0, g_diagPublishedId = 0, g_diagLastEyeFrame[2] = {};
static int64_t g_diagPriorStart = 0;
static LARGE_INTEGER g_diagFrequency{};

// Log is defined by the host source before this header is included.
static uint32_t DiagUs(int64_t start) {
    LARGE_INTEGER now{}; QueryPerformanceCounter(&now);
    if (!g_diagFrequency.QuadPart) QueryPerformanceFrequency(&g_diagFrequency);
    return start && g_diagFrequency.QuadPart ?
        uint32_t((now.QuadPart - start) * 1000000LL / g_diagFrequency.QuadPart) : 0;
}
static int64_t DiagQpc() { LARGE_INTEGER q{}; QueryPerformanceCounter(&q); return q.QuadPart; }
static void DiagMax(volatile LONG* p, LONG n) {
    LONG old = *p;
    while (n > old) { const LONG was = InterlockedCompareExchange(p, n, old); if (was == old) break; old = was; }
}
static void DiagXi(bool get, int64_t start) {
    if (!InterlockedCompareExchange(&g_diagEnabled, 0, 0)) return;
    const LONG us = LONG(DiagUs(start));
    if (get) { InterlockedIncrement(&g_diagXiGetCalls); InterlockedAdd(&g_diagXiGetTotal, us); DiagMax(&g_diagXiGetMax, us); }
    else { InterlockedIncrement(&g_diagXiSetCalls); InterlockedAdd(&g_diagXiSetTotal, us); DiagMax(&g_diagXiSetMax, us); }
}
static void DiagStart() {
    if (!InterlockedCompareExchange(&g_diagEnabled, 0, 0)) return;
    g_diagCurrent = {};
    g_diagCurrent.id = ++g_diagNextId;
    g_diagCurrent.thread = GetCurrentThreadId();
    g_diagCurrent.mode = LONG(InterlockedCompareExchange(&g_diagMode, 0, 0));
    g_diagCurrent.qpcStart = DiagQpc();
    g_diagCurrent.frameUs = g_diagPriorStart ? uint32_t((g_diagCurrent.qpcStart - g_diagPriorStart) * 1000000LL / g_diagFrequency.QuadPart) : 0;
    g_diagPriorStart = g_diagCurrent.qpcStart;
    g_diagCurrent.xiGetCalls = InterlockedExchange(&g_diagXiGetCalls, 0);
    g_diagCurrent.xiSetCalls = InterlockedExchange(&g_diagXiSetCalls, 0);
    g_diagCurrent.xiGetTotalUs = InterlockedExchange(&g_diagXiGetTotal, 0);
    g_diagCurrent.xiSetTotalUs = InterlockedExchange(&g_diagXiSetTotal, 0);
    g_diagCurrent.xiGetMaxUs = InterlockedExchange(&g_diagXiGetMax, 0);
    g_diagCurrent.xiSetMaxUs = InterlockedExchange(&g_diagXiSetMax, 0);
    g_diagCurrent.resetCount = InterlockedCompareExchange(&g_diagResetCount, 0, 0);
    g_diagCurrent.swapchainCount = InterlockedCompareExchange(&g_diagSwapchainCount, 0, 0);
}
static void DiagPhaseAdd(DiagPhase p, int64_t start) {
    if (InterlockedCompareExchange(&g_diagEnabled, 0, 0)) {g_diagCurrent.phaseUs[p] += DiagUs(start);++g_diagCurrent.phaseCalls[p];}
}
static void DiagFinish(uint32_t packedButtons, uint32_t priorButtons, uint32_t publishedButtons, uint32_t eye) {
    if (!InterlockedCompareExchange(&g_diagEnabled, 0, 0)) return;
    g_diagCurrent.buttons = packedButtons & 0xffff;
    g_diagCurrent.publishedButtons = publishedButtons & 0xffff;
    g_diagCurrent.previous = priorButtons & 0xffff;
    g_diagCurrent.rising = g_diagCurrent.buttons & ~g_diagCurrent.previous;
    g_diagCurrent.falling = g_diagCurrent.previous & ~g_diagCurrent.buttons;
    g_diagCurrent.triggerL = (packedButtons >> 16) & 255;
    g_diagCurrent.triggerR = (packedButtons >> 24) & 255;
    g_diagCurrent.eye = eye;
    g_diagLastEyeFrame[eye & 1] = g_diagCurrent.id;
    for (int i=0; i<2; ++i) g_diagCurrent.eyeAge[i] = g_diagLastEyeFrame[i] ? g_diagCurrent.id - g_diagLastEyeFrame[i] : 0xffffffffu;
    g_diagCurrent.qpcEnd = DiagQpc();
    g_diagCurrent.presentUs = DiagUs(g_diagCurrent.qpcStart);
    EnterCriticalSection(&g_diagLock);
    g_diagRing[g_diagCurrent.id % kDiagRing] = g_diagCurrent;
    g_diagPublishedId = g_diagCurrent.id;
    LeaveCriticalSection(&g_diagLock);
}
static const char* DiagModeName(uint32_t mode) {
    static const char* names[] = { "NORMAL", "HAPTICS_OFF", "XINPUT_OFF", "XINPUT_OFF+HAPTICS_OFF", "BUTTONS_OFF" };
    return mode < 5 ? names[mode] : "UNKNOWN";
}
static const char* DiagButtonName(uint32_t bit) {
    switch(bit) {case 0x0001:return "XI_DPAD_UP";case 0x0002:return "XI_DPAD_DOWN";
    case 0x0010:return "XI_START";case 0x0040:return "XI_LTHUMB";case 0x0080:return "XI_RTHUMB";
    case 0x0100:return "XI_LSHOULDER";case 0x0200:return "XI_RSHOULDER";
    case 0x1000:return "XI_A";case 0x2000:return "XI_B";case 0x4000:return "XI_X";case 0x8000:return "XI_Y";
    default:return "XI_OTHER";}
}
static void DiagLog(_Printf_format_string_ const char* fmt, ...) {
    // Called only by DiagWorker, never by the render/XInput threads. Separate file and
    // handle avoid contending with the mod's synchronous Log critical section.
    static char path[MAX_PATH]{};
    if(!path[0]) {
        strcpy_s(path,g_logPath);
        char* slash=strrchr(path,'\\');if(!slash)return;
        SYSTEMTIME t{};GetLocalTime(&t);
        sprintf_s(slash+1,MAX_PATH-size_t(slash+1-path),"input_diagnostic_%04u%02u%02u_%02u%02u%02u.log",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
    }
    FILE* f=nullptr;if(fopen_s(&f,path,"a")||!f)return;
    va_list args;va_start(args,fmt);vfprintf(f,fmt,args);va_end(args);fputc('\n',f);fclose(f);
}
static bool DiagReadSteamSettings(uint64_t* hash, double* scale, int* manual, bool* appEntry) {
    char base[MAX_PATH]{}, path[MAX_PATH]{}; static char data[32768]{};
    if (!GetEnvironmentVariableA("ProgramFiles(x86)",base,MAX_PATH)) return false;
    if (sprintf_s(path,"%s\\Steam\\config\\steamvr.vrsettings",base)<=0) return false;
    HANDLE file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD n=0;const BOOL ok=ReadFile(file,data,sizeof(data)-1,&n,nullptr);CloseHandle(file);
    if(!ok)return false;data[n]=0;
    uint64_t h=14695981039346656037ULL;for(DWORD i=0;i<n;++i){h^=uint8_t(data[i]);h*=1099511628211ULL;}*hash=h;
    *scale=-1.0;*manual=-1;*appEntry=false;
    if(const char* p=strstr(data,"\"supersampleScale\"")){if(const char* c=strchr(p,':'))*scale=strtod(c+1,nullptr);}
    if(const char* p=strstr(data,"\"supersampleManualOverride\"")){if(const char* c=strchr(p,':')){++c;while(*c==' '||*c=='\t')++c;*manual=_strnicmp(c,"true",4)==0?1:0;}}
    for(const char* p=data;*p;++p)if(_strnicmp(p,"singularity",11)==0){*appEntry=true;break;}
    return true;
}
static DWORD WINAPI DiagWorker(LPVOID) {
    uint32_t next = 1, reports = 0, lastPeriodic = 0; LONG lastMode=-1, lastPoll=-1;
    uint64_t oldSettingsHash=0, initialSettingsHash=0; bool haveSettings=false; uint32_t lastSettingsCheck=0;
    while (WaitForSingleObject(g_diagStop, 250) == WAIT_TIMEOUT) {
        const LONG mode=InterlockedCompareExchange(&g_diagMode,0,0), poll=InterlockedCompareExchange(&g_diagPollOff,0,0);
        if(mode!=lastMode||poll!=lastPoll) { DiagLog("DIAG MODE %s actionPolling=%s (pose location remains active)",DiagModeName(mode),poll?"OFF":"ON");lastMode=mode;lastPoll=poll; }
        static DiagFrame local[kDiagRing]; uint32_t newest;
        EnterCriticalSection(&g_diagLock);
        newest = g_diagPublishedId;
        memcpy(local, g_diagRing, sizeof(local));
        LeaveCriticalSection(&g_diagLock);
        if(!haveSettings || newest>=lastSettingsCheck+600) {
            uint64_t hash=0;double scale=0;int manual=0;bool app=false;
            if(DiagReadSteamSettings(&hash,&scale,&manual,&app)) {
                if(!haveSettings || hash!=oldSettingsHash)DiagLog("STEAMVR SETTINGS hash=fnv1a64:%016llX supersampleScale=%.4f manualOverride=%d singularityEntry=%d changed=%d (read-only; writer unknown)",hash,scale,manual,app?1:0,haveSettings?1:0);
                if(!haveSettings)initialSettingsHash=hash;
                oldSettingsHash=hash;haveSettings=true;
            }
            lastSettingsCheck=newest;
        }
        static uint32_t priorBackW=0,priorBackH=0,priorSwapW=0,priorSwapH=0,priorSceneW=0,priorSceneH=0,priorClientW=0,priorClientH=0,priorReset=0,priorRecreate=0;
        if(newest) {const DiagFrame& f=local[newest%kDiagRing];
            if(f.id==newest && (f.backW!=priorBackW||f.backH!=priorBackH||f.swapW!=priorSwapW||f.swapH!=priorSwapH||f.sceneW!=priorSceneW||f.sceneH!=priorSceneH||f.clientW!=priorClientW||f.clientH!=priorClientH||f.resetCount!=priorReset||f.swapchainCount!=priorRecreate)) {
                DiagLog("RESOLUTION frame=%u recommendedEye=%ux%u auto=%ux%u client=%ux%u requestedReset=%ldx%ld backbuffer=%ux%u sceneCandidate=%ux%u shared=%ux%u bridge=%ux%u swapchain=%ux%u imageRect=%ux%u resets=%u recreates=%u",f.id,f.recEyeW,f.recEyeH,f.autoW,f.autoH,f.clientW,f.clientH,InterlockedCompareExchange(&g_diagResetRequestW,0,0),InterlockedCompareExchange(&g_diagResetRequestH,0,0),f.backW,f.backH,f.sceneW,f.sceneH,f.sharedW,f.sharedH,f.bridgeW,f.bridgeH,f.swapW,f.swapH,f.rectW,f.rectH,f.resetCount,f.swapchainCount);
                priorBackW=f.backW;priorBackH=f.backH;priorSwapW=f.swapW;priorSwapH=f.swapH;priorSceneW=f.sceneW;priorSceneH=f.sceneH;priorClientW=f.clientW;priorClientH=f.clientH;priorReset=f.resetCount;priorRecreate=f.swapchainCount;
            }
        }
        if (newest > kDiagRing && next < newest - kDiagRing + 1) next = newest - kDiagRing + 1;
        if (newest >= lastPeriodic + 240) {
            uint64_t wall=0, present=0, xr=0, bridge=0, xi=0; uint32_t n=0, mx=0, samples[kDiagRing]{},badLifecycle=0;
            for (uint32_t id = newest > 240 ? newest-239 : 1; id <= newest; ++id) {
                const DiagFrame& f=local[id%kDiagRing]; if(f.id!=id)continue;
                samples[n++]=f.frameUs; wall+=f.frameUs; present+=f.presentUs; bridge+=f.phaseUs[DP_BRIDGE]; xi+=f.xiGetTotalUs+f.xiSetTotalUs;
                if((f.phaseCalls[DP_WAIT]||f.phaseCalls[DP_BEGIN]||f.phaseCalls[DP_END]) &&
                   (f.phaseCalls[DP_WAIT]!=1||f.phaseCalls[DP_BEGIN]!=1||f.phaseCalls[DP_END]!=1||(!poll&&f.phaseCalls[DP_SYNC]!=1)))++badLifecycle;
                for(int p=0;p<DP_COUNT;++p)if(p!=DP_BRIDGE)xr+=f.phaseUs[p];
                if(f.frameUs>mx)mx=f.frameUs;
            }
            if(n) {for(uint32_t i=1;i<n;++i){const uint32_t v=samples[i];uint32_t j=i;while(j&&samples[j-1]>v){samples[j]=samples[j-1];--j;}samples[j]=v;}
                DiagLog("InputTiming mode=%s frames=%u avg_us=%llu p95_us=%u max_us=%u present_avg_us=%llu xr_avg_us=%llu bridge_avg_us=%llu xinput_avg_us=%llu lifecycleViolations=%u", DiagModeName(InterlockedCompareExchange(&g_diagMode,0,0)),n,wall/n,samples[(n*95+99)/100-1],mx,present/n,xr/n,bridge/n,xi/n,badLifecycle);}
            lastPeriodic=newest;
        }
        while(next+90<=newest) {
            const DiagFrame& e=local[next%kDiagRing];
            if(e.id==next && (e.rising||e.falling) && reports<128) {
                uint64_t base=0; uint32_t bn=0, peak=0, peakId=0;
                for(uint32_t id=next>30?next-30:1; id<next; ++id){const DiagFrame& f=local[id%kDiagRing]; if(f.id==id){base+=f.frameUs;++bn;}}
                for(uint32_t id=next;id<=next+90;++id){const DiagFrame& f=local[id%kDiagRing];if(f.id==id&&f.frameUs>peak){peak=f.frameUs;peakId=id;}}
                DiagLog("BUTTON EDGE frame=%u mode=%s rising=0x%04X falling=0x%04X held=0x%04X published=0x%04X baseline_avg_us=%llu edge_us=%u plus1_us=%u plus2_us=%u max_us=%u max_frame=%u eye=%u ageL=%u ageR=%u",e.id,DiagModeName(e.mode),e.rising,e.falling,e.buttons&~e.rising,e.publishedButtons,bn?base/bn:0,e.frameUs,local[(next+1)%kDiagRing].frameUs,local[(next+2)%kDiagRing].frameUs,peak,peakId,e.eye,e.eyeAge[0],e.eyeAge[1]);
                for(uint32_t bit=1;bit<=0x8000;bit<<=1)if((e.rising|e.falling)&bit)DiagLog("  ButtonPress action=%s state=%s processingFrame=%u published=%d",DiagButtonName(bit),(e.rising&bit)?"RISING":"FALLING",e.id,(e.publishedButtons&bit)?1:0);
                for(uint32_t id : {next,peakId}) { const DiagFrame& f=local[id%kDiagRing]; if(f.id!=id)continue; DiagLog("  phase frame=%u present=%u wait=%u begin=%u sync=%u actions=%u poses=%u end=%u bridge=%u acquire=%u swapwait=%u release=%u haptic=%u xiGet=%u/%u/%u xiSet=%u/%u/%u trig=%u/%u rumble=%u/%u hapticCalls=%u apply=%u stop=%u apiUs=%u ampMilli=%u reset=%u swapchain=%u residual=%d",f.id,f.presentUs,f.phaseUs[DP_WAIT],f.phaseUs[DP_BEGIN],f.phaseUs[DP_SYNC],f.phaseUs[DP_ACTIONS],f.phaseUs[DP_POSES],f.phaseUs[DP_END],f.phaseUs[DP_BRIDGE],f.phaseUs[DP_ACQUIRE],f.phaseUs[DP_SWAPWAIT],f.phaseUs[DP_RELEASE],f.phaseUs[DP_HAPTIC],f.xiGetCalls,f.xiGetTotalUs,f.xiGetMaxUs,f.xiSetCalls,f.xiSetTotalUs,f.xiSetMaxUs,f.triggerL,f.triggerR,f.rumbleL,f.rumbleR,f.hapticCalls,f.hapticApplyCalls,f.hapticStopCalls,f.hapticCallUs,f.hapticAmplitudeMilli,f.resetCount,f.swapchainCount,int(f.frameUs)-int(f.presentUs)); }
                ++reports;
            }
            ++next;
        }
    }
    uint64_t hash=0;double scale=0;int manual=0;bool app=false;
    if(DiagReadSteamSettings(&hash,&scale,&manual,&app))DiagLog("STEAMVR SETTINGS shutdown hash=fnv1a64:%016llX supersampleScale=%.4f manualOverride=%d singularityEntry=%d changedSinceStartup=%d",hash,scale,manual,app?1:0,haveSettings&&hash!=initialSettingsHash?1:0);
    return 0;
}
static void DiagDisable();
static void DiagEnable() {
    if(g_diagLockReady)return;
    QueryPerformanceFrequency(&g_diagFrequency);
    InitializeCriticalSection(&g_diagLock);g_diagLockReady=true;
    g_diagStop=CreateEventA(nullptr,TRUE,FALSE,nullptr);
    if(g_diagStop)g_diagThread=CreateThread(nullptr,0,DiagWorker,nullptr,0,nullptr);
    if(g_diagThread){InterlockedExchange(&g_diagEnabled,1);atexit(DiagDisable);}
}
static void DiagDisable() {
    InterlockedExchange(&g_diagEnabled,0);
    if(g_diagStop)SetEvent(g_diagStop);
    if(g_diagThread){WaitForSingleObject(g_diagThread,1000);CloseHandle(g_diagThread);g_diagThread=nullptr;}
    if(g_diagStop){CloseHandle(g_diagStop);g_diagStop=nullptr;}
    if(g_diagLockReady){DeleteCriticalSection(&g_diagLock);g_diagLockReady=false;}
}
