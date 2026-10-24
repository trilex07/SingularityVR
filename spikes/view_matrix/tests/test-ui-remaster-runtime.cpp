#include "../settings_panel.h"
#include "../ui_remaster_runtime.h"
#include "../weapon_wheel_layout.h"
#include <cassert>
#include <cstdio>
int main(){
    for(int flags=0;flags<8;++flags){
        const bool p0=(flags&1)!=0,p1=(flags&2)!=0,p2=(flags&4)!=0;
        assert(uiremaster::SettingsEnabled(p0,p1)==(p0&&p1));
        assert(uiremaster::WheelEnabled(p2)==p2);
    }
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
    const long gunVerts[4]{5799,11387,8297,11389};
    for(int i=0;i<4;++i){const auto icon=uiremaster::WheelIcon(gunVerts[i]);
        assert(uiremaster::SpriteValid(icon));
        for(int j=0;j<i;++j)assert(icon.u0!=uiremaster::WheelIcon(gunVerts[j]).u0);}
    assert(!uiremaster::SpriteValid(uiremaster::WheelIcon(0)));
    const auto m=wheel_layout::RoundMetrics(2684);
    const int pairX[2]{1316,3948},pairY[2]{1342,1342};
    uiremaster::Vertex pair[12]{};
    assert(uiremaster::BuildWheelPair(pairX,pairY,m.outerR,5264,2684,
                                      2*m.outerR+16,pair));
    assert(pair[6].x-pair[0].x==2632.f);
    assert(pair[0].u==pair[6].u&&pair[0].v==pair[6].v);
    const int invalidX[2]{1316,2632};
    assert(!uiremaster::BuildWheelPair(invalidX,pairY,m.outerR,5264,2684,
                                       2*m.outerR+16,pair));
    assert(!uiremaster::BuildWheelPair(pairX,pairY,m.outerR,5264,2684,
                                       2*m.outerR,pair));
    for(int eye=0;eye<2;++eye){const int half=5264/2,cx=eye*half+half/2;
        const uiremaster::Rect wheel{cx-m.outerR,2684/2-m.outerR,
                                      cx+m.outerR,2684/2+m.outerR};
        assert(uiremaster::Valid(wheel,eye,half,5264,2684));}
    std::puts("M02 plate geometry PASS: mono/stereo 5264x2684, bounded eyes and shared settings layout");
}
