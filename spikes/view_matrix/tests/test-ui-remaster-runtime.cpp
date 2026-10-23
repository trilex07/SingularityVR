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
        }
        if(stereo){
            const auto a=uiremaster::Plate(0,2,l.half,l.width,l.height,l.y,l.px);
            const auto b=uiremaster::Plate(1,2,l.half,l.width,l.height,l.y,l.px);
            assert(b.left-a.left==l.half&&b.right-a.right==l.half);
        }
    }
    assert(uiremaster::settingsPlate.u0>=0&&uiremaster::settingsPlate.u1<=1);
    assert(uiremaster::settingsPlate.v0>=0&&uiremaster::settingsPlate.v1<=1);
    std::puts("M02 plate geometry PASS: mono/stereo 5264x2684, bounded eyes and shared settings layout");
}
