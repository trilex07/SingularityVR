#include "../dual_tmd_weapon_pose.h"
#include <cassert>
#include <cstdio>

static void Close(const armbind::Mat& a,const armbind::Mat& b,float tolerance=.001f) {
    assert(dualtmdgunpose::LargestDifference(a,b)<tolerance);
}
int main() {
    using namespace dualtmdgunpose;
    // Actual Volk body root palettes from accepted HANDS_WEAPON frame 1150
    // and failed HANDS_TMD + support frame 1364 (2026-10-25 PSVR2 trace).
    const float normal[3][4]={{.999460876f,.00218125968f,.0327707119f,-11.3739738f},
        {-.0138435578f,.932803512f,.360120386f,-43.2222519f},
        {-.0297831409f,-.360379875f,.932330132f,-61.093914f}};
    const float tmd[3][4]={{.39405477f,-.555337012f,.732340217f,-98.7063675f},
        {-.224594444f,.714472353f,.662636399f,-63.9109726f},
        {-.891223192f,-.425594538f,.156815305f,-50.9467392f}};
    const Key key{0x1BD46400u,0x3CDC3000u,8297u,2u};
    Reference r;assert(r.Observe(key,normal,1000));
    float source[kMaxBones][3][4]{};
    std::memcpy(source[2],tmd,sizeof(tmd));
    // A child bone's local firing/recoil pose must survive the same rigid
    // retarget as the root, including a nontrivial translation.
    armbind::Mat current,child,offset,childSkin;
    assert(armbind::FromGpu(tmd,current));
    offset.m[0][0]=offset.m[1][1]=offset.m[2][2]=offset.m[3][3]=1.f;
    offset.m[3][0]=3.f;offset.m[3][1]=-4.f;offset.m[3][2]=7.f;
    skeletal::Multiply(child.m,offset.m,current.m);
    armbind::ToGpu(child,source[0]);
    float target[kMaxBones][3][4]{};
    armbind::Mat delta;
    assert(r.Correct(key,source,4,2000,target,&delta));
    armbind::Mat mappedRoot,mappedChild,normalRoot;
    assert(armbind::FromGpu(target[2],mappedRoot));
    assert(armbind::FromGpu(target[0],mappedChild));
    assert(armbind::FromGpu(normal,normalRoot));
    Close(mappedRoot,normalRoot);
    armbind::Mat inv,actualLocal;
    assert(skeletal::Inverse(inv.m,mappedRoot.m));
    skeletal::Multiply(actualLocal.m,mappedChild.m,inv.m);
    Close(actualLocal,offset);
    // Never turn unused palette slots into visible geometry.
    for(const auto& row:target[1])for(float value:row)assert(value==0.f);
    // Both eye submissions must receive an identical, noncumulative palette.
    float rightEye[kMaxBones][3][4]{};
    assert(r.Correct(key,source,4,2100,rightEye));
    assert(std::memcmp(target,rightEye,sizeof(target))==0);
    Key other=key;other.weapon++;
    assert(!r.Correct(other,source,4,2200,rightEye));
    assert(!r.Correct(key,source,2,2200,rightEye));
    r.Clear();assert(!r.Correct(key,source,4,2200,rightEye));
    assert(r.Observe(key,normal,1000));
    assert(!r.Correct(key,source,4,6001,rightEye));
    std::puts("PASS dual TMD weapon body: measured normal/TMD Volk roots, common rigid delta, relative animated child, stereo identity, zero slots, ownership and freshness");
}
