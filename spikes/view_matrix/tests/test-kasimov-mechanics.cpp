#define NOMINMAX
#include "../kasimov_mechanics.h"
#include <cassert>
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);std::ifstream in(argv[1],std::ios::binary);std::vector<unsigned char>b{std::istreambuf_iterator<char>(in),{}};
 weaponreloadanim::Data d;assert(weaponreloadanim::Decode(b.data(),b.size(),weaponreload::Kind::Kasimov,d));assert(weaponreloadanim::ToNative(d));
 assert(d.frames==239&&d.fps==60&&d.slots==6&&d.parent[3]==2&&d.parent[4]==3);
 weaponreload::Animation timing;assert(timing.Measure(d));float worst=0;unsigned count=0;
 for(unsigned slot:{1u,2u,3u}){armbind::Mat delta;assert(volkmechanics::Relative(d,slot,238,238,delta));for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)worst=std::max(worst,fabsf(delta.m[r][c]-(r==c?1.f:0.f)));}
 assert(worst<.00003f);
 for(unsigned pose=0;pose<3;++pose){
  float parent[72],out[72],before[72];
  for(unsigned slot=0;slot<6;++slot){armbind::Mat m{};float a=pose*.83f;m.m[0][0]=m.m[1][1]=cosf(a);m.m[0][1]=sinf(a);m.m[1][0]=-sinf(a);m.m[2][2]=m.m[3][3]=1;m.m[3][0]=slot+pose*8.f;armbind::ToGpu(m,reinterpret_cast<float(*)[4]>(parent+slot*12));}
  for(unsigned ms=0;ms<=500;++ms){
   float frame=timing.RefillFrame(d,weaponreload::Phase::WaitingNative,1000,1000,1000+ms);
   assert(weaponreload::SeatingPalette(d,frame,parent,18,out));memcpy(before,out,sizeof(out));
   assert(kasimovmechanics::Seat(d,timing,weaponreload::Phase::WaitingNative,1000,1000+ms,parent,18,out));
   for(unsigned slot:{0u,4u,5u})assert(!memcmp(out+slot*12,before+slot*12,12*sizeof(float)));
   for(float f:out)assert(std::isfinite(f));
   if(ms>=180)assert(!memcmp(out,before,sizeof(out)));
   else for(unsigned slot:{1u,2u,3u}){armbind::Mat delta,root,skin;assert(volkmechanics::Relative(d,slot,frame,238,delta));assert(armbind::FromGpu(reinterpret_cast<const float(*)[4]>(parent),root));skeletal::Multiply(skin.m,delta.m,root.m);float gpu[12];armbind::ToGpu(skin,reinterpret_cast<float(*)[4]>(gpu));assert(!memcmp(out+slot*12,gpu,sizeof(gpu)));}
   ++count;
  }
  memcpy(out,parent,sizeof(out));assert(kasimovmechanics::Seat(d,timing,weaponreload::Phase::Carrying,0,1000,parent,18,out));assert(!memcmp(out,parent,sizeof(out)));
 }
 printf("Kasimov 239f/60fps inherited wire chain: %u pose/timeline samples, measured out/in %.0f/%.0f, endpoint error %.9g; physical clip/root/Trigger preserved PASS\n",count,timing.outFrame,timing.inFrame,worst);
}
