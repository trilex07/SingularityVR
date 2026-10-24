#pragma once
#include <cstring>
#include <cstddef>
namespace vrprompts {
enum Action {Use,Reload,Health,Jump,Fire,Aim,Age,Impulse,Dash,Cycle,Crouch,Count,Unknown=Count};
enum Control {Unbound,LeftGrip,RightGrip,L2,R2,Square,Triangle,Cross,Circle,L3,R3,RightUp,RightDown};
inline Action Token(const char* s,size_t n){
    struct Entry{const char* name;Action action;};
    static const Entry entries[]={{"use",Use},{"reload",Reload},{"health",Health},{"jump",Jump},
        {"fire",Fire},{"aim",Aim},{"age",Age},{"ageyoung",Age},{"impulse",Impulse},
        {"dash",Dash},{"cycleweapon",Cycle},{"crouch",Crouch}};
    for(const auto& e:entries)if(strlen(e.name)==n&&!memcmp(s,e.name,n))return e.action;
    return Unknown;
}
// Actual bound-source path, not a DirectInput pad-number translation.
inline Control Path(const char* p){
    if(!p)return Unbound;
    const bool left=strstr(p,"/user/hand/left/")!=nullptr;
    const bool right=strstr(p,"/user/hand/right/")!=nullptr;
    if(!left&&!right)return Unbound;
    if(strstr(p,"/input/squeeze/"))return left?LeftGrip:RightGrip;
    if(strstr(p,"/input/trigger/"))return left?L2:R2;
    if(strstr(p,"/input/thumbstick/click"))return left?L3:R3;
    if(left&&strstr(p,"/input/x/click"))return Square;
    if(left&&strstr(p,"/input/y/click"))return Triangle;
    if(right&&strstr(p,"/input/a/click"))return Cross;
    if(right&&strstr(p,"/input/b/click"))return Circle;
    return Unbound;
}
inline const char* Label(Control c,bool pl){
    switch(c){
    case LeftGrip:return pl?"LEWY CHWYT":"LEFT GRIP";
    case RightGrip:return pl?"PRAWY CHWYT":"RIGHT GRIP";
    case L2:return "L2";case R2:return "R2";
    case Square:return pl?"KWADRAT":"SQUARE";case Triangle:return pl?"TROJKAT":"TRIANGLE";
    case Cross:return "(X)";case Circle:return "(O)";
    case L3:return "L3";case R3:return "R3";
    case RightUp:return pl?"PRAWA GALKA W GORE":"RIGHT STICK UP";
    case RightDown:return pl?"PRAWA GALKA W DOL":"RIGHT STICK DOWN";
    default:return "VR"; // known action, unresolved profile: never invent a button
    }
}
struct Result{bool changed=false,overflow=false;};
inline bool Digits(const char* p){if(!p||!*p)return false;for(;*p;++p)if(*p<'0'||*p>'9')return false;return true;}
// Only action tokens, or literal Pad N inside an EXACT Tutorial action key.
// Never deletes a sentence, a GFx draw, or arbitrary text containing 'Pad'.
inline Result Rewrite(const char* text,char* out,size_t cap,const Control (&controls)[Count],
                      bool pl,const char* section=nullptr,const char* key=nullptr){
    Result r;if(!text||!out||!cap)return r;size_t at=0;
    // Observed native producer: Localize(Keyboard, Button2/10, RvGame).
    // Empty only an exact controller-label field. No action identity can be
    // inferred from this alias; action tokens are handled separately below.
    if(section&&key&&!strcmp(section,"Keyboard")&&!strncmp(key,"Button",6)&&Digits(key+6)&&
       !strncmp(text,"Pad ",4)&&Digits(text+4)){out[0]=0;return {true,false};}
    const bool protectedText=section&&(!strcmp(section,"Notes")||!strcmp(section,"ChronoNote")||
        !strcmp(section,"Subtitles")||!strcmp(section,"Objectives"));
    Action context=Unknown;
    if(section&&key&&!strcmp(section,"Tutorial")){
        if(!strcmp(key,"Reload"))context=Reload;
        else if(!strcmp(key,"Jump"))context=Jump;
        else if(!strcmp(key,"Crouch"))context=Crouch;
    }
    for(size_t i=0;text[i];){
        const char* src=text+i;size_t len=1,used=1;Action action=Unknown;
        if(!protectedText&&text[i]=='['){size_t end=i+1;while(text[end]&&text[end]!=']'&&end-i<=24)++end;
            if(text[end]==']'){action=Token(text+i+1,end-i-1);if(action!=Unknown)used=end-i+1;}}
        if(context!=Unknown&&!strncmp(text+i,"Pad",3)&&(i==0||text[i-1]==' ')){
            size_t end=i+3;if(text[end]==' ')++end;size_t first=end;
            while(text[end]>='0'&&text[end]<='9')++end;
            if(end>first&&(text[end]==0||text[end]==' '||text[end]=='.'||text[end]==',')){action=context;used=end-i;}}
        if(action!=Unknown){src=Label(controls[action],pl);len=strlen(src);r.changed=true;}
        if(len>=cap-at){out[0]=0;return {false,true};}
        memcpy(out+at,src,len);at+=len;i+=used;
    }
    out[at]=0;return r;
}
}
