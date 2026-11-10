#include "../dual_hand_action_pose.h"
#include "../arm_natural.h"
#include <cstdio>
#include <type_traits>

// Compile the exact same fixture against the baseline and corrected production
// header. Missing opt-in on the baseline is a runtime negative, not build failure.
template<class T,class=void> struct OptIn {static void Set(T&,bool) {}};
template<class T> struct OptIn<T,std::void_t<decltype(T::supportContinuity)>> {
    static void Set(T& c,bool on){c.supportContinuity=on;}
};
static unsigned checks=0,failures=0;
static void Check(bool ok,const char* label){++checks;if(!ok){++failures;std::printf("FAIL %s\n",label);}}
static armbind::Mat Identity(){armbind::Mat m{};for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
int main(){
    dualaction::Context c{true,true,true,true,false,false,false,false,true};
    Check(dualaction::Eligible(c),"free tracked context admitted");
    c.support=true;Check(!dualaction::Eligible(c),"default support behavior retained");
    OptIn<dualaction::Context>::Set(c,true);
    Check(dualaction::Eligible(c),"opt-in right wrist remains admitted across support edge");
    Check(dualaction::SideEligible(true,true,true),"right support correction admitted");
    Check(!dualaction::SideEligible(true,true,false),"left remains support-owned");
    Check(dualaction::SideEligible(false,true,false),"free left unchanged");
    Check(dualaction::SideEligible(true,false,false),"flag OFF side behavior unchanged");
    auto blocked=c;blocked.tracked=false;Check(!dualaction::Eligible(blocked),"tracking remains mandatory");
    blocked=c;blocked.carrying=true;Check(!dualaction::Eligible(blocked),"carry still refused");
    blocked=c;blocked.nativeAnimation=true;Check(!dualaction::Eligible(blocked),"native animation still refused");
    blocked=c;blocked.gameplay=false;Check(!dualaction::Eligible(blocked),"menu still refused");
    blocked=c;blocked.holstered=true;Check(!dualaction::Eligible(blocked),"holster still refused");
    blocked=c;blocked.rightHanded=false;Check(!dualaction::Eligible(blocked),"unsupported handedness refused");
    for(unsigned mask=0;mask<512;++mask){
        dualaction::Context x{bool(mask&1),bool(mask&2),bool(mask&4),bool(mask&8),
            bool(mask&16),bool(mask&32),bool(mask&64),bool(mask&128),bool(mask&256)};
        const bool legacy=x.enabled&&x.dual&&x.tracked&&x.gameplay&&!x.nativeAnimation&&
            !x.holstered&&!x.support&&!x.carrying&&x.rightHanded;
        Check(dualaction::Eligible(x)==legacy,"all flag-OFF contexts exactly preserve baseline");
    }
    const auto& bind=armnatural::TmdBind();Check(bind.ready,"actual native bind ready");
    for(unsigned vertices:{5799u,11387u,8297u,11389u}){
        auto local=Identity(),controller=Identity();controller.m[3][0]=11;controller.m[3][1]=17;
        auto transition=Identity();transition.m[3][0]=23;transition.m[3][1]=-9;
        const auto left=dualhandreference::Left,right=dualaction::Product(*dualhandreference::Right(vertices),transition);
        armbind::Mat inverse;float skin[2][3][4];
        skeletal::Inverse(inverse.m,bind.bind[9].m);armbind::ToGpu(dualaction::Product(inverse,left),skin[0]);
        skeletal::Inverse(inverse.m,bind.bind[51].m);armbind::ToGpu(dualaction::Product(inverse,right),skin[1]);
        dualaction::Frame frame;dualaction::Key key{18,1,2,vertices};
        Check(frame.Capture(key,local,bind.bind[9],skin[0],bind.bind[51],skin[1]),"original frame captured");
        auto before=controller,after=controller;
        auto free=c;free.support=false;
        if(dualaction::Eligible(free))frame.Apply(key,true,before.m);
        if(dualaction::Eligible(c))frame.Apply(key,true,after.m);
        float error=0;for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)error=std::fmax(error,std::fabs(before.m[i][j]-after.m[i][j]));
        Check(error<.0001f,"same current right correction before and after support");
        Check(!frame.Apply({19,1,2,vertices},true,after.m),"cross-frame rejected");
        Check(!frame.Apply({18,1,3,vertices},true,after.m),"replacement weapon rejected");
    }
    std::printf("tracked-support continuity: %u checks %u failures\n",checks,failures);
    return failures?1:0;
}
