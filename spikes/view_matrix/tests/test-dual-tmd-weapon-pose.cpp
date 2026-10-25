#include "../dual_tmd_weapon_pose.h"
#include <cassert>
#include <cstdio>

using namespace dualtmdgunpose;
static Mat Matrix(const float (&rows)[4][4]) {
    Mat m;std::memcpy(m.m,rows,sizeof(rows));return m;
}
static float Distance(const Mat& a,const Mat& b) {
    float sum=0;
    for(unsigned i=0;i<3;++i){const float d=a.m[3][i]-b.m[3][i];sum+=d*d;}
    return std::sqrt(sum);
}

int main() {
    // Captured in the failed physical PSVR2 session: normal HANDS_WEAPON
    // frame 2459 and L2/HANDS_TMD frame 2461. The previous source-root-only
    // correction placed the gun 59 UU away from this final wrist-relative pose.
    const float normalSkin[4][4]={
        {.999453723f,-.008252810f,-.032014504f,0},
        {.001834927f,.980696440f,-.195529059f,0},
        {.033010263f,.195363343f,.980175257f,0},
        {-11.425499f,-34.307083f,-45.872021f,1}};
    const float tmdSkin[4][4]={
        {.394054562f,.011824495f,-.919010878f,0},
        {-.555337191f,.799809575f,-.227827266f,0},
        {.732339621f,.600136995f,.321734995f,0},
        {-99.490959f,-53.245621f,-33.049820f,1}};
    const float localRows[4][4]={
        {.058641106f,.998279154f,0,0},
        {0,0,-1,0},
        {-.998279154f,.058641106f,0,0},
        {2.24414062f,.722167969f,-149.15625f,1}};
    const float rideRows[4][4]={
        {.19546252f,.70777678f,-.15279545f,0},
        {-.42841887f,.24063513f,.56661457f,0},
        {.58373928f,-.06038859f,.46701366f,0},
        {13.9416409f,-4.87926865f,-5.36904335f,1}};
    const float normalWristRows[4][4]={
        {-.052466807f,-.747926407f,-.018802573f,0},
        {.737238130f,-.048475069f,-.128958519f,0},
        {.127386735f,-.027504031f,.738590884f,0},
        {1.40508917f,-8.79354913f,-29.2803700f,1}};
    const Mat local=Matrix(localRows),ride=Matrix(rideRows),normalWrist=Matrix(normalWristRows);
    Mat raw;for(unsigned i=0;i<4;++i)raw.m[i][i]=1;
    const Key key{0x1BD46400u,0x3CDC3000u,8297u,2u};
    Reference reference;
    float normalGpu[3][4];armbind::ToGpu(Matrix(normalSkin),normalGpu);
    assert(reference.ObserveGun(key,normalGpu,local,ride,2459,10000));
    assert(reference.ObserveWrist(key,normalWrist,raw,2459,10001,true));
    assert(reference.valid&&reference.wristValid);
    float original[kMaxBones][3][4]{};
    armbind::ToGpu(Matrix(tmdSkin),original[key.rootSlot]);
    Mat childOffset;for(unsigned i=0;i<4;++i)childOffset.m[i][i]=1;
    childOffset.m[3][0]=3;childOffset.m[3][1]=-4;childOffset.m[3][2]=7;
    const Mat originalChild=Product(childOffset,Matrix(tmdSkin));
    armbind::ToGpu(originalChild,original[0]);
    float left[kMaxBones][3][4]{},right[kMaxBones][3][4]{};
    Mat finalGun;
    assert(reference.Correct(key,original,4,local,ride,raw,2461,10012,left,&finalGun));
    assert(reference.Correct(key,original,4,local,ride,raw,2461,10012,right));
    assert(std::memcmp(left,right,sizeof(left))==0);
    const Mat expected=Product(Product(Matrix(normalSkin),local),ride);
    assert(Distance(finalGun,expected)<.03f);
    Mat correctedRoot,correctedChild,inverseRoot;
    assert(armbind::FromGpu(left[key.rootSlot],correctedRoot));
    assert(armbind::FromGpu(left[0],correctedChild));
    assert(skeletal::Inverse(inverseRoot.m,correctedRoot.m));
    assert(LargestDifference(Product(correctedChild,inverseRoot),childOffset)<.02f);
    for(const auto& row:left[1])for(float value:row)assert(value==0);
    // Actual UE3 order in the failed session: one corrected gun body, then the
    // TMD material-0 wrist, then two more submissions of the SAME gun body in
    // frame 35921. The TMD wrist is not the normal HANDS_WEAPON gun anchor.
    Mat animatedTmdWrist=normalWrist;
    animatedTmdWrist.m[3][0]+=60.f;
    assert(reference.ObserveWrist(key,animatedTmdWrist,raw,2461,10046,false));
    Mat sameFrameGun;
    assert(reference.Correct(key,original,4,local,ride,raw,2461,10062,right,&sameFrameGun));
    assert(Distance(sameFrameGun,expected)<.03f);
    assert(std::memcmp(left,right,sizeof(left))==0);
    assert(reference.Correct(key,original,4,local,ride,raw,2462,10080,right,&sameFrameGun));
    assert(Distance(sameFrameGun,expected)<.03f);
    // Repeated stereo-confirmed TMD frames may last longer than five seconds.
    // They refresh tracking evidence, not the normal gun/wrist anchor.
    for(unsigned frame=2462;frame<=3008;++frame)
        assert(reference.ObserveWrist(key,animatedTmdWrist,raw,frame,
                                      10080+(frame-2462)*11,false));
    const auto longNow=10080+(3008-2462)*11;
    assert(reference.Correct(key,original,4,local,ride,raw,3008,longNow+1,right,&sameFrameGun));
    assert(Distance(sameFrameGun,expected)<.03f);
    assert(!reference.Correct(key,original,4,local,ride,raw,3011,longNow+2,right));
    assert(!reference.Correct(key,original,4,local,ride,raw,3008,longNow+501,right));
    Key other=key;++other.weapon;
    assert(!reference.Correct(other,original,4,local,ride,raw,3008,longNow+1,right));
    assert(!reference.Correct(key,original,2,local,ride,raw,3008,longNow+1,right));
    reference.Clear();assert(!reference.Correct(key,original,4,local,ride,raw,3008,longNow+1,right));
    std::puts("PASS dual TMD gun: normal gun anchor survives animated TMD wrist, repeated same-frame draws, long L2 hold, child/recoil pose, stereo, freshness and identity");
}
