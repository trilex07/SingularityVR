#pragma once
// FFrame slot filter and fault-tolerant aligned loads.
// Alignment rejects the recorded crash pointer. It does not prove the page
// is readable: Readable()'s positive region cache can outlive the mapping.
// Only access faults are swallowed. Anything else propagates.

#include <cstdint>
#include <windows.h>

inline int MemoryFaultFilter(unsigned code) {
    if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

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
    } __except (MemoryFaultFilter(GetExceptionCode())) {
        return false;
    }
}

// One registration covers the FFrame slot and the following name dword.
// False means the frame or the node page faulted. *name is -1 when the
// slot was readable but is not an aligned in-range candidate.
inline bool TryLoadNodeName(uintptr_t frame, int slotOffset, int nameOffset, int32_t* name) {
    if (!name || (frame & 3u) != 0 || (slotOffset & 3) != 0 || (nameOffset & 3) != 0)
        return false;
    __try {
        const uintptr_t node = *reinterpret_cast<const volatile uintptr_t*>(frame + static_cast<unsigned>(slotOffset));
        if ((node & 3u) != 0 || node < 0x01000000u || node > 0x7FFF0000u) {
            *name = -1;
            return true;
        }
        *name = *reinterpret_cast<const volatile int32_t*>(node + static_cast<unsigned>(nameOffset));
        return true;
    } __except (MemoryFaultFilter(GetExceptionCode())) {
        return false;
    }
}

inline bool TryLoadFrameSlots(uintptr_t frame, uint32_t* slots, int count) {
    if (!slots || count <= 0 || (frame & 3u) != 0) return false;
    __try {
        const volatile uint32_t* source = reinterpret_cast<const volatile uint32_t*>(frame);
        for (int i = 0; i < count; ++i) slots[i] = source[i];
        return true;
    } __except (MemoryFaultFilter(GetExceptionCode())) {
        return false;
    }
}
