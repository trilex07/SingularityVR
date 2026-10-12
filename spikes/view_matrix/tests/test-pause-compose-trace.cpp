#include "../pause_compose_trace.h"
#include <cstdio>
#include <string>
#include <vector>

static int g_fails = 0;

static void Expect(bool cond, const char* name) {
    if (cond) {
        std::printf("PASS %s\n", name);
        return;
    }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

static std::string Begin(const char* id, const char* state) {
    return std::string("pause-compose txn=") + id + " ev=begin token=10 owner=3 panel=0 state=" + state + " frame=1";
}
static std::string LineDraw(const char* id, const char* group, const char* vp) {
    return std::string("pause-compose txn=") + id + " ev=draw token=10 surf=100 group=" + group +
           " vp=" + vp + " rtw=5264";
}
static std::string End(const char* id) {
    return std::string("pause-compose txn=") + id + " ev=end token=10 reason=complete";
}

static std::vector<std::string> CompleteSet() {
    return {
        Begin("1", "root"), LineDraw("1", "main", "0,0,5264,2684"), LineDraw("1", "objectives", "0,0,5264,2684"), End("1"),
        Begin("2", "selection"), LineDraw("2", "main", "0,0,5264,2684"), LineDraw("2", "objectives", "0,0,5264,2684"),
        LineDraw("2", "selection", "0,0,5264,2684"), End("2"),
        Begin("3", "objectives"), LineDraw("3", "main", "0,0,5264,2684"), LineDraw("3", "objectives", "0,0,5264,2684"), End("3"),
    };
}

int main() {
    using namespace pausecompose;
    Expect(Go(EvaluateLog(CompleteSet()).why), "three closed states on one surface are GO");

    auto missingEnd = CompleteSet();
    missingEnd.pop_back();
    Expect(EvaluateLog(missingEnd).why & MissingEnd, "missing end is rejected");

    auto noObj = CompleteSet();
    noObj[2] = LineDraw("1", "panel", "0,0,5264,2684");
    Expect(EvaluateLog(noObj).why & MissingObjectives, "root without Objectives is rejected");

    auto unknown = CompleteSet();
    unknown.insert(unknown.begin() + 2, LineDraw("1", "unknown", "0,0,5264,2684"));
    Expect(EvaluateLog(unknown).why & UnknownGroup, "unknown group is rejected");

    auto world = CompleteSet();
    world.insert(world.begin() + 2, LineDraw("1", "world", "0,0,5264,2684"));
    Expect(EvaluateLog(world).why & UnrelatedDraw, "world draw inside the transaction is rejected");

    auto split = CompleteSet();
    split[1] = "pause-compose txn=1 ev=draw token=10 surf=200 group=main vp=0,0,5264,2684 rtw=5264";
    Expect(EvaluateLog(split).why & RtSplit, "main and Objectives on different surfaces are rejected");

    auto stolen = CompleteSet();
    stolen[1] = "pause-compose txn=1 ev=draw token=99 surf=100 group=main vp=0,0,5264,2684 rtw=5264";
    Expect(EvaluateLog(stolen).why & TokenMismatch, "draw owned by another token is rejected");

    auto halves = CompleteSet();
    halves[1] = LineDraw("1", "main", "0,0,2632,2684");
    halves[2] = LineDraw("1", "objectives", "2632,0,2632,2684");
    Expect(EvaluateLog(halves).why & BakedSbs, "eye-half viewports are rejected");

    auto panel = CompleteSet();
    panel[0] = "pause-compose txn=1 ev=begin token=10 owner=3 panel=1 state=root frame=1";
    Expect(EvaluateLog(panel).why & PanelOpen, "VR Settings open contaminates the transaction");

    Expect(!Go(EvaluateLog({}).why), "an empty log is NO-GO");
    if (g_fails) return 1;
    std::puts("pause compose trace: boundaries, groups, ownership, and render targets");
    return 0;
}
