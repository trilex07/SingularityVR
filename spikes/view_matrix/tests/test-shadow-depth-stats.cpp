#include <cassert>
#include <limits>
#include "../shadow_depth_stats.h"
int main(){
 shadowdepth::Stats s;float pixels[8]={0,1,2,99,3,4,std::numeric_limits<float>::quiet_NaN(),99};
 assert(shadowdepth::Read(pixels,16,3,2,s));
 assert(s.finite==5 && s.nonfinite==1 && s.zero==1 && s.one==1 && s.minimum==0 && s.maximum==4);
 assert(s.grid[0]==0 && s.grid[15]==2 && s.grid[240]==3);
 assert(!shadowdepth::Read(pixels,8,3,2,s));assert(!shadowdepth::Read(nullptr,16,3,2,s));
 assert(!shadowdepth::Read(pixels,16,513,2,s));
}
