#include "../laser_style.h"
#include <cassert>
#include <initializer_list>
int main(){using namespace laserstyle;
assert(Mode(false,true)==0 && Mode(true,true)==1 && Mode(true,false)==2);
for(int m=0;m<3;++m){assert(Next(Next(m,1),-1)==m);assert(Next(Next(Next(m,1),1),1)==m);}
for(float eye: {640.f,1280.f,2064.f}){assert(HalfWidth(eye)<eye*.0015f+.5f);assert(DotRadius(eye)>HalfWidth(eye));}
for(unsigned t=0;t<100000;t+=17){assert(Shimmer(t)>=.799f && Shimmer(t)<=1.001f);}
}
