#pragma once
// MAIN: minimal weapon-wheel catalog file reader (startup/lazy-load only,
// never the hot path). Parses the fixed SIDE catalog shape:
// {"sectors":[{"index":N,...,"gun_verts":M,...},...],"cycle_order":[a,b,...]}
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace wheel_catalog {

struct Sector { int index = -1; long verts = 0; std::string atlas; };
struct Catalog { std::vector<Sector> sectors; std::vector<long> cycle; };

inline const char* SkipWs(const char* p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
    return p;
}

// Integer value of "key":N inside one {...} object. False when absent/broken.
inline bool ObjInt(const char* obj, const char* objEnd, const char* key, long& v) {
    const size_t kl = std::strlen(key);
    for (const char* p = obj; p + kl + 1 < objEnd; ++p) {
        if (*p != '"') continue;
        if (std::strncmp(p + 1, key, kl) != 0 || p[1 + kl] != '"') continue;
        const char* c = SkipWs(p + 1 + kl + 1);
        if (*c != ':') continue;
        c = SkipWs(c + 1);
        char* end = nullptr;
        const long n = std::strtol(c, &end, 10);
        if (end == c) continue;
        v = n;
        return true;
    }
    return false;
}

inline bool ObjStr(const char* obj, const char* objEnd, const char* key, std::string& v) {
    const size_t kl = std::strlen(key);
    for (const char* p = obj; p + kl + 1 < objEnd; ++p) {
        if (*p != '"') continue;
        if (std::strncmp(p + 1, key, kl) != 0 || p[1 + kl] != '"') continue;
        const char* c = SkipWs(p + 1 + kl + 1);
        if (*c != ':') continue;
        c = SkipWs(c + 1);
        if (*c != '"') continue;
        const char* e = std::strchr(c + 1, '"');
        if (!e || e >= objEnd) continue;
        v.assign(c + 1, e);
        return true;
    }
    return false;
}

inline bool Parse(const char* text, Catalog& out) {
    out.sectors.clear();
    out.cycle.clear();
    if (!text) return false;
    const char* sec = std::strstr(text, "\"sectors\"");
    if (!sec) return false;
    const char* arr = std::strchr(sec, '[');
    if (!arr) return false;
    for (const char* p = arr + 1; *p; ++p) {
        if (*p == ']') break;
        if (*p != '{') continue;
        const char* end = std::strchr(p, '}');
        if (!end) return false;
        long index = -1, verts = 0;
        if (!ObjInt(p, end, "index", index)) return false;
        ObjInt(p, end, "gun_verts", verts); // absent means unused sector
        std::string atlas;
        ObjStr(p, end, "atlas_icon", atlas); // absent means no icon
        out.sectors.push_back(Sector{ (int)index, verts, atlas });
        p = end;
    }
    if (out.sectors.empty()) return false;
    const char* cyc = std::strstr(text, "\"cycle_order\"");
    if (!cyc) return false;
    const char* ca = std::strchr(cyc, '[');
    if (!ca) return false;
    for (const char* p = ca + 1; *p; ++p) {
        if (*p == ']') break;
        if ((*p >= '0' && *p <= '9') || *p == '-') {
            char* end = nullptr;
            out.cycle.push_back(std::strtol(p, &end, 10));
            if (end == p) return false;
            p = end - 1;
        }
    }
    return !out.cycle.empty();
}

} // namespace wheel_catalog
