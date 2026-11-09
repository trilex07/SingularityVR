#pragma once
#include <cstring>
#include <cstdio>

// Physical controller indices stay stable. Only gameplay roles are exchanged;
// locomotion axes and the two-stick recenter chord remain independent.
namespace handedness {
inline int Aim(bool left){return left?0:1;}
inline int Off(bool left){return 1-Aim(left);}
inline bool WheelButton(bool left,bool physicalL3,bool physicalR3){return left?physicalL3:physicalR3;}
inline bool WheelVeto(bool left,bool physicalL3,bool physicalR3){return left?physicalR3:physicalL3;}
inline unsigned Wrist(int side){return side==0?9u:51u;}
inline const char* WristName(int side){return side==0?"l_arm_wrist":"r_arm_wrist";}
inline bool MirrorGameplayButton(const char* path){
    // Face/menu actions keep their physical names: Triangle remains pause,
    // Circle remains reload/back. Mirroring them aliases reload with pause.
    return path&&(std::strstr(path,"/input/trigger/")||
        std::strstr(path,"/input/squeeze/")||std::strstr(path,"/input/thumbstick/"));
}
inline const char* MirrorPath(bool left,const char* path,char* out,size_t capacity){
    if(!left||!path||std::strstr(path,"/input/menu/click"))return path;
    struct Swap {const char* from;const char* to;};
    static const Swap faces[]={
        {"/user/hand/right/input/a/","/user/hand/left/input/x/"},
        {"/user/hand/right/input/b/","/user/hand/left/input/y/"},
        {"/user/hand/left/input/x/","/user/hand/right/input/a/"},
        {"/user/hand/left/input/y/","/user/hand/right/input/b/"},
        {"/user/hand/left/input/","/user/hand/right/input/"},
        {"/user/hand/right/input/","/user/hand/left/input/"}};
    for(const auto& swap:faces){
        const auto n=std::strlen(swap.from);
        if(std::strncmp(path,swap.from,n))continue;
        const auto needed=std::strlen(swap.to)+std::strlen(path+n)+1;
        if(!out||needed>capacity)return path; // never suggest a truncated XR path
        std::snprintf(out,capacity,"%s%s",swap.to,path+n);return out;
    }
    return path;
}
}
