#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace inventoryvm {
// Observed ProcessInternal dispatch: Code=frame+24, Node=frame+16,
// Object=frame+20. Both opcode dispatch sites use the same GNatives table.
inline uintptr_t DispatchTable(const unsigned char* code, size_t n) {
    if (!code || n < 40) return 0;
    uintptr_t table = 0; unsigned hits = 0;
    for (size_t i = 0; i + 12 <= n; ++i) {
        // mov edx,[edx*4+absolute table]
        if (code[i] != 0x8b || code[i+1] != 0x14 || code[i+2] != 0x95) continue;
        uint32_t p = 0; std::memcpy(&p, code+i+3, 4);
        if (!p || (p & 3)) return 0;
        if (table && table != p) return 0;
        table = p; ++hits;
    }
    return hits == 2 ? table : 0;
}
inline bool DropTail(const unsigned char* p, size_t n) {
    // VirtualFunction FName is followed by EmptyParmValue and EndFunctionParms.
    // These are part of the call, not independent following statements.
    return p && n >= 11 && p[0] == 0x1b && p[9] == 0x4a && p[10] == 0x16;
}
}
