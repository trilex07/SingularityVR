// Quality-tier naming for the standalone texture injector.
//
// The deploy step and the runtime lookup MUST agree on the file name, so both
// derive it from here:
//   material-remaster  -> "<name>.dds"        (native-resolution remaster)
//   2x full pipeline    -> "<name>_2x.dds"     (Lanczos + IBP output)
//   4K proof set        -> "<name>_4K.dds"
//   8K proof set        -> "<name>_8K.dds"
#pragma once
#include <cstddef>
#include <cstring>

namespace texq {
inline const char* QualitySuffix(const char* tier) {
    if (!tier || !tier[0]) return "_2x";
    if (strcmp(tier, "4K") == 0) return "_4K";
    if (strcmp(tier, "8K") == 0) return "_8K";
    if (strcmp(tier, "2x") == 0) return "_2x";
    return "_2x";
}
}
