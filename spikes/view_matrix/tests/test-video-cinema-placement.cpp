#include "../video_cinema_placement.h"
#include <cassert>
#include <cmath>
#include <cstdio>
static bool near(float a,float b){return std::fabs(a-b)<0.00001f;}
int main(){
    for(int eye=0;eye<2;++eye){
        float x=.907f,y=1.f; PlaceVideoInEye(x,y,eye,100);
        const float centre=eye==0?-.5f:.5f;
        assert(near(x,centre+.4535f)&&near(y,.5f)); // legacy unchanged at 100
        float sx=.907f,sy=1.f; PlaceVideoInEye(sx,sy,eye,50);
        assert(near(sx-centre,(x-centre)*.5f)&&near(sy,y*.5f));
        assert(near((sx-centre)/sy,(x-centre)/y)); // aspect unchanged
        float cx=0,cy=0; PlaceVideoInEye(cx,cy,eye,50);
        assert(near(cx,centre)&&near(cy,0)); // no lateral/vertical drift
    }
    assert(ClampVideoCinemaScale(-1)==10&&ClampVideoCinemaScale(1000)==100);
    float left=-1,ly=-1,right=1,ry=1;
    PlaceVideoInEye(left,ly,0,50); PlaceVideoInEye(right,ry,1,50);
    assert(left>=-1&&right<=1&&near(ly,-.25f)&&near(ry,.25f));
    std::puts("PASS video 50% width/height, same aspect and per-eye centres; 100% legacy control");
}
