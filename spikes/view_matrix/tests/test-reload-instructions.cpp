#include "../vr_tutorial.h"
#include "../prompt_tokens.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
int main(){
    const unsigned meshes[]{5799,11387,8297,11389};
    const char* names[]{"CENTURION","VALKYRIE","VOLK S4","KASIMOV SNV-E99"};
    for(auto language:{vrtutorial::Language::Polish,vrtutorial::Language::English}){
        for(unsigned i=0;i<4;++i){
            const int page=vrtutorial::ReloadStep(meshes[i]);
            assert(page>=0&&page<vrtutorial::kStepCount);
            const auto& s=vrtutorial::GetStep(language,page);
            assert(!std::strcmp(s.title,names[i]));
            assert(std::strstr(s.line[0]," O," )||std::strstr(s.line[0]," O TO"));
            for(const char* line:s.line)assert(line&&*line&&std::strlen(line)<96);
        }
    }
    assert(vrtutorial::ReloadStep(0)==-1&&vrtutorial::ReloadStep(3314)==-1);
    char out[256];
    auto r=prompttokens::Rewrite("[use] [jump] [reload] [health]",out,sizeof(out),true,true,false);
    assert(r.changed&&!r.overflow&&!std::strcmp(out,"RIGHT GRIP RIGHT STICK UP (O) (X)"));
    r=prompttokens::Rewrite("[use] [reload]",out,sizeof(out),false,false,false);
    assert(r.changed&&!std::strcmp(out,"LEFT GRIP LEFT GRIP"));
    r=prompttokens::Rewrite("Pad 3 [unknown]",out,sizeof(out),true,true,false);
    assert(!r.changed&&!std::strcmp(out,"Pad 3 [unknown]"));
    r=prompttokens::Rewrite("[reload]",out,2,true,true,false);
    assert(r.overflow&&!r.changed&&out[0]==0);
    std::puts("PASS four weapon recipes PL/EN, unknown refusal, English tokens and overflow; NOT native prompt proof");
}
