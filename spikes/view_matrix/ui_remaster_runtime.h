#pragma once
#include <cmath>

// M02 first slice: the verified neutral atlas' settings_console_plate sprite.
// Geometry is derived from the SAME settingspanel::Layout used for text/hits.
namespace uiremaster {
struct Rect {int left,top,right,bottom;};
struct Sprite {float u0,v0,u1,v1;};
struct Vertex {float x,y,z,rhw,u,v;};
constexpr Sprite settingsPlate{4.f/4096.f,4.f/2048.f,1204.f/4096.f,844.f/2048.f};
inline Rect Plate(int eye,int eyes,int half,int width,int height,int y,int px){
    if(eyes<1||eyes>2||eye<0||eye>=eyes||half<=0||width<=0||height<=0||px<=0)return {};
    const int centre=eye*half+half/2;
    return {centre-width/2-4*px,y-4*px,centre+width/2+4*px,y+height+4*px};
}
inline bool Valid(Rect r,int eye,int half,int backW,int backH){
    return half>0&&eye>=0&&r.left>=eye*half&&r.right<=(eye+1)*half&&
        r.top>=0&&r.bottom<=backH&&r.left<r.right&&r.top<r.bottom&&r.right<=backW;
}
inline void BuildQuad(Rect q,Sprite uv,Vertex (&v)[4]){
    v[0]={q.left-.5f,q.top-.5f,0,1,uv.u0,uv.v0};
    v[1]={q.right-.5f,q.top-.5f,0,1,uv.u1,uv.v0};
    v[2]={q.left-.5f,q.bottom-.5f,0,1,uv.u0,uv.v1};
    v[3]={q.right-.5f,q.bottom-.5f,0,1,uv.u1,uv.v1};
}
}
