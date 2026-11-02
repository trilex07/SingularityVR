#include "../damage_haptics.h"
#include <cstdio>
#include <cstdlib>
static void check(bool b,const char* s){if(!b){std::printf("FAIL: %s\n",s);std::exit(1);}}
int main(){damagehaptics::Engine e;
 check(!e.OnNativeReturn(false,100,80,100),"enemy/foreign pawn ignored");
 check(!e.OnNativeReturn(true,100,100,100),"blocked damage ignored");
 check(!e.OnNativeReturn(true,80,100,100),"healing ignored");
 check(!e.OnNativeReturn(true,0,0,100),"dead pawn ignored");
 check(e.Sample(true,100)==0,"no false idle pulse");
 check(e.OnNativeReturn(true,100,90,100),"actual health loss");
 check(e.Sample(true,110)>.4f&&e.Sample(true,110)<.8f,"bounded damage amplitude");
 check(e.OnNativeReturn(true,90,20,120)&&e.Sample(true,121)==.8f,"nested pulse max without addition");
 check(e.Sample(true,185)==0,"no prolonging nested hit");
 check(e.OnNativeReturn(true,20,-5,200),"lethal hit clamps at zero");
 check(e.Sample(false,201)==0&&e.Sample(true,202)==0,"pause/focus clear without replay");
 check(!e.OnNativeReturn(true,100,-1000001,300),"corrupt health rejected");
 check(e.OnNativeReturn(true,100,80,300),"next legitimate hit");e.Clear();check(e.Sample(true,301)==0,"explicit reset");
 puts("local damage native-return pulse / gates / expiry: PASS");
}
