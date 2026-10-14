#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>

// Read-only chain proved from live UObject::execGetStateName (004B7B60).
// These are OBJECT/FRAME fields, not guessed game cinematic flags. Caller must
// additionally name/validate the controller and StateNode class before use.
struct CinematicStateObservation {
    std::uintptr_t controllerClass=0, frame=0, stateNode=0, stateClass=0;
    std::int32_t stateName=-1;
    bool valid=false;
};
inline int CinematicObservationFaultFilter(unsigned code) {
    return code==EXCEPTION_ACCESS_VIOLATION || code==EXCEPTION_IN_PAGE_ERROR
        ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}
inline CinematicStateObservation ReadCinematicStateObservation(std::uintptr_t controller) {
    CinematicStateObservation s;
    if(!controller || (controller&3))return s;
    __try {
        s.controllerClass=*reinterpret_cast<const std::uintptr_t*>(controller+0x34);
        s.frame=*reinterpret_cast<const std::uintptr_t*>(controller+0x18);
        if(!s.frame || (s.frame&3))return s;
        s.stateNode=*reinterpret_cast<const std::uintptr_t*>(s.frame+0x28);
        if(!s.stateNode || (s.stateNode&3))return s;
        s.stateClass=*reinterpret_cast<const std::uintptr_t*>(s.stateNode+0x34);
        s.stateName=*reinterpret_cast<const std::int32_t*>(s.stateNode+0x2c);
        s.valid=s.controllerClass && s.stateClass && s.stateName>=0;
    } __except(CinematicObservationFaultFilter(GetExceptionCode())) {s.valid=false;}
    return s;
}
