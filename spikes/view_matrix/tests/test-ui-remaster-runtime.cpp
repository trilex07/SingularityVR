#include "../settings_panel.h"
#include "../ui_remaster_runtime.h"
#include <cassert>
#include <cstdio>
int main(){
    for(bool stereo:{false,true}){
        const auto l=settingspanel::Make(5264,2684,stereo,12,30,34);
        const int eyes=stereo?2:1;
        for(int eye=0;eye<eyes;++eye){
            const auto r=uiremaster::Plate(eye,eyes,l.half,l.width,l.height,l.y,l.px);
            assert(uiremaster::Valid(r,eye,l.half,5264,2684));
            assert(r.left<eye*l.half+l.half/2&&r.right>eye*l.half+l.half/2);
            const int cx=eye*l.half+l.half/2;
            const uiremaster::Rect header{r.left+l.px,r.top+l.px,r.right-l.px,r.top+2*l.line};
            const uiremaster::Rect tab{r.right-22*l.px,r.top+2*l.px,r.right-2*l.px,r.top+8*l.px};
            const uiremaster::Rect footer{cx-l.width/2,l.y+l.line*(l.rows+2),
                                          cx+l.width/2,l.y+l.line*(l.rows+3)};
            assert(uiremaster::Valid(header,eye,l.half,5264,2684));
            assert(uiremaster::Valid(tab,eye,l.half,5264,2684));
            assert(uiremaster::Valid(footer,eye,l.half,5264,2684));
            for(int row=0;row<l.rows;++row){
                const auto f=uiremaster::Focus(r,l.y,l.line,l.px,row);
                assert(uiremaster::Valid(f,eye,l.half,5264,2684));
                assert(f.top>=r.top&&f.bottom<=r.bottom);
            }
        }
        if(stereo){
            const auto a=uiremaster::Plate(0,2,l.half,l.width,l.height,l.y,l.px);
            const auto b=uiremaster::Plate(1,2,l.half,l.width,l.height,l.y,l.px);
            assert(b.left-a.left==l.half&&b.right-a.right==l.half);
        }
    }
    assert(uiremaster::settingsPlate.u0>=0&&uiremaster::settingsPlate.u1<=1);
    assert(uiremaster::settingsPlate.v0>=0&&uiremaster::settingsPlate.v1<=1);
    for(int page=0;page<=12;++page){const auto t=uiremaster::Tab(page);
        assert(t.u0>=0&&t.u1<=1&&t.v0>=0&&t.v1<=1&&t.u0<t.u1&&t.v0<t.v1);}
    std::puts("M02 plate geometry PASS: mono/stereo 5264x2684, bounded eyes and shared settings layout");
}
