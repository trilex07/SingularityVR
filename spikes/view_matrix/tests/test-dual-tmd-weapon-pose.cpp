#include "../dual_tmd_weapon_pose.h"
#include <cassert>
#include <cstdio>
#include <limits>

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
    CorrectGate gate=CorrectGate::Reference;
    assert(reference.Correct(key,original,4,local,ride,raw,2461,10012,left,&finalGun,&gate));
    assert(gate==CorrectGate::Pass);
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
    assert(!reference.Correct(key,original,4,local,ride,raw,3011,longNow+2,right,nullptr,&gate));
    assert(gate==CorrectGate::Frame);
    assert(!reference.Correct(key,original,4,local,ride,raw,3008,longNow+501,right,nullptr,&gate));
    assert(gate==CorrectGate::Frame);
    // PSVR2 frame 1293 had a valid right-wrist witness from frame 1290:
    // the gun was forwarded with the head pose. The next fully drawn TMD
    // glove must rearm tracking instead of keeping the old three-frame gap.
    assert(reference.ObserveWrist(key,animatedTmdWrist,raw,3011,longNow+501,false));
    assert(reference.wristFrame==3011&&reference.wristTick==longNow+501);
    assert(reference.Correct(key,original,4,local,ride,raw,3012,longNow+502,right,&sameFrameGun,&gate));
    assert(gate==CorrectGate::Pass&&Distance(sameFrameGun,expected)<.03f);
    assert(!reference.ObserveWrist(key,animatedTmdWrist,raw,3010,longNow+503,false));
    assert(!reference.ObserveWrist(key,animatedTmdWrist,raw,3012,longNow+500,false));
    assert(reference.wristFrame==3011&&reference.wristTick==longNow+501);
    // Controller translation/rotation after L2 must move every point of the
    // gun by that SAME world delta. At the unchanged reference pose both
    // multiplication orders cancel, which concealed the old defect.
    Mat moved=raw;
    moved.m[3][0]=20;moved.m[3][1]=-7;moved.m[3][2]=4;
    assert(reference.Correct(key,original,4,local,ride,moved,3012,longNow+502,right,&sameFrameGun));
    assert(LargestDifference(sameFrameGun,Product(expected,moved))<.03f);
    moved.m[0][0]=0;moved.m[0][1]=1;moved.m[1][0]=-1;moved.m[1][1]=0;
    assert(reference.Correct(key,original,4,local,ride,moved,3012,longNow+502,right,&sameFrameGun));
    assert(LargestDifference(sameFrameGun,Product(expected,moved))<.03f);
    Mat inverseWrist;assert(skeletal::Inverse(inverseWrist.m,normalWrist.m));
    const auto historical=Product(Product(normalWrist,moved),Product(inverseWrist,expected));
    const float oldError=Distance(historical,Product(expected,moved));
    assert(oldError>1.f); // old order must demonstrably fail this same fixture
    std::printf("row-vector motion regression: old position error %.4f UU; corrected %.6f UU\n",
        oldError,Distance(sameFrameGun,Product(expected,moved)));
    // Native camera/mesh tail changes must cancel out, not own the weapon.
    Mat cameraLocal=local;cameraLocal.m[3][0]+=11;cameraLocal.m[3][2]-=9;
    assert(reference.Correct(key,original,4,cameraLocal,ride,moved,3012,longNow+502,right,&sameFrameGun));
    assert(LargestDifference(sameFrameGun,Product(expected,moved))<.03f);

    // Actual CB43F6DC PSVR2 refusal, frame 7607. Rotation's pivot
    // compensation was 258.909 UU, although the root moved only 211.930 UU.
    // The coefficient-based guard fell back to the native/head-owned gun.
    {
        const float capturedRootRows[4][4]={
            {0.3915916f,-0.07211018f,-0.9173095f,0.f},
            {-0.5570611f,0.7748869f,-0.2987183f,0.f},
            {0.7323517f,0.6279724f,0.2632686f,0.f},
            {-98.98649f,-57.06403f,-39.52105f,1.f},
        };
        const Mat capturedRoot=Matrix(capturedRootRows);
        const float capturedLocalRows[4][4]={
            {-0.6800389f,0.7331761f,0.f,0.f},
            {-8.978521e-17f,-8.327799e-17f,-1.f,0.f},
            {-0.7331761f,-0.6800389f,1.224606e-16f,0.f},
            {3.085938f,2.861816f,-148.75f,1.f},
        };
        const Mat capturedLocal=Matrix(capturedLocalRows);
        const float capturedRideRows[4][4]={
            {-0.7398694f,-0.097303f,0.07500491f,0.f},
            {-0.09585835f,0.7436037f,0.01909769f,0.f},
            {-0.07684296f,0.009252965f,-0.7459959f,0.f},
            {3.528355f,16.89458f,-33.20248f,1.f},
        };
        const Mat capturedRide=Matrix(capturedRideRows);
        const float capturedRawRows[4][4]={
            {0.6464674f,-0.2221442f,0.3085965f,0.f},
            {0.3625877f,0.5434458f,-0.3683704f,0.f},
            {-0.1144988f,0.4667104f,0.5758224f,0.f},
            {-8.98482f,3.405397f,-27.43406f,1.f},
        };
        const Mat capturedRaw=Matrix(capturedRawRows);
        const float capturedWristRows[4][4]={
            {0.5773802f,-0.2558765f,-0.4045485f,0.f},
            {0.4154352f,0.5827417f,0.2243345f,0.f},
            {0.2377937f,-0.3967866f,0.5903516f,0.f},
            {-2.880146f,-20.69319f,-35.68251f,1.f},
        };
        const Mat capturedWrist=Matrix(capturedWristRows);
        const float capturedNormalRows[4][4]={
            {0.6155529f,0.4163581f,-0.1011957f,0.f},
            {-0.2631842f,0.5071729f,0.4858082f,0.f},
            {0.3381251f,-0.3632101f,0.5623609f,0.f},
            {-1.640893f,-19.70633f,-19.61127f,1.f},
        };
        const Mat capturedNormal=Matrix(capturedNormalRows);
        const float capturedAttachmentRows[4][4]={
            {-0.9878559f,-0.09991282f,0.1189891f,0.f},
            {-0.1040552f,-0.143295f,-0.9841951f,0.f},
            {0.115384f,-0.9846237f,0.1311584f,0.f},
            {-16.47694f,14.90055f,-107.3749f,1.f},
        };
        const Mat capturedAttachment=Matrix(capturedAttachmentRows);
        Reference captured;
        captured.key=key;captured.valid=captured.wristValid=true;
        captured.tick=1000;captured.wristTick=1266;captured.wristFrame=7606;
        captured.normalWrist=capturedWrist;captured.normalRawController=capturedNormal;
        captured.gunFromWrist=capturedAttachment;
        float palette[kMaxBones][3][4]{};
        armbind::ToGpu(capturedRoot,palette[key.rootSlot]);
        Mat actual;
        assert(captured.Correct(key,palette,4,capturedLocal,capturedRide,capturedRaw,
                                7607,1281,right,&actual,&gate));
        Mat skin,inverseCapturedRoot;
        assert(armbind::FromGpu(right[key.rootSlot],skin));
        assert(skeletal::Inverse(inverseCapturedRoot.m,capturedRoot.m));
        const Mat oldDelta=Product(inverseCapturedRoot,skin);
        const auto oldP=armbind::Position(oldDelta);
        const float oldLength=std::sqrt(oldP.x*oldP.x+oldP.y*oldP.y+oldP.z*oldP.z);
        assert(oldLength>256.f && Distance(skin,capturedRoot)<256.f);
        const Mat desired=Product(Product(Product(capturedAttachment,capturedWrist),
            [&](){Mat inv;assert(skeletal::Inverse(inv.m,capturedNormal.m));return inv;}()),capturedRaw);
        assert(LargestDifference(actual,desired)<.03f);
        std::printf("captured turn guard: coefficient %.4f UU, root displacement %.4f UU, final error %.6f\n",
                    oldLength,Distance(skin,capturedRoot),LargestDifference(actual,desired));
        // A genuinely excessive root move still rejects; failed output is untouched.
        Reference tooFar=captured;tooFar.gunFromWrist.m[3][0]+=2000.f;
        float sentinel[kMaxBones][3][4];std::memcpy(sentinel,right,sizeof(right));
        assert(!tooFar.Correct(key,palette,4,capturedLocal,capturedRide,capturedRaw,
                               7607,1281,right,nullptr,&gate));
        assert(gate==CorrectGate::Displacement && std::memcmp(sentinel,right,sizeof(right))==0);
    }

    // CTAB declares 75 slots but the audited body only consumes its own map.
    // Unused registers are not skin matrices; active invalid slots still reject.
    assert(PaletteBones(75,4,true)==4&&PaletteBones(75,4,false)==75);
    assert(PaletteBones(3,4,true)==3&&PaletteBones(75,0,true)==75);
    original[4][0][0]=std::numeric_limits<float>::quiet_NaN();
    assert(reference.Correct(key,original,PaletteBones(75,4,true),local,ride,raw,3012,longNow+502,right));
    assert(!reference.Correct(key,original,PaletteBones(75,4,false),local,ride,raw,3012,longNow+502,right,nullptr,&gate));
    assert(gate==CorrectGate::Bone);
    original[1][0][0]=std::numeric_limits<float>::quiet_NaN();
    assert(!reference.Correct(key,original,4,local,ride,raw,3012,longNow+502,right,nullptr,&gate));
    assert(gate==CorrectGate::Bone);
    Key other=key;++other.weapon;
    assert(!reference.Correct(other,original,4,local,ride,raw,3012,longNow+502,right,nullptr,&gate));
    assert(gate==CorrectGate::Reference);
    assert(!reference.Correct(key,original,2,local,ride,raw,3012,longNow+502,right,nullptr,&gate));
    assert(gate==CorrectGate::Matrix);
    reference.Clear();assert(!reference.Correct(key,original,4,local,ride,raw,3012,longNow+502,right,nullptr,&gate));
    assert(gate==CorrectGate::Reference);
    std::puts("PASS dual TMD gun: normal gun anchor survives animated TMD wrist, repeated same-frame draws, long L2 hold, child/recoil pose, stereo, freshness and identity");
}
