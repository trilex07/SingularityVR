#include "../gore_policy.h"
#include <cassert>
#include <limits>
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
static int g_upChildren=0x50,g_upNext=0x40,g_upOffset=0x60;
static const int OBJ_CLASS=0x20;
static std::unordered_map<uintptr_t,std::string> names;
static std::vector<std::unique_ptr<unsigned char[]>> objects;
static bool Readable(void* p,size_t n){return p&&n;}
static bool IsLive(uintptr_t p){return names.count(p)!=0;}
static bool NameOf(uintptr_t p,char* out,size_t n){
    const auto it=names.find(p);if(it==names.end())return false;
    strcpy_s(out,n,it->second.c_str());return true;
}
static void Log(const char*,...){}
#include "../gore_dismember.h"
static uintptr_t Object(const char* n){
    auto p=std::make_unique<unsigned char[]>(512);
    const auto address=(uintptr_t)p.get();names[address]=n;objects.push_back(std::move(p));return address;
}
static void Link(uintptr_t p,int offset,uintptr_t value){*(uintptr_t*)(p+offset)=value;}
static uintptr_t Property(const char* n,uintptr_t type,int offset){
    auto p=Object(n);Link(p,OBJ_CLASS,type);*(int*)(p+g_upOffset)=offset;return p;
}
int main(){
    assert(gorepolicy::EnemyBase("RvAI_PawnSoldierBase"));
    assert(gorepolicy::EnemyBase("RvAI_PawnRevertBase"));
    assert(!gorepolicy::EnemyBase("RvPlayerPawn"));
    assert(!gorepolicy::EnemyBase("RvAI_PawnCombatNPC"));
    assert(gorepolicy::Excluded("RvPlayerPawnSP"));
    assert(gorepolicy::Excluded("RvAI_PawnCombatNPC"));
    assert(gorepolicy::Chance(-20)==0&&gorepolicy::Chance(150)==1);
    assert(gorepolicy::Chance(50)==.5f);
    assert(gorepolicy::Settings(-1,-1));
    assert(!gorepolicy::Settings(2,1));
    assert(!gorepolicy::Settings(std::numeric_limits<float>::quiet_NaN(),1));
    const auto soldier=Object("RvAI_PawnSoldierBase"),limbClass=Object("RvLimbSettings");
    const auto objectType=Object("ObjectProperty"),floatType=Object("FloatProperty");
    const auto settings=Property("mLimbSettings",objectType,0x80);
    const auto chance=Property("mRandomChanceToSeverLimb",floatType,0x80);
    const auto threshold=Property("mHealthPercentToTryRandomSever",floatType,0x84);
    Link(soldier,g_upChildren,settings);Link(limbClass,g_upChildren,chance);Link(chance,g_upNext,threshold);
    const auto pawn=Object("Enemy_1"),limb=Object("Settings_1");
    Link(pawn,OBJ_CLASS,soldier);Link(limb,OBJ_CLASS,limbClass);Link(pawn,0x80,limb);
    auto a=(float*)(limb+0x80);auto b=(float*)(limb+0x84);*a=-1;*b=-1;
    {singularity_gore::DamageScope s(pawn,true);assert(*a==-1);}
    singularity_gore::enabled=true;
    {singularity_gore::DamageScope s(pawn,false);assert(*a==-1);}
    {singularity_gore::DamageScope s(pawn,true);assert(s.active&&*a==1&&*b==1);
        {singularity_gore::DamageScope nested(pawn,true);assert(nested.active);}
        assert(*a==1&&*b==1);
    }
    assert(*a==-1&&*b==-1);
    // Reproduce the real RvSharedPawn chain (field at index 477).
    auto head=settings;
    for(unsigned i=0;i<477;++i){auto f=Object("UnrelatedField");Link(f,g_upNext,head);head=f;}
    Link(soldier,g_upChildren,head);
    {singularity_gore::DamageScope s(pawn,true);assert(s.active&&*a==1);}
    assert(*a==-1&&*b==-1);
    const auto derived=Object("DerivedLimbSettings");
    Link(derived,g_upNext-4,limbClass);Link(limb,OBJ_CLASS,derived);
    {singularity_gore::DamageScope s(pawn,true);assert(s.active&&*a==1);}
    assert(*a==-1&&*b==-1);
    names[soldier]="RvAI_PawnCombatNPC";
    {singularity_gore::DamageScope s(pawn,true);assert(!s.active&&*a==-1);}
    names[soldier]="RvAI_PawnSoldierBase";names[floatType]="BoolProperty";
    {singularity_gore::DamageScope s(pawn,true);assert(!s.active&&*a==-1);}
    names[floatType]="FloatProperty";
    {singularity_gore::DamageScope s(pawn,true);assert(s.active);*a=.25f;}
    assert(*a==.25f&&*b==-1); // Never overwrite a native update during the call.
    puts("PASS gore scoped: OFF/event gates, typed fields, enemy exclusion, nested restore, native update preservation [CPU mock, NOT game proof]");
}
