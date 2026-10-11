#pragma once
// FFrame slot filter and a single aligned dword load.
// Alignment rejects the recorded crash pointer. It does not prove the page
// is readable: Readable()'s positive region cache can outlive the mapping.

#include <cstdint>
#include <windows.h>

inline bool ScriptNodeCandidate(uintptr_t pointer, uintptr_t frame) {
    if (pointer < 0x01000000u || pointer > 0x7FFF0000u) return false;
    if ((pointer & 3u) != 0) return false;
    if (frame != 0 && pointer > frame - 0x10000u && pointer < frame + 0x10000u) return false;
    return true;
}

inline bool TryLoadAlignedI32(uintptr_t address, int32_t* out) {
    if (!out || (address & 3u) != 0) return false;
    __try {
        *out = *reinterpret_cast<const volatile int32_t*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
