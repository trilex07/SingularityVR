#pragma once
// MAIN: minimal plates.json reader for the Katorga plates atlas sheet.
// Same shape as the icon atlas: {"width":W,"height":H,"png_sha256":"...",
// "rects":[{"id":"...","x":N,"y":N,"w":N,"h":N},...]}. Pure scan, no file
// I/O (tested below). Installed file lives next to the plates PNG in
// vr-assets/hud/katorga/plates-atlas/.
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace plates_atlas {

struct Rect { std::string id; int x = 0, y = 0, w = 0, h = 0; };
struct Sheet { int width = 0, height = 0; std::string pngSha256; std::vector<Rect> rects; };

inline const char* SkipWs(const char* p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
    return p;
}

inline bool ObjInt(const char* obj, const char* objEnd, const char* key, int& v) {
    const size_t kl = std::strlen(key);
    for (const char* p = obj; p + kl + 1 < objEnd; ++p) {
        if (*p != '"') continue;
        if (std::strncmp(p + 1, key, kl) != 0 || p[1 + kl] != '"') continue;
        const char* c = SkipWs(p + 1 + kl + 1);
        if (*c != ':') continue;
        c = SkipWs(c + 1);
        char* end = nullptr;
        const long n = std::strtol(c, &end, 10);
        if (end == c || n < 0 || n > 32768) continue;
        v = (int)n;
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

inline bool Parse(const char* text, Sheet& out) {
    out.rects.clear();
    out.pngSha256.clear();
    if (!text) return false;
    ObjInt(text, text + std::strlen(text), "width", out.width);
    ObjInt(text, text + std::strlen(text), "height", out.height);
    ObjStr(text, text + std::strlen(text), "png_sha256", out.pngSha256);
    const char* rc = std::strstr(text, "\"rects\"");
    if (!rc) return false;
    const char* arr = std::strchr(rc, '[');
    if (!arr) return false;
    for (const char* p = arr + 1; *p; ++p) {
        if (*p == ']') break;
        if (*p != '{') continue;
        const char* end = std::strchr(p, '}');
        if (!end) return false;
        Rect r;
        if (!ObjStr(p, end, "id", r.id)) return false;
        if (!ObjInt(p, end, "x", r.x) || !ObjInt(p, end, "y", r.y) ||
            !ObjInt(p, end, "w", r.w) || !ObjInt(p, end, "h", r.h)) return false;
        if (r.w <= 0 || r.h <= 0) return false;
        out.rects.push_back(r);
        p = end;
    }
    return out.width > 0 && out.height > 0 && !out.rects.empty();
}

inline const Rect* Find(const Sheet& sheet, const char* id) {
    if (!id) return nullptr;
    for (size_t i = 0; i < sheet.rects.size(); ++i)
        if (sheet.rects[i].id == id) return &sheet.rects[i];
    return nullptr;
}

} // namespace plates_atlas
