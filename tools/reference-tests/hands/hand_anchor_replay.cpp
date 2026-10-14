// No captured game constants committed: read the private bounded replay file.
#include "../../../spikes/view_matrix/arm_bind.h"
#include "../../../spikes/view_matrix/singularity_bind_reference.h"
#include "../../../spikes/view_matrix/hand_anchor_math.h"
#include <sstream>
#include <fstream>
#include <cstdio>
static armik::V Point(armik::V p,const float m[4][4]){
    return {p.x*m[0][0]+p.y*m[1][0]+p.z*m[2][0]+m[3][0],
            p.x*m[0][1]+p.y*m[1][1]+p.z*m[2][1]+m[3][1],
            p.x*m[0][2]+p.y*m[1][2]+p.z*m[2][2]+m[3][2]};
}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    std::ifstream f(argv[1]);if(!f)return 2;
    armbind::Mat bind[90];if(!armbind::BuildReference(bindassets::noTmd,90,bind))return 3;
    unsigned count=0,frame;int moved;
    std::puts("ASSUMPTION: live ride camera equals copied controllerAim/Off; not pixel validation");
    std::string line;
    while(std::getline(f,line)){
        std::istringstream row(line);if(!(row>>frame>>moved))return 3;
        if(++count>8||moved<0||moved>2)return 3;
        float target[2][3],local[4][4],primary[4][4],off[4][4],native[2][3][4],posed[2][3][4];
        auto read=[&](float* p,unsigned n){for(unsigned i=0;i<n;++i)if(!(row>>p[i])||!std::isfinite(p[i]))return false;return true;};
        if(!read(target[0],3)||!read(target[1],3)||!read(local[0],16)||!read(primary[0],16)||!read(off[0],16)||
           !read(native[0][0],12)||!read(native[1][0],12)||!read(posed[0][0],12)||!read(posed[1][0],12))return 3;
        float current[4][4]{};
        if(moved)std::memcpy(current,moved==1?primary:off,sizeof(current));else for(int n=0;n<4;++n)current[n][n]=1;
        float ride0;bool actualRide=false;
        if(row>>ride0){current[0][0]=ride0;if(!read(&current[0][0]+1,15))return 3;actualRide=true;}
        for(unsigned side=0;side<2;++side){
            armbind::Mat a,b;if(!armbind::CurrentJoint(bind[side?51:9],native[side],a)||!armbind::CurrentJoint(bind[side?51:9],posed[side],b))return 3;
            auto nativeWorld=Point(armbind::Position(a),local);
            auto posedWorld=Point(Point(armbind::Position(b),local),current);
            armik::V h{target[side][0],target[side][1],target[side][2]};auto error=posedWorld-h;
            float adjusted[4][4],deltaL[4][4],deltaR[4][4],fixed[3][4];
            if(!handanchor::TargetWrist(adjusted,bind[side?51:9],native[side],local,side?primary:off,h)||
               !skeletal::LocalDeltaPair(deltaL,deltaR,local,current,adjusted,adjusted))return 3;
            skeletal::TransformBone(fixed,native[side],deltaL);
            if(!armbind::CurrentJoint(bind[side?51:9],fixed,b))return 3;
            auto corrected=Point(Point(armbind::Position(b),local),current);
            float correctedError=armik::Length(corrected-h);if(correctedError>.003f)return 3;
            std::printf("frame=%u side=%u moved=%d actualRide=%d native=(%.3f,%.3f,%.3f) reconstructed=(%.3f,%.3f,%.3f) target=(%.3f,%.3f,%.3f) residualUU=%.3f correctedReplayUU=%.6f\n",
                frame,side,moved,actualRide,nativeWorld.x,nativeWorld.y,nativeWorld.z,posedWorld.x,posedWorld.y,posedWorld.z,h.x,h.y,h.z,armik::Length(error),correctedError);
        }
    }
    return count?0:3;
}
