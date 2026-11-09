#include "../laser_style.h"
#include <cassert>
#include <initializer_list>
int main(){using namespace laserstyle;
assert(Clamp(-1,25,200)==25 && Clamp(201,25,200)==200 && Clamp(100,25,200)==100);
assert(Brightness(0xA0FF5050u,50)==0x50FF5050u);
assert(Brightness(0xA0FF5050u,200)==0xA0FF5050u);
assert(Brightness(0x00FF3030u,10)==0x00FF3030u);
assert(Mode(false,true)==0 && Mode(true,true)==1 && Mode(true,false)==2);
for(int m=0;m<3;++m){assert(Next(Next(m,1),-1)==m);assert(Next(Next(Next(m,1),1),1)==m);}
for(float eye: {640.f,1280.f,2064.f}){assert(HalfWidth(eye)<eye*.0015f+.5f);assert(DotRadius(eye)>HalfWidth(eye));}
for(unsigned t=0;t<100000;t+=17){assert(Shimmer(t)>=.799f && Shimmer(t)<=1.001f);}
}
