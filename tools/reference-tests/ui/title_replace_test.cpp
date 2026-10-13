// Tests the exact-match game-title mapping for the Scaleform rewrite.
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include "../../../spikes/view_matrix/title_replace.h"

using namespace titlereplace;

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}

static bool Streq(const wchar_t* a, const wchar_t* b) {
    if (!a || !b) return a == b;
    return std::wcscmp(a, b) == 0;
}

int main() {
    Check(Streq(TitleForText(L"SINGULARITY"), L"SINGULARITY VR"), "menu title -> SINGULARITY VR");
    Check(Streq(TitleForText(L"Singularity"), L"Singularity VR"), "mixed case -> Singularity VR");
    Check(TitleForText(L"") == nullptr, "empty stays untouched");
    Check(TitleForText(nullptr) == nullptr, "null stays untouched");
    // near-misses that must NEVER rewrite (objectives, locations, identifiers)
    Check(TitleForText(L"Singularity Labs, Katorga-12") == nullptr, "location untouched");
    Check(TitleForText(L"Kill Barisov or use the TMD on the Singularity") == nullptr, "objective untouched");
    Check(TitleForText(L"singularity") == nullptr, "wrong case untouched");
    Check(TitleForText(L"SINGULARITY ") == nullptr, "trailing space untouched");
    Check(TitleForText(L" SINGULARITY") == nullptr, "leading space untouched");
    Check(TitleForText(L"Completed Singularity") == nullptr, "achievement untouched");
    Check(TitleForText(L"Singularity (Running)") == nullptr, "process title untouched");
    Check(TitleForText(L"RvGame.RvUIDataStore_SingularityObjectives") == nullptr, "class name untouched");
    if (g_fail) { std::fprintf(stderr, "title-replace tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: exact-match game-title mapping");
    return 0;
}
