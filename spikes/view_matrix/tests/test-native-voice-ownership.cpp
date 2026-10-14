#include "../polish_localization.h"
#include <fstream>
#include <cassert>
#include <cstdio>
#include <cstring>
int main(){
    // Run only in an owned private test directory; synthetic lines, no game data.
    std::ofstream("text.jsonl")<<"{\"id\":\"fixture\",\"en\":\"UI fixture\",\"pl\":\"PL fixture\",\"status\":\"translated\"}\n";
    std::ofstream("voice.jsonl")<<"{\"id\":\"ui\",\"en\":\"UI fixture\",\"file\":\"ui.wav\"}\n";
    std::ofstream("story.jsonl")<<"{\"id\":\"native\",\"text_en\":\"Radio fixture\",\"text_pl\":\"Radio PL\",\"file\":\"native.wav\",\"native_replaced\":\"1\"}\n"
        <<"{\"id\":\"legacy\",\"text_en\":\"Uncovered fixture\",\"text_pl\":\"Uncovered PL\",\"file\":\"legacy.wav\"}\n";
    auto& pl=PolishLocalization::PL();assert(pl.Initialize("text.jsonl","voice.jsonl","story.jsonl"));pl.SetPolishLanguage(true);
    char out[128];assert(pl.WavForEnglish("Radio fixture",out,sizeof(out))); // flag OFF leaves old route
    pl.SetNativeSpeech(true);assert(!pl.WavForEnglish("Radio fixture",out,sizeof(out))&&out[0]==0);
    assert(pl.PolishForEnglish("Radio fixture",out,sizeof(out))&&!strcmp(out,"Radio PL"));
    assert(pl.WavForEnglish("Uncovered fixture",out,sizeof(out))&&!strcmp(out,"legacy.wav"));
    assert(pl.WavForEnglish("UI fixture",out,sizeof(out))&&!strcmp(out,"ui.wav"));
    pl.SetNativeSpeech(false);assert(pl.WavForEnglish("Radio fixture",out,sizeof(out)));
    pl.Shutdown();puts("PASS: default-off/native exact-line voice ownership, text preserved, uncovered/UI audio untouched");
}
