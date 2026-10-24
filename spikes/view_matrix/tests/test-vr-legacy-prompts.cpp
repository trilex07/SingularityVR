#include "../vr_legacy_prompts.h"
#include "../ui_menu_skin.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace vrprompts;Control c[Count]{};c[Reload]=Circle;c[Use]=RightGrip;c[Impulse]=L2;
 char out[1024];
 auto r=Rewrite("Press [reload], then [use].",out,sizeof(out),c,false);assert(r.changed&&!strcmp(out,"Press (O), then RIGHT GRIP."));
 r=Rewrite("Pad 11 to reload",out,sizeof(out),c,false,"Tutorial","Reload");assert(r.changed&&!strcmp(out,"(O) to reload"));
 r=Rewrite("Pad 11 is the name. [unknown]",out,sizeof(out),c,false,"Notes","Story");assert(!r.changed&&!strcmp(out,"Pad 11 is the name. [unknown]"));
 r=Rewrite("Mission Pad 11",out,sizeof(out),c,true,"Objectives","Reload");assert(!r.changed);
    r=Rewrite("Pad 11",out,sizeof(out),c,false,"Keyboard","Button10");assert(r.changed&&!out[0]);
    r=Rewrite("Pad 11",out,sizeof(out),c,false,"Keyboard","Unknown");assert(!r.changed);
    r=Rewrite("[reload] Pad 11",out,sizeof(out),c,false,"Subtitles","Line");assert(!r.changed);
 r=Rewrite("Pad 110x",out,sizeof(out),c,false,"Tutorial","Reload");assert(!r.changed);
 r=Rewrite("Nacisnij [use]",out,sizeof(out),c,true);assert(!strcmp(out,"Nacisnij PRAWY CHWYT"));
 r=Rewrite("[use]",out,2,c,false);assert(r.overflow&&!r.changed&&!out[0]);
 assert(Path("/user/hand/right/input/squeeze/value")==RightGrip);
 assert(Path("/user/hand/left/input/trigger/value")==L2);
 assert(Path("Pad 11")==Unbound);assert(Path("/user/hand/left/input/a/click")==Unbound);
 assert(Path("/user/hand/right/input/b/click")==Circle);
 assert(menuskin::Eligible(true,2,false,false));assert(menuskin::Eligible(true,3,false,false));
 assert(!menuskin::Eligible(true,4,false,false));assert(!menuskin::Eligible(true,2,true,false));assert(!menuskin::Eligible(false,2,false,false));
 puts("PASS: action-scoped replacement, PL/EN, bound-source controls, protected literal text, overflow and menu gates");
}
