#pragma once
#include "script_node_candidate.h"

// Readable() is only a predicate; a released/reprotected UE3 pawn can fault
// between that cached predicate and this copy (PID41200, d3d9 RVA0x2898C).
// Keep the fault boundary outside Hook_Present's C++ objects. Only publish a
// complete sample. This is NOT an atomic game-thread snapshot or a liveness
// proof; it bounds memory access without changing camera math.
struct PawnCameraSample { float location[3]; float eyeHeight; };

inline bool TryReadPawnCameraSample(uintptr_t pawn, int locationOffset,
                                   int eyeHeightOffset, PawnCameraSample* out) {
    if (!out || !pawn || (pawn & 3u) || locationOffset < 0 || eyeHeightOffset < 0 ||
        (locationOffset & 3) || (eyeHeightOffset & 3)) return false;
    const uintptr_t loc = static_cast<unsigned>(locationOffset);
    const uintptr_t eye = static_cast<unsigned>(eyeHeightOffset);
    if (pawn > UINTPTR_MAX - loc - 12u || pawn > UINTPTR_MAX - eye - 4u) return false;
    PawnCameraSample next{};
    __try {
        const volatile float* xyz = reinterpret_cast<const volatile float*>(pawn + loc);
        next.location[0] = xyz[0];
        next.location[1] = xyz[1];
        next.location[2] = xyz[2];
        next.eyeHeight = *reinterpret_cast<const volatile float*>(pawn + eye);
    } __except (MemoryFaultFilter(GetExceptionCode())) {
        return false;
    }
    *out = next;
    return true;
}
