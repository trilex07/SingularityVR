#include "../cinema_mutant_aim.h"
#include <cassert>
#include <cstdio>
#include <limits>
struct Scene {
    bool interaction=true,attacking=true;uintptr_t instigator=2;
    bool PlayerInteraction(uintptr_t p){return interaction && p==1;}
    uintptr_t Instigator(uintptr_t){return instigator;}
    bool Attacking(uintptr_t object,uintptr_t pawn){return attacking && object==2 && pawn==1;}
};
int main(){
    Scene scene;assert(cinemamutant::InteractionAttacker(scene,1)==2);
    scene.interaction=false;assert(!cinemamutant::InteractionAttacker(scene,1));
    scene.interaction=true;scene.instigator=3;assert(!cinemamutant::InteractionAttacker(scene,1));
    scene.instigator=0;assert(!cinemamutant::InteractionAttacker(scene,1));
    scene.instigator=2;scene.attacking=false;assert(!cinemamutant::InteractionAttacker(scene,1));
    float start[3]={18465.7f,-24344.4f,423.137f};
    float end[3]={18084.7f,-25085.1f,8379.65f};
    float center[3]={18464.9727f,-24315.6426f,475.1683f},out[3];
    auto redirect=[&](bool en,bool cinema,bool fire,uintptr_t src,uintptr_t target){
        return cinemamutant::Redirect(en,cinema,fire,src,1,target,start,end,center,out);};
    assert(!redirect(false,true,true,1,2));assert(!redirect(true,false,true,1,2));
    assert(!redirect(true,true,false,1,2));assert(!redirect(true,true,true,9,2));
    assert(!redirect(true,true,true,1,0));assert(redirect(true,true,true,1,2));
    double before=0,after=0,cross=0;
    for(int i=0;i<3;++i){before+=std::pow(double(end[i])-start[i],2);after+=std::pow(double(out[i])-start[i],2);}
    assert(std::abs(std::sqrt(before)-std::sqrt(after))<0.01);
    for(int i=0;i<3;++i)for(int j=0;j<3;++j)
        cross+=std::abs((double(out[i])-start[i])*(center[j]-start[j])-(double(out[j])-start[j])*(center[i]-start[i]));
    assert(cross<1.0); // float world-coordinate roundoff
    assert(!cinemamutant::Redirect(true,true,true,1,1,2,start,end,start,out));
    float shortEnd[3]={start[0],start[1],start[2]+100};
    assert(!cinemamutant::Redirect(true,true,true,1,1,2,start,shortEnd,center,out));
    center[0]=std::numeric_limits<float>::quiet_NaN();assert(!redirect(true,true,true,1,2));
    center[0]=1e8f;assert(!redirect(true,true,true,1,2));
    puts("cinema mutant exact-state selection and native-range ray: PASS");
}
