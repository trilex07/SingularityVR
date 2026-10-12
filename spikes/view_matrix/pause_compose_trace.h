#pragma once
// Offline contract for PauseComposeTrace. Not included by the renderer.
// A GO verdict means one class-3 token owned a closed transaction whose
// main-canvas and Objectives draws shared one render target and were not
// split into eye halves. It does not prove pixels or input.
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace pausecompose {

enum Reason : unsigned {
    Ok = 0,
    MissingBegin = 1u << 0,
    MissingEnd = 1u << 1,
    TokenMismatch = 1u << 2,
    MissingMain = 1u << 3,
    MissingObjectives = 1u << 4,
    UnknownGroup = 1u << 5,
    UnrelatedDraw = 1u << 6,
    RtSplit = 1u << 7,
    BakedSbs = 1u << 8,
    EndNotComplete = 1u << 9,
    PanelOpen = 1u << 10,
    Truncated = 1u << 11,
    MissingState = 1u << 12
};

enum class Group { Main, Objectives, Selection, Cursor, Panel, World, Overlay, Unknown };
enum class State { Root, Selection, Objectives, Other };

struct Draw {
    unsigned long long token = 0;
    unsigned long long surf = 0;
    Group group = Group::Unknown;
    int vpX = 0, vpW = 0, rtW = 0;
};

struct Txn {
    bool began = false;
    bool ended = false;
    bool panelOpen = false;
    bool truncated = false;
    bool endComplete = false;
    State state = State::Other;
    unsigned long long token = 0;
    unsigned long long menuSurf = 0;
    bool haveSurf = false;
    bool sawMain = false;
    bool sawObjectives = false;
    std::vector<Draw> draws;
};

inline bool IsMenuGroup(Group g) {
    return g == Group::Main || g == Group::Objectives || g == Group::Selection ||
           g == Group::Cursor || g == Group::Panel;
}

inline unsigned Evaluate(const Txn& txn) {
    unsigned why = Ok;
    if (!txn.began) why |= MissingBegin;
    if (!txn.ended) why |= MissingEnd;
    if (txn.panelOpen) why |= PanelOpen;
    if (txn.truncated) why |= Truncated;
    if (txn.ended && !txn.endComplete) why |= EndNotComplete;
    if (txn.state == State::Other) why |= MissingState;
    if (!txn.sawMain) why |= MissingMain;
    if (!txn.sawObjectives) why |= MissingObjectives;
    for (const Draw& d : txn.draws) {
        if (txn.began && d.token != txn.token) why |= TokenMismatch;
        if (d.group == Group::Unknown) why |= UnknownGroup;
        if (d.group == Group::World || d.group == Group::Overlay) why |= UnrelatedDraw;
        if (IsMenuGroup(d.group) && txn.haveSurf && d.surf != txn.menuSurf) why |= RtSplit;
    }
    int mainL = 0, mainR = 0, objL = 0, objR = 0;
    bool haveMain = false, haveObj = false;
    for (const Draw& d : txn.draws) {
        if (d.group == Group::Main) { mainL = d.vpX; mainR = d.vpX + d.vpW; haveMain = true; }
        if (d.group == Group::Objectives) { objL = d.vpX; objR = d.vpX + d.vpW; haveObj = true; }
    }
    if (haveMain && haveObj && mainR <= objL && objL > 0) why |= BakedSbs;
    if (haveMain && haveObj && objR <= mainL && mainL > 0) why |= BakedSbs;
    (void)mainR;
    return why;
}

inline bool Go(unsigned why) { return why == Ok; }

struct LogVerdict {
    unsigned why = MissingBegin | MissingEnd | MissingMain | MissingObjectives | MissingState;
    bool sawRoot = false, sawSelection = false, sawObjectivesState = false;
};

inline Group ParseGroup(const char* s) {
    if (!std::strcmp(s, "main")) return Group::Main;
    if (!std::strcmp(s, "objectives")) return Group::Objectives;
    if (!std::strcmp(s, "selection")) return Group::Selection;
    if (!std::strcmp(s, "cursor")) return Group::Cursor;
    if (!std::strcmp(s, "panel")) return Group::Panel;
    if (!std::strcmp(s, "world")) return Group::World;
    if (!std::strcmp(s, "overlay")) return Group::Overlay;
    return Group::Unknown;
}

inline State ParseState(const char* s) {
    if (!std::strcmp(s, "root")) return State::Root;
    if (!std::strcmp(s, "selection")) return State::Selection;
    if (!std::strcmp(s, "objectives")) return State::Objectives;
    return State::Other;
}

inline const char* Field(const std::string& line, const char* key, char* out, int cap) {
    const std::string needle = std::string(key) + "=";
    const auto at = line.find(needle);
    if (at == std::string::npos) { if (cap) out[0] = 0; return out; }
    auto end = line.find(' ', at);
    const auto value = line.substr(at + needle.size(), end == std::string::npos ? std::string::npos : end - (at + needle.size()));
    const int n = (int)value.size() < cap - 1 ? (int)value.size() : cap - 1;
    if (n > 0) std::memcpy(out, value.data(), (size_t)n);
    out[n] = 0;
    return out;
}

inline unsigned long long FieldU(const std::string& line, const char* key) {
    char buf[64];
    Field(line, key, buf, 64);
    return buf[0] ? std::strtoull(buf, nullptr, 0) : 0;
}

// One transaction per txn id. Later lines with the same id extend it.
inline LogVerdict EvaluateLog(const std::vector<std::string>& lines) {
    std::vector<Txn> txns;
    auto txn = [&](unsigned long long id) -> Txn& {
        if (id >= txns.size()) txns.resize((size_t)id + 1);
        return txns[(size_t)id];
    };
    for (const std::string& line : lines) {
        if (line.find("pause-compose ") != 0 && line.find("pause-compose\t") != 0) continue;
        const unsigned long long id = FieldU(line, "txn");
        char ev[32];
        Field(line, "ev", ev, 32);
        Txn& t = txn(id);
        if (!std::strcmp(ev, "begin")) {
            t.began = true;
            t.token = FieldU(line, "token");
            t.panelOpen = FieldU(line, "panel") != 0;
            char state[32];
            Field(line, "state", state, 32);
            t.state = ParseState(state);
        } else if (!std::strcmp(ev, "end")) {
            t.ended = true;
            t.endComplete = std::strstr(line.c_str(), "reason=complete") != nullptr;
            t.truncated = std::strstr(line.c_str(), "reason=truncated") != nullptr;
            if (FieldU(line, "token") && FieldU(line, "token") != t.token) t.token = ~0ull;
        } else if (!std::strcmp(ev, "draw")) {
            Draw d;
            d.token = FieldU(line, "token");
            d.surf = FieldU(line, "surf");
            char group[32];
            Field(line, "group", group, 32);
            d.group = ParseGroup(group);
            char vp[64];
            Field(line, "vp", vp, 64);
            d.vpX = (int)std::strtol(vp, nullptr, 10);
            const char* comma = std::strchr(vp, ',');
            comma = comma ? std::strchr(comma + 1, ',') : nullptr;
            d.vpW = comma ? (int)std::strtol(comma + 1, nullptr, 10) : 0;
            d.rtW = (int)FieldU(line, "rtw");
            if (IsMenuGroup(d.group)) {
                if (!t.haveSurf) { t.menuSurf = d.surf; t.haveSurf = true; }
            }
            if (d.group == Group::Main) t.sawMain = true;
            if (d.group == Group::Objectives) t.sawObjectives = true;
            t.draws.push_back(d);
        }
    }
    LogVerdict v;
    v.why = Ok;
    bool any = false;
    for (const Txn& t : txns) {
        if (!t.began && !t.ended && t.draws.empty()) continue;
        any = true;
        v.why |= Evaluate(t);
        if (Evaluate(t) == Ok && t.state == State::Root) v.sawRoot = true;
        if (Evaluate(t) == Ok && t.state == State::Selection) v.sawSelection = true;
        if (Evaluate(t) == Ok && t.state == State::Objectives) v.sawObjectivesState = true;
    }
    if (!any) v.why |= MissingBegin | MissingEnd | MissingMain | MissingObjectives | MissingState;
    if (!v.sawRoot || !v.sawSelection || !v.sawObjectivesState) v.why |= MissingState;
    return v;
}

}  // namespace pausecompose
