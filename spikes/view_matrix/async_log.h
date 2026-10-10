#pragma once
#include <cstdarg>
#include <cstdio>
#include <share.h>
// Bounded, allocation-free producers. Only the worker opens/writes/flushes the file.
static SRWLOCK asyncLogLock=SRWLOCK_INIT;
static char asyncLogLines[256][4096];
static unsigned asyncLogHead=0,asyncLogTail=0,asyncLogCount=0;
static LONG asyncLogActive=0,asyncLogDropped=0;
static HANDLE asyncLogWake=nullptr;
static char asyncLogPath[MAX_PATH]{};
static DWORD WINAPI AsyncLogWorker(void*) {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
    FILE* file=nullptr;
    for(;;) {
        WaitForSingleObject(asyncLogWake,1000);
        if(!file)file=_fsopen(asyncLogPath,"a",_SH_DENYNO);
        if(!file)continue;
        char line[4096];bool wrote=false;
        for(;;) {
            AcquireSRWLockExclusive(&asyncLogLock);
            if(!asyncLogCount){ReleaseSRWLockExclusive(&asyncLogLock);break;}
            memcpy(line,asyncLogLines[asyncLogTail],sizeof(line));
            asyncLogTail=(asyncLogTail+1)%256;--asyncLogCount;
            ReleaseSRWLockExclusive(&asyncLogLock);
            fputs(line,file);fputc('\n',file);wrote=true;
        }
        const LONG lost=InterlockedExchange(&asyncLogDropped,0);
        if(lost){fprintf(file,"AsyncLog: dropped=%ld (queue full; render thread never waits for disk)\n",lost);wrote=true;}
        if(wrote)fflush(file);
    }
}
static void AsyncLogInit(const char* path) {
    if(!path || !path[0] || InterlockedCompareExchange(&asyncLogActive,2,0)!=0)return;
    strncpy_s(asyncLogPath,path,_TRUNCATE);
    asyncLogWake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    // The worker must never execute from an unloaded DLL. Game shutdown remains process-owned.
    HMODULE owner=nullptr;
    if(!asyncLogWake || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCSTR>(&AsyncLogWorker),&owner)) {
        if(asyncLogWake)CloseHandle(asyncLogWake);asyncLogWake=nullptr;InterlockedExchange(&asyncLogActive,0);return;
    }
    HANDLE worker=CreateThread(nullptr,0,AsyncLogWorker,nullptr,0,nullptr);
    if(!worker){CloseHandle(asyncLogWake);asyncLogWake=nullptr;InterlockedExchange(&asyncLogActive,0);return;}
    CloseHandle(worker);InterlockedExchange(&asyncLogActive,1);
}
static bool AsyncLogEnqueue(const char* fmt,va_list ap) {
    if(InterlockedCompareExchange(&asyncLogActive,0,0)!=1)return false;
    char line[4096];_vsnprintf_s(line,sizeof(line),_TRUNCATE,fmt,ap);
    AcquireSRWLockExclusive(&asyncLogLock);
    if(asyncLogCount==256)InterlockedIncrement(&asyncLogDropped);
    else {memcpy(asyncLogLines[asyncLogHead],line,sizeof(line));asyncLogHead=(asyncLogHead+1)%256;++asyncLogCount;}
    ReleaseSRWLockExclusive(&asyncLogLock);SetEvent(asyncLogWake);return true;
}
