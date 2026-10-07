#pragma once
#include <dbghelp.h>
// Desktop-only diagnostic: observe execute-access violations; never handle/suppress them.
static decltype(&MiniDumpWriteDump) startupDumpWriter=nullptr;
static LONG startupDumpAttempted=0;
static LONG CALLBACK CaptureStartupException(EXCEPTION_POINTERS* info) {
    if(info && info->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION &&
       info->ExceptionRecord->NumberParameters>=2 && info->ExceptionRecord->ExceptionInformation[0]==8 &&
       InterlockedCompareExchange(&startupDumpAttempted,1,0)==0 && startupDumpWriter) {
        HANDLE file=CreateFileW(L"C:\\Users\\Trilex\\vr-mods\\research\\etw\\performance1-crash-20261006\\startup-veh.dmp",GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);
        if(file!=INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION ei{GetCurrentThreadId(),info,FALSE};
            startupDumpWriter(GetCurrentProcess(),GetCurrentProcessId(),file,
                (MINIDUMP_TYPE)(MiniDumpWithFullMemory|MiniDumpWithThreadInfo),&ei,nullptr,nullptr);
            CloseHandle(file);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static void InstallStartupCrashCapture() {
    static bool installed=false;if(installed)return;installed=true;
    HMODULE lib=LoadLibraryW(L"dbghelp.dll");
    if(lib){startupDumpWriter=(decltype(startupDumpWriter))GetProcAddress(lib,"MiniDumpWriteDump");if(startupDumpWriter)AddVectoredExceptionHandler(1,CaptureStartupException);}
}
