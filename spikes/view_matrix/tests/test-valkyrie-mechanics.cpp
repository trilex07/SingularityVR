#define NOMINMAX
#include "../valkyrie_mechanics.h"
#include <cassert>
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);std::ifstream in(argv[1],std::ios::binary);std::vector<unsigned char> b{std::istreambuf_iterator<char>(in),{}};
 weaponreloadanim::Data d;assert(weaponreloadanim::Decode(b.data(),b.size(),weaponreload::Kind::Valkyrie,d));assert(weaponreloadanim::ToNative(d));
 weaponreload::Animation timing;assert(timing.Measure(d));assert(d.frames==52&&d.fps==24&&d.slots==6);
 float worst=0;
 for(unsigned slot:{4u,5u}){armbind::Mat m;assert(volkmechanics::Relative(d,slot,51,51,m));
  for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)worst=std::max(worst,fabsf(m.m[r][c]-(r==c?1.f:0.f)));}
 assert(worst<.00003f);
 for(unsigned pose=0;pose<3;++pose){
  float parent[72],out[72],before[72];
  for(unsigned slot=0;slot<6;++slot){armbind::Mat m{};float a=pose*.83f;m.m[0][0]=m.m[1][1]=cosf(a);m.m[0][1]=sinf(a);m.m[1][0]=-sinf(a);m.m[2][2]=m.m[3][3]=1;m.m[3][0]=slot+pose*8.f;armbind::ToGpu(m,reinterpret_cast<float(*)[4]>(parent+slot*12));}
  for(unsigned ms=0;ms<=500;++ms){
   assert(weaponreload::SeatingPalette(d,timing.RefillFrame(d,weaponreload::Phase::WaitingNative,1000,1000,1000+ms),parent,18,out));memcpy(before,out,sizeof(out));
   assert(valkyriemechanics::Seat(d,timing,weaponreload::Phase::WaitingNative,1000,1000+ms,parent,18,out));
   assert(!memcmp(out,before,48*sizeof(float))); // root, Trigger, release and tracked clip unchanged
   for(float f:out)assert(std::isfinite(f));
   if(ms>=180)assert(!memcmp(out,before,sizeof(out))); // no override of native first shot
  }
  memcpy(out,parent,sizeof(out));assert(valkyriemechanics::Seat(d,timing,weaponreload::Phase::Carrying,0,1000,parent,18,out));assert(!memcmp(out,parent,sizeof(out)));
 }
 printf("Valkyrie original 52f/24fps: measured clip out/in=%.0f/%.0f; 1503 wrist/timeline samples; endpoint error %.9g; root/Trigger/release/clip unchanged PASS\n",timing.outFrame,timing.inFrame,worst);
}
