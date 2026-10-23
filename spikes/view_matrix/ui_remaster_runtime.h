#pragma once
#include <cmath>

// M02 first slice: the verified neutral atlas' settings_console_plate sprite.
// Geometry is derived from the SAME settingspanel::Layout used for text/hits.
namespace uiremaster {
struct Rect {int left,top,right,bottom;};
struct Sprite {float u0,v0,u1,v1;};
struct Vertex {float x,y,z,rhw,u,v;};
constexpr Sprite Atlas(int x,int y,int w,int h,int atlasW,int atlasH){
    return {(float)x/atlasW,(float)y/atlasH,(float)(x+w)/atlasW,(float)(y+h)/atlasH};
}
constexpr Sprite settingsPlate{4.f/4096.f,4.f/2048.f,1204.f/4096.f,844.f/2048.f};
constexpr Sprite focus=Atlas(1796,1172,1120,48,4096,2048);
constexpr Sprite header=Atlas(3040,852,560,90,4096,2048);
constexpr Sprite senseR2=Atlas(1152,1100,64,64,4096,2048);
constexpr Sprite senseOptions=Atlas(1008,1100,64,64,4096,2048);
constexpr Sprite senseCircle=Atlas(720,1100,64,64,4096,2048);
constexpr Sprite footer=Atlas(300,638,900,32,2048,1024);
constexpr Sprite wheelChassis=Atlas(1900,4,800,800,4096,2048);
constexpr Sprite wheelHub=Atlas(652,4,260,260,2048,1024);
constexpr Sprite wheelSelected=Atlas(1944,1100,64,64,4096,2048);
constexpr Sprite WheelIcon(long verts){
    switch(verts){
        case 5799:return Atlas(1976,852,240,110,4096,2048);
        case 11387:return Atlas(2224,852,240,110,4096,2048);
        case 8297:return Atlas(2688,852,240,107,4096,2048);
        case 11389:return Atlas(3608,852,240,86,4096,2048);
        default:return {};
    }
}
inline bool SpriteValid(Sprite s){return s.u0>=0&&s.v0>=0&&s.u1<=1&&s.v1<=1&&s.u0<s.u1&&s.v0<s.v1;}
constexpr Sprite Tab(int page){
    switch(page){
        case 0:return Atlas(1580,578,140,36,2048,1024); // general
        case 1:return Atlas(1284,578,140,36,2048,1024); // controller
        case 2:return Atlas(1136,578,140,36,2048,1024); // comfort
        case 3:case 12:return Atlas(1432,578,140,36,2048,1024); // display
        case 4:return Atlas(988,578,140,36,2048,1024); // advanced
        case 5:return Atlas(152,638,140,36,2048,1024); // weapon
        case 6:case 7:case 8:return Atlas(1876,578,140,36,2048,1024); // TMD
        case 9:return Atlas(1728,578,140,36,2048,1024); // hands
        default:return Atlas(4,638,140,36,2048,1024); // tutorial
    }
}
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
inline Rect Focus(const Rect& plate,int y,int line,int px,int row){
    const int top=y+line*(2+row)-px;
    return {plate.left+2*px,top,plate.right-2*px,top+8*px};
}
}
