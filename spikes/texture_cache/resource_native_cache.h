#pragma once
#include <windows.h>
#include <cstddef>
#include <cstdint>

namespace texnative {
// Project-owned namespace, not a UE3 field or an engine GUID. A byte record,
// flags0: no IUnknown references, original resource pins, or pixel writes.
inline const GUID nativeDecisionId =
    {0xfda244ce,0x8924,0x496f,{0xa3,0x2d,0x9c,0xfa,0x9e,0x06,0xc7,0x1c}};
struct Stamp { std::uint32_t magic; std::uint32_t version; };
inline constexpr Stamp nativeDecision{0x4e545653u,1u};

template<class Resource> bool KnownNative(Resource* resource) {
    if (!resource) return false;
    Stamp sample{};
    DWORD size=sizeof(sample);
    return SUCCEEDED(resource->GetPrivateData(nativeDecisionId,&sample,&size)) &&
        size==sizeof(sample) && sample.magic==nativeDecision.magic &&
        sample.version==nativeDecision.version;
}
template<class Resource> bool RememberNative(Resource* resource) {
    return resource && SUCCEEDED(resource->SetPrivateData(nativeDecisionId,
        &nativeDecision,sizeof(nativeDecision),0));
}
// Only the unsupported-private-data fallback uses this bounded table. Rotate
// across nonreplacement victims, never evict/Release a held positive result.
template<class Entry> std::size_t NegativeVictim(const Entry* entries,
                                                std::size_t count,
                                                std::size_t& cursor) {
    if (!entries || !count) return SIZE_MAX;
    cursor%=count;
    for(std::size_t n=0;n<count;++n) {
        const std::size_t slot=(cursor+n)%count;
        if(!entries[slot].replacement) {cursor=(slot+1)%count;return slot;}
    }
    return SIZE_MAX;
}
}
