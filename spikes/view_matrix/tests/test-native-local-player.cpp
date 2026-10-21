#include "../native_local_player.h"
#include <cassert>
#include <map>
#include <string>
#include <cstdio>
struct Access {
    using Ptr=std::uintptr_t;
    std::map<Ptr,std::string> types;
    std::map<std::pair<Ptr,std::string>,Ptr> links;
    bool IsCamera(Ptr p){return types[p]=="camera";}
    bool IsController(Ptr p){return types[p]=="controller";}
    bool IsPawn(Ptr p){return types[p]=="pawn";}
    Ptr Link(Ptr p,const char* field){return links[{p,field}];}
    void Add(Ptr c,Ptr pc,Ptr p) {
        types[c]="camera";types[pc]="controller";types[p]="pawn";
        links[{c,"PCOwner"}]=pc;links[{pc,"PlayerCamera"}]=c;
        links[{pc,"Pawn"}]=p;links[{p,"Controller"}]=pc;
    }
};
int main() {
    using nativeplayer::Resolve;using nativeplayer::Stage;
    Access a;a.Add(10,20,30);a.Add(110,120,130);
    assert(Resolve(10,a).pawn==30 && Resolve(110,a).pawn==130);
    // The previous controller/pawn remain fully readable and live. The current
    // camera still selects only its own pair without waiting for any scan.
    assert(Resolve(110,a).controller==120 && Resolve(110,a).stage==Stage::Ready);
    a.links[{120,"Pawn"}]=0;
    assert(Resolve(110,a).controller==120 && !Resolve(110,a).pawn);
    assert(Resolve(110,a).stage==Stage::Pawn); // unpossessed cinema still has an owner
    a.links[{120,"Pawn"}]=30;
    assert(!Resolve(110,a).pawn && Resolve(110,a).stage==Stage::PawnReciprocal);
    a.links[{120,"Pawn"}]=130;a.types[130]="enemy";
    assert(!Resolve(110,a).pawn && Resolve(110,a).stage==Stage::PawnType);
    a.types[130]="pawn";a.links[{120,"PlayerCamera"}]=10;
    assert(!Resolve(110,a).controller && Resolve(110,a).stage==Stage::CameraReciprocal);
    a.links[{120,"PlayerCamera"}]=110;a.types[120]="dead-controller";
    assert(!Resolve(110,a).controller && Resolve(110,a).stage==Stage::Owner);
    assert(!Resolve(0,a).controller && Resolve(0,a).stage==Stage::Camera);
    using nativeplayer::ObjectField;
    assert(ObjectField("Mesh","ComponentProperty",0x440,1,4));
    assert(!ObjectField("Mesh","ObjectProperty",0x440,1,4));
    assert(ObjectField("SkeletalMesh","ObjectProperty",0x440,1,4));
    assert(!ObjectField("Pawn","ComponentProperty",0x440,1,4));
    assert(!ObjectField("Pawn","ObjectProperty",-1,1,4));
    assert(!ObjectField("Pawn","ObjectProperty",0x441,1,4));
    assert(!ObjectField("Pawn","ObjectProperty",0x440,2,4));
    assert(!ObjectField("Pawn","ObjectProperty",0x440,1,8));
    puts("PASS local camera ownership: travel with live stale objects, no pawn, reciprocal/type rejection, native ComponentProperty contract");
}
