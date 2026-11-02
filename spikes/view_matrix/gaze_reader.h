#pragma once
#include "gaze_interaction.h"
namespace svrgaze {
class Reader {
    HANDLE mapping=nullptr;
    const svr::psvr2::GazeShared* view=nullptr;
    ULONGLONG retry=0;
    SRWLOCK lock=SRWLOCK_INIT;
    bool Open(){
        if(view)return true;
        const auto now=GetTickCount64();if(now<retry)return false;retry=now+2000;
        wchar_t name[80]{};swprintf_s(name,L"Local\\SingularityVR.Gaze.%lu",GetCurrentProcessId());
        mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,name);
        if(!mapping)return false;
        view=static_cast<const svr::psvr2::GazeShared*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof(*view)));
        if(!view){CloseHandle(mapping);mapping=nullptr;return false;}
        return true;
    }
    bool ReadSample(svr::psvr2::GazeSample& s){
        if(!Open())return false;
        __try{return svr::psvr2::GazeRead(*view,s);}
        __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
public:
    bool Read(float (&d)[3]){
        AcquireSRWLockExclusive(&lock);
        svr::psvr2::GazeSample s{};const bool read=ReadSample(s);
        ReleaseSRWLockExclusive(&lock);
        return read&&Direction(s,GetTickCount64(),d);
    }
    void Close(){
        AcquireSRWLockExclusive(&lock);
        if(view)UnmapViewOfFile(view);if(mapping)CloseHandle(mapping);
        view=nullptr;mapping=nullptr;retry=0;
        ReleaseSRWLockExclusive(&lock);
    }
    ~Reader(){Close();}
};
}
