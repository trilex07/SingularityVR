#include "../settings_panel.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main(){
    const XrAction action=(XrAction)123; // OpenXR handles are uint64 on x86, pointers on x64
    unsigned calls=0;
    const auto active=[&](XrSession,const XrActionStateGetInfo* info,XrActionStateFloat* state){
        ++calls;assert(info->action==action&&info->subactionPath==XR_NULL_PATH);
        state->isActive=XR_TRUE;state->currentState=.8f;return XR_SUCCESS;
    };
    assert(settingspanel::Trigger(XR_NULL_HANDLE,action,false,active)==.8f);
    assert(settingspanel::Trigger(XR_NULL_HANDLE,action,true,active)==0);
    assert(settingspanel::Trigger(XR_NULL_HANDLE,XR_NULL_HANDLE,false,active)==0);
    assert(calls==1);
    const auto inactive=[](XrSession,const XrActionStateGetInfo*,XrActionStateFloat*){return XR_SUCCESS;};
    assert(settingspanel::Trigger(XR_NULL_HANDLE,action,false,inactive)==0);
    const auto error=[](XrSession,const XrActionStateGetInfo*,XrActionStateFloat*){return XR_ERROR_PATH_UNSUPPORTED;};
    assert(settingspanel::Trigger(XR_NULL_HANDLE,action,false,error)==0);
    for(float value:{-1.f,2.f,std::numeric_limits<float>::quiet_NaN()}){
        const auto query=[&](XrSession,const XrActionStateGetInfo*,XrActionStateFloat* state){
            state->isActive=XR_TRUE;state->currentState=value;return XR_SUCCESS;
        };
        const float expected=std::isnan(value)?0.f:std::max(0.f,std::min(1.f,value));
        assert(settingspanel::Trigger(XR_NULL_HANDLE,action,false,query)==expected);
    }
    settingspanel::Click click;
    assert(!click.Update(true,true)); // held opening R2 must not activate a row
    assert(!click.Update(true,true));
    assert(!click.Update(true,false));
    assert(click.Update(true,true));
    assert(!click.Update(true,true)); // one click, not one per held frame
    assert(!click.Update(false,true));
    assert(!click.Update(true,true));
    assert(!click.Update(true,false));
    assert(click.Update(true,true));

    unsigned hits=0;
    for(int width:{1920,3840,5376})for(int height:{1080,2160,2688})
    for(bool stereo:{false,true})for(int rows:{3,7,9,13})for(int cols:{30,35,40}){
        const auto p=settingspanel::Make(width,height,stereo,rows,cols+2,cols);
        assert(p.px>0&&p.width+8*p.px<=p.half*9/10);
        for(int row=0;row<rows;++row){
            const float eyeY=float(p.y+p.line*(2+row)+4*p.px)/float(height);
            for(float inset:{.5f,.7f,1.f}){
                const float logicalY=.5f+(eyeY-.5f)/inset;
                float x=0,y=0;
                assert(settingspanel::EyeUv(.5f,logicalY,inset,x,y));
                assert(settingspanel::RowAt(p,height,x,y)==row);
                ++hits;
            }
            const float gap=float(p.y+p.line*(2+row)+10*p.px)/float(height);
            assert(settingspanel::RowAt(p,height,.5f,gap)==-1);
        }
        assert(settingspanel::RowAt(p,height,0,.5f)==-1);
        assert(settingspanel::RowAt(p,height,1,.5f)==-1);
    }
    float x=0,y=0;
    assert(!settingspanel::EyeUv(-.01f,.5f,.7f,x,y));
    assert(!settingspanel::EyeUv(.5f,.5f,0,x,y));
    assert(!settingspanel::EyeUv(.5f,std::numeric_limits<float>::quiet_NaN(),.7f,x,y));
    assert(!settingspanel::Make(0,1080,true,7,35,35).px);
    assert(!settingspanel::Make(1920,1080,true,14,35,35).px);
    std::printf("Settings panel: aggregate XR trigger, click debounce, %u pixel/UV row hits PASS\n",hits);
}
