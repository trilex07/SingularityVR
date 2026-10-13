#pragma once
#include "script_node_candidate.h"

// PID14600: FindCamera's cached Readable() returned true, but the subsequent
// UObject::Class load (+0x34) faulted during a second New Game. Guard the whole
// lookup, including its bounded name/Outer walk: a predicate cannot pin UE3
// allocations across a load. This does not establish object liveness, make
// the graph atomic, or protect later accesses made by callers.
using CameraLookup = uintptr_t (*)(void*);

__declspec(noinline) inline bool TryCameraLookup(CameraLookup lookup, void* context,
                                                uintptr_t* out) {
    if (!lookup || !out) return false;
    uintptr_t next = 0;
    __try {
        next = lookup(context);
    } __except (MemoryFaultFilter(GetExceptionCode())) {
        return false;
    }
    *out = next; // Never publish a partially completed lookup.
    return true;
}
