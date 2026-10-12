#include "../pause_canvas_trace_validate.h"
#include <cstdio>
#include <string>
#include <vector>

static int g_fails = 0;

static void Expect(bool ok, const char* name) {
    if (ok) std::printf("PASS %s\n", name);
    else { std::printf("FAIL %s\n", name); ++g_fails; }
}

static void PrintSummary(const pausecanvas::validate::Summary& s) {
    std::printf("canvas validator why=0x%X GO=%s txns=%u settledPass=%u sameSurf=%u surf=%llX\n",
                s.why, pausecanvas::validate::Go(s.why) ? "yes" : "no", s.txnCount, s.settledPass,
                s.sameSurfSettled, (unsigned long long)s.commonSurf);
}

int main(int argc, char** argv) {
    using namespace pausecanvas::validate;
    const std::vector<std::string> settledOk = {
        "pause-canvas txn=1 ev=begin token=3 scope=backbuffer_canvas panel=0 frame=1",
        "pause-canvas txn=1 ev=draw band=A sx=7.812e-05",
        "pause-canvas txn=1 ev=draw band=B sx=1.249e-05",
        "pause-canvas txn=1 ev=end token=3 reason=complete bandA=2 bandB=1 mid=0 notOrtho=0 settled=1"
        " boundsA_tx=[-0.5,0.1] ty=[0.5,0.6] boundsB_tx=[-0.4,0.2] ty=[0.5,0.55]",
    };
    PrintSummary(EvaluateLog(settledOk));
    Expect(Go(EvaluateLog(settledOk).why), "settled txn with A+B passes");

    const std::vector<std::string> unsettledOnly = {
        "pause-canvas txn=1 ev=begin token=3 scope=backbuffer_canvas panel=0 frame=1",
        "pause-canvas txn=1 ev=end token=3 reason=complete bandA=3 bandB=0 mid=0 notOrtho=0 settled=0",
    };
    Expect(EvaluateLog(unsettledOnly).why & MissingSettled, "unsettled-only log is NO-GO");

    if (argc >= 2) {
        std::vector<std::string> lines;
        // minimal file read without fstream for test exe size - use fopen
        FILE* f = fopen(argv[1], "r");
        if (!f) return 2;
        char buf[4096];
        while (fgets(buf, sizeof(buf), f)) {
            const char* p = std::strstr(buf, "pause-canvas txn=");
            if (p) lines.emplace_back(p);
        }
        fclose(f);
        PrintSummary(EvaluateLog(lines));
        return Go(EvaluateLog(lines).why) ? 0 : 1;
    }

    return g_fails ? 1 : 0;
}
