#pragma once
#include <windows.h>
#include <dinput.h>

// Owner-thread-only snapshot; notification threads touch only the epoch.
// Native callbacks still run on every replay, preserving device setup/recovery.
class InputEnumerationCache {
    DIDEVICEINSTANCEW devices_[64]{};
    unsigned count_=0;
    LONG capturedEpoch_=0;
    DWORD identity_=0, result_=0;
    bool valid_=false, capturing_=false;
public:
    volatile LONG epoch=0;
    void Invalidate(){InterlockedIncrement(&epoch);}
    LONG Epoch(){return InterlockedCompareExchange(&epoch,0,0);}
    void Begin(DWORD identity){valid_=false;capturing_=true;count_=0;identity_=identity;capturedEpoch_=Epoch();}
    void Append(const DIDEVICEINSTANCEW* device){
        if(!capturing_)return;
        if(!device || device->dwSize!=sizeof(*device) || count_==64){capturing_=false;return;}
        devices_[count_++]=*device;
    }
    void Abort(){capturing_=false;valid_=false;Invalidate();}
    void Finish(DWORD result){
        result_=result;
        valid_=capturing_ && identity_!=0 && !(result&0x80000000u) && capturedEpoch_==Epoch();
        capturing_=false;
    }
    bool Ready(DWORD identity){return valid_ && identity!=0 && identity==identity_ && capturedEpoch_==Epoch();}
    template<class Callback> DWORD Replay(Callback callback){
        // Epoch changes invalidate the next request, not a partially replayed list.
        for(unsigned i=0;i<count_;++i)if(!callback(&devices_[i],nullptr))break;
        return result_;
    }
};
