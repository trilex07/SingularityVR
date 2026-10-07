#pragma once
#include <windows.h>
#include <tlhelp32.h>
// Snapshot creation can take ~100 ms on a host with thousands of threads.
// Only this worker performs it; the render thread copies a bounded completed result.
static SRWLOCK threadSnapshotLock=SRWLOCK_INIT;
static HANDLE threadSnapshotWake=nullptr,threadSnapshotWorkerHandle=nullptr,threadSnapshotStop=nullptr;
static DWORD threadSnapshotIds[256]{};
static unsigned threadSnapshotCount=0,threadSnapshotGeneration=0;
static bool threadSnapshotOverflow=false;
static DWORD WINAPI ThreadSnapshotWorker(void*) {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
    const DWORD pid=GetCurrentProcessId();
    HANDLE events[2]={threadSnapshotStop,threadSnapshotWake};
    while(WaitForMultipleObjects(2,events,FALSE,INFINITE)==WAIT_OBJECT_0+1) {
        HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
        if(snapshot==INVALID_HANDLE_VALUE)continue;
        DWORD ids[256];unsigned count=0;bool overflow=false;
        THREADENTRY32 entry{};entry.dwSize=sizeof(entry);
        if(!Thread32First(snapshot,&entry)){CloseHandle(snapshot);continue;}
        do {
            if(entry.th32OwnerProcessID!=pid)continue;
            if(count<_countof(ids))ids[count++]=entry.th32ThreadID;else overflow=true;
        } while(Thread32Next(snapshot,&entry));
        CloseHandle(snapshot);
        AcquireSRWLockExclusive(&threadSnapshotLock);
        memcpy(threadSnapshotIds,ids,count*sizeof(DWORD));
        threadSnapshotCount=count;threadSnapshotOverflow=overflow;++threadSnapshotGeneration;
        ReleaseSRWLockExclusive(&threadSnapshotLock);
    }
    return 0;
}
static bool AsyncThreadSnapshotRequest() {
    if(!threadSnapshotWorkerHandle) {
        HMODULE module=nullptr;
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&ThreadSnapshotWorker),&module))return false;
        threadSnapshotWake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        threadSnapshotStop=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        if(!threadSnapshotWake||!threadSnapshotStop) {
            if(threadSnapshotWake)CloseHandle(threadSnapshotWake);
            if(threadSnapshotStop)CloseHandle(threadSnapshotStop);
            threadSnapshotWake=threadSnapshotStop=nullptr;return false;
        }
        threadSnapshotWorkerHandle=CreateThread(nullptr,0,ThreadSnapshotWorker,nullptr,0,nullptr);
        if(!threadSnapshotWorkerHandle) {
            CloseHandle(threadSnapshotWake);CloseHandle(threadSnapshotStop);
            threadSnapshotWake=threadSnapshotStop=nullptr;return false;
        }
    }
    return SetEvent(threadSnapshotWake)!=FALSE;
}
static bool AsyncThreadSnapshotRead(DWORD (&ids)[256],unsigned& count,bool& overflow,unsigned& generation) {
    bool fresh=false;
    AcquireSRWLockShared(&threadSnapshotLock);
    if(generation!=threadSnapshotGeneration) {
        memcpy(ids,threadSnapshotIds,threadSnapshotCount*sizeof(DWORD));
        count=threadSnapshotCount;overflow=threadSnapshotOverflow;generation=threadSnapshotGeneration;fresh=true;
    }
    ReleaseSRWLockShared(&threadSnapshotLock);
    return fresh;
}
// For standalone tests / explicit orderly shutdown, never from DllMain.
static void AsyncThreadSnapshotShutdown() {
    if(threadSnapshotStop)SetEvent(threadSnapshotStop);
    if(threadSnapshotWorkerHandle){WaitForSingleObject(threadSnapshotWorkerHandle,INFINITE);CloseHandle(threadSnapshotWorkerHandle);}
    if(threadSnapshotWake)CloseHandle(threadSnapshotWake);
    if(threadSnapshotStop)CloseHandle(threadSnapshotStop);
    threadSnapshotWorkerHandle=threadSnapshotWake=threadSnapshotStop=nullptr;
}
