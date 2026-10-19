#pragma once
#include "ui_pointer.h"
#include <openxr/openxr.h>
namespace settingspanel {
// fire/altfire have no subaction paths. Match gameplay's aggregate query.
template<class Query>
inline float Trigger(XrSession session,XrAction action,bool unavailable,Query query){
    if(unavailable||action==XR_NULL_HANDLE)return 0;
    XrActionStateGetInfo info{XR_TYPE_ACTION_STATE_GET_INFO};info.action=action;
    XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
    if(XR_FAILED(query(session,&info,&state))||!state.isActive||!std::isfinite(state.currentState))return 0;
    return std::max(0.f,std::min(1.f,state.currentState));
}
struct Click {
    bool wasOpen=false,armed=false;
    bool Update(bool open,bool held){
        if(!open){wasOpen=false;armed=false;return false;}
        if(!wasOpen){wasOpen=true;armed=!held;return false;}
        if(!held){armed=true;return false;}
        if(!armed)return false;
        armed=false;return true;
    }
};
struct Layout { int px=0,line=0,width=0,height=0,y=0,half=0,rows=0; };
inline Layout Make(int width,int height,bool stereo,int rows,int widest,int minimum){
    Layout p;
    if(width<8||height<8||width>32768||height>32768||rows<1||rows>13||widest<0||widest>95||minimum<1||minimum>97)return p;
    const int half=stereo?width/2:width,cols=std::max(widest+2,minimum);
    int px=std::max(3,int(height*.28/(13*16)));
    while(px>1&&(cols*6+8)*px>half*9/10)--px;
    p={px,13*px,cols*6*px,13*px*(rows+3),height/2-13*px*(rows+3)/2,half,rows};
    return p;
}
inline bool EyeUv(float u,float v,float inset,float& x,float& y){
    if(!std::isfinite(u)||!std::isfinite(v)||!std::isfinite(inset)||u<0||u>1||v<0||v>1||inset<=0||inset>1)return false;
    x=uipointer::CanvasToEye(u,inset);y=uipointer::CanvasToEye(v,inset);return true;
}
inline int RowAt(const Layout& p,int height,float eyeU,float eyeV){
    if(!p.px||!std::isfinite(eyeU)||!std::isfinite(eyeV)||eyeU<0||eyeU>1||eyeV<0||eyeV>1)return -1;
    const float x=eyeU*p.half,y=eyeV*height;
    if(x<p.half*.5f-p.width*.5f||x>p.half*.5f+p.width*.5f)return -1;
    for(int i=0;i<p.rows;++i){const int top=p.y+p.line*(2+i);if(y>=top&&y<=top+8*p.px)return i;}
    return -1;
}
}
