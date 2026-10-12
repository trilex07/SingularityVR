#include "../pause_compose_trace.h"
#include <cstdio>
#include <fstream>
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

static void PrintVerdict(unsigned why) {
    std::printf("validator why=0x%X GO=%s\n", why, pausecompose::Go(why) ? "yes" : "no");
    if (why & pausecompose::MissingBegin) std::puts("  MissingBegin");
    if (why & pausecompose::MissingEnd) std::puts("  MissingEnd");
    if (why & pausecompose::TokenMismatch) std::puts("  TokenMismatch");
    if (why & pausecompose::MissingMain) std::puts("  MissingMain");
    if (why & pausecompose::MissingObjectives) std::puts("  MissingObjectives");
    if (why & pausecompose::UnknownGroup) std::puts("  UnknownGroup");
    if (why & pausecompose::UnrelatedDraw) std::puts("  UnrelatedDraw");
    if (why & pausecompose::RtSplit) std::puts("  RtSplit");
    if (why & pausecompose::BakedSbs) std::puts("  BakedSbs");
    if (why & pausecompose::EndNotComplete) std::puts("  EndNotComplete");
    if (why & pausecompose::PanelOpen) std::puts("  PanelOpen");
    if (why & pausecompose::Truncated) std::puts("  Truncated");
    if (why & pausecompose::MissingState) std::puts("  MissingState");
}

static std::vector<std::string> ReadLog(const char* path) {
    std::vector<std::string> lines;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        const auto at = line.find("pause-compose ");
        if (at != std::string::npos) lines.push_back(line.substr(at));
    }
    return lines;
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

int main(int argc, char** argv) {
    using namespace pausecompose;
    if (argc >= 2) {
        const auto lines = ReadLog(argv[1]);
        std::printf("pause-compose lines=%zu\n", lines.size());
        const LogVerdict v = EvaluateLog(lines);
        PrintVerdict(v.why);
        return Go(v.why) ? 0 : 1;
    }

    Expect(Go(EvaluateLog(CompleteSet()).why), "three closed states on one surface are GO");

    auto missingEnd = CompleteSet();
    missingEnd.pop_back();
    Expect(EvaluateLog(missingEnd).why & MissingEnd, "missing end is rejected");

    auto noObj = CompleteSet();
    noObj[2] = LineDraw("1", "panel", "0,0,5264,2684");
    Expect(EvaluateLog(noObj).why & MissingObjectives, "root without Objectives is rejected");

    auto world = CompleteSet();
    world.insert(world.begin() + 2, LineDraw("1", "world", "0,0,5264,2684"));
    Expect(EvaluateLog(world).why & UnrelatedDraw, "world draw inside the transaction is rejected");

    if (g_fails) return 1;
    std::puts("pause compose trace: unit checks");
    return 0;
}
