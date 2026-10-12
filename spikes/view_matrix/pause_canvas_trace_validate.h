#pragma once
// Offline GO/NO-GO for narrow pause-canvas logs.
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace pausecanvas {
namespace validate {

enum Reason : unsigned {
    Ok = 0,
    MissingBegin = 1u << 0,
    MissingEnd = 1u << 1,
    MissingSettled = 1u << 2,
    MissingBandA = 1u << 3,
    MissingBandB = 1u << 4,
    RtSplit = 1u << 5,
    NotOrthoInScope = 1u << 6,
    UnexplainedMid = 1u << 7,
    PanelOpen = 1u << 8,
    Truncated = 1u << 9,
    BakedSbsColumns = 1u << 10
};

struct Txn {
    bool began = false, ended = false, settled = false, panelOpen = false, truncated = false;
    unsigned long long token = 0, surf = 0;
    unsigned bandA = 0, bandB = 0, mid = 0, notOrtho = 0;
    float aMinTx = 0, aMaxTx = 0, bMinTx = 0, bMaxTx = 0;
    bool haveA = false, haveB = false;
};

inline unsigned EvaluateTxn(const Txn& t) {
    unsigned why = Ok;
    if (!t.began) why |= MissingBegin;
    if (!t.ended) why |= MissingEnd;
    if (t.panelOpen) why |= PanelOpen;
    if (t.truncated) why |= Truncated;
    if (t.notOrtho) why |= NotOrthoInScope;
    if (t.mid) why |= UnexplainedMid;
    if (!t.bandA) why |= MissingBandA;
    if (!t.bandB) why |= MissingBandB;
    if (t.settled && t.haveA && t.haveB) {
        // Clip-space columns: disjoint tx ranges with gap suggest pre-baked SBS columns in GFx space.
        const float gap = t.bMinTx - t.aMaxTx;
        if (gap > 0.05f && t.aMaxTx < 0.f && t.bMinTx > 0.f) why |= BakedSbsColumns;
    }
    return why;
}

inline bool Go(unsigned why) { return why == Ok; }

struct Summary {
    unsigned why = Ok;
    unsigned txnCount = 0;
    unsigned settledPass = 0;
    unsigned sameSurfSettled = 0;
    unsigned long long commonSurf = 0;
};

inline Summary EvaluateLog(const std::vector<std::string>& lines) {
    std::vector<Txn> txns;
    auto get = [&](unsigned id) -> Txn& {
        if (id >= txns.size()) txns.resize(id + 1);
        return txns[id];
    };

    for (const std::string& line : lines) {
        if (line.find("pause-canvas txn=") != 0) continue;
        const char* tpos = std::strstr(line.c_str(), "txn=");
        if (!tpos) continue;
        unsigned id = (unsigned)std::strtoul(tpos + 4, nullptr, 10);
        Txn& t = get(id);
        if (line.find(" ev=begin") != std::string::npos) {
            t.began = true;
            t.panelOpen = line.find("panel=1") != std::string::npos;
        } else if (line.find(" ev=end") != std::string::npos) {
            t.ended = true;
            t.settled = line.find(" settled=1") != std::string::npos;
            t.truncated = line.find("reason=truncated") != std::string::npos;
            if (const char* p = std::strstr(line.c_str(), "bandA="))
                t.bandA = (unsigned)std::strtoul(p + 6, nullptr, 10);
            if (const char* p = std::strstr(line.c_str(), " bandB="))
                t.bandB = (unsigned)std::strtoul(p + 7, nullptr, 10);
            if (const char* p = std::strstr(line.c_str(), " mid="))
                t.mid = (unsigned)std::strtoul(p + 5, nullptr, 10);
            if (const char* p = std::strstr(line.c_str(), " notOrtho="))
                t.notOrtho = (unsigned)std::strtoul(p + 10, nullptr, 10);
            if (const char* p = std::strstr(line.c_str(), " boundsA_tx=[")) {
                std::sscanf(p, " boundsA_tx=[%f,%f]", &t.aMinTx, &t.aMaxTx);
                t.haveA = true;
            }
            if (const char* p = std::strstr(line.c_str(), " boundsB_tx=[")) {
                std::sscanf(p, " boundsB_tx=[%f,%f]", &t.bMinTx, &t.bMaxTx);
                t.haveB = true;
            }
        } else if (line.find(" ev=draw") != std::string::npos) {
            if (const char* p = std::strstr(line.c_str(), "surf=")) {
                const unsigned long long surf = std::strtoull(p + 5, nullptr, 16);
                if (!t.surf) t.surf = surf;
                else if (surf != t.surf) { /* flagged at summary */ }
            }
        }
    }

    Summary s;
    unsigned long long surf = 0;
    for (Txn& t : txns) {
        if (!t.began && !t.ended) continue;
        ++s.txnCount;
        if (!t.settled) continue;
        const unsigned w = EvaluateTxn(t);
        s.why |= w;
        if (w == Ok) ++s.settledPass;
        if (t.surf) {
            if (!surf) surf = t.surf;
            if (t.surf == surf) ++s.sameSurfSettled;
        }
    }
    s.commonSurf = surf;
    if (s.settledPass == 0) s.why |= MissingSettled;
    if (s.txnCount == 0) s.why |= MissingBegin | MissingEnd;
    return s;
}

}  // namespace validate
}  // namespace pausecanvas
