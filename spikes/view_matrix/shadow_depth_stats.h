#pragma once
#include <cmath>
#include <cstring>
namespace shadowdepth {
struct Stats {
 unsigned finite=0,nonfinite=0,zero=0,one=0;float minimum=0,maximum=0,grid[256]{};
};
inline bool Read(const void* data,int pitch,unsigned width,unsigned height,Stats& out){
 out={};if(!data||!width||!height||width>512||height>512||pitch<int(width*4))return false;
 for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
  float v;memcpy(&v,(const char*)data+y*pitch+x*4,4);
  if(!std::isfinite(v)){++out.nonfinite;continue;}
  if(!out.finite)out.minimum=out.maximum=v;
  else{if(v<out.minimum)out.minimum=v;if(v>out.maximum)out.maximum=v;}
  ++out.finite;if(v==0)++out.zero;if(v==1)++out.one;
 }
 for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x){
  const unsigned px=((2*x+1)*width)/32,py=((2*y+1)*height)/32;
  memcpy(&out.grid[y*16+x],(const char*)data+py*pitch+px*4,4);
 }
 return true;
}
}
