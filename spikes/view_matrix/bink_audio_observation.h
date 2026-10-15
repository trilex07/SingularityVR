#pragma once
#include <windows.h>
#include "script_node_candidate.h"

// Read-only snapshots of observed exported Bink arguments. No media changes,
// ownership transfer or dependence on BINK handle layout. Faults never prevent
// the native API from receiving the original arguments.
inline bool BinkObserveTracks(DWORD count, const DWORD* tracks, DWORD out[8]) {
    if (!out || count>8 || (count && !tracks)) return false;
    __try {
        for (DWORD i=0;i<count;++i) out[i]=tracks[i];
        return true;
    } __except (MemoryFaultFilter(GetExceptionCode())) { return false; }
}
inline bool BinkObserveName(const char* source, char out[256], DWORD flags = 0) {
    if (!source || !out) return false;
    out[0]=0;
    if (flags & 0x04000000) {
        HANDLE hFile = (HANDLE)(uintptr_t)source;
        DWORD ret = GetFinalPathNameByHandleA(hFile, out, 256, FILE_NAME_NORMALIZED);
        if (ret > 0 && ret < 256) {
            if (strncmp(out, "\\\\?\\", 4) == 0) {
                memmove(out, out + 4, strlen(out + 4) + 1);
            }
            return true;
        }
        out[0] = 0;
        return false;
    }
    __try {
        for (unsigned i=0;i<255;++i) {
            out[i]=source[i];
            if (!out[i]) return true;
            if(static_cast<unsigned char>(out[i])<32 || static_cast<unsigned char>(out[i])>126) {
                out[0]=0;return false; // memory-backed BIK header is not a pathname
            }
        }
        out[255]=0;
        return false; // truncated/not a filename; don't interpret it
    } __except (MemoryFaultFilter(GetExceptionCode())) { out[0]=0; return false; }
}
