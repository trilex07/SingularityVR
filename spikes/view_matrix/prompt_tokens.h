#pragma once
#include <cstddef>
#include <cstring>
namespace prompttokens {
// Replace native action TOKENS before UE3 resolves them to legacy pad labels.
// Preserve the sentence, markup and unknown tokens. No pad-number guessing.
struct Result {bool changed=false,overflow=false;unsigned replaced=0;};
inline const char* Label(const char* token,std::size_t n,bool layout,bool right,bool polish=true){
    const auto is=[&](const char* name){return std::strlen(name)==n&&!std::memcmp(token,name,n);};
    const char* grip=polish?(right?"PRAWY CHWYT":"LEWY CHWYT"):(right?"RIGHT GRIP":"LEFT GRIP");
    if(is("use"))return grip;
    if(is("reload"))return layout?"(O)":grip;
    if(layout&&is("health"))return "(X)";
    if(layout&&is("jump"))return polish?"PRAWA GALKA W GORE":"RIGHT STICK UP";
    return nullptr;
}
inline Result Rewrite(const char* input,char* out,std::size_t capacity,bool layout,bool right,bool polish=true){
    Result r;if(!input||!out||!capacity)return r;
    std::size_t at=0;
    for(std::size_t i=0;input[i];){
        const char* text=input+i;std::size_t length=1,consumed=1;
        if(input[i]=='['){
            std::size_t end=i+1;while(input[end]&&input[end]!=']'&&end-i<=24)++end;
            if(input[end]==']')if(const char* label=Label(input+i+1,end-i-1,layout,right,polish)){
                text=label;length=std::strlen(label);consumed=end-i+1;r.changed=true;++r.replaced;
            }
        }
        if(length>=capacity-at){out[0]=0;r.overflow=true;r.changed=false;return r;}
        std::memcpy(out+at,text,length);at+=length;i+=consumed;
    }
    out[at]=0;return r;
}
}
