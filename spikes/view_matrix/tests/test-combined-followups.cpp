#include <cstdio>
#include <cstdlib>
#include <limits>
#include <fstream>
#include <vector>
#include "../npc_name_policy.h"
#include "../inventory_retention_policy.h"
static unsigned checks=0;
static void Check(bool pass){++checks;if(!pass){std::fprintf(stderr,"FAIL check %u\n",checks);std::exit(1);}}
int main(int argc,char** argv) {
 using namespace inventoryretention;
 Check(npcname::ShouldScale(100,100,60));
 Check(!npcname::ShouldScale(0,0,60));
 Check(!npcname::ShouldScale(60,60,60));
 Check(!npcname::ShouldScale(100,0,60));
 Check(!npcname::ShouldScale(100,100,100));
 Check(!npcname::ShouldScale(100,100,39));
 Check(!npcname::ShouldScale(std::numeric_limits<float>::quiet_NaN(),100,60));
 Check(!npcname::ShouldScale(100,std::numeric_limits<float>::infinity(),60));
 Check(WeaponMesh("ARPistol")==5799);
 Check(WeaponMesh("ARMachineGun")==11387);
 Check(WeaponMesh("ARShotgun")==8297);
 Check(WeaponMesh("ARSniperRifle")==11389);
 Check(WeaponMesh("Unknown")==0&&WeaponMesh(nullptr)==0);
 Check((WeaponBit(5799)|WeaponBit(11387)|WeaponBit(8297)|WeaponBit(11389))==15);
 Check(WeaponBit(0)==0&&WeaponBit(9999)==0);
 // A missing first/middle icon cannot suppress later owned entries.
 unsigned drawn=0;
 const bool named[]={false,true,false,true},icons[]={true,true,false,true};
 for(unsigned i=0;i<4;++i)if(IconSlot(named[i],icons[i]))drawn|=1u<<i;
 Check(drawn==10);
 Check(!OriginalShape(nullptr,284));
 if(argc!=2){std::fprintf(stderr,"Provide private original decoded RvGame fixture\n");return 2;}
 std::ifstream file(argv[1],std::ios::binary);
 Check(bool(file));file.seekg(4079214+44);
 std::vector<unsigned char> code(ScriptBytes);file.read((char*)code.data(),code.size());
 Check(file.gcount()==ScriptBytes&&OriginalShape(code.data(),code.size()));
 auto relocated=code;
 for(auto r:Objects)for(unsigned i=0;i<4;++i)relocated[r.at+i]=(unsigned char)(r.at+i);
 for(auto r:Names)for(unsigned i=0;i<8;++i)relocated[r.at+i]=(unsigned char)(r.at+i);
 Check(OriginalShape(relocated.data(),relocated.size()));
 // Every non-relocation byte is required, including branch targets and tail.
 unsigned mutations=0;
 for(unsigned i=0;i<ScriptBytes;++i){
  bool masked=false;
  for(auto r:Objects)if(i>=r.at&&i<r.at+4)masked=true;
  for(auto r:Names)if(i>=r.at&&i<r.at+8)masked=true;
  if(masked)continue;
  auto changed=code;changed[i]^=1;
  Check(!OriginalShape(changed.data(),changed.size()));++mutations;
 }
 auto changed=code;
 std::memset(changed.data()+DropAt,0x0b,DropBytes);
 Check(std::memcmp(changed.data(),code.data(),DropAt)==0);
 Check(std::memcmp(changed.data()+DropAt+DropBytes,code.data()+DropAt+DropBytes,
                   ScriptBytes-DropAt-DropBytes)==0);
 Check(changed.size()==code.size()&&!OriginalShape(changed.data(),changed.size()));
 std::memcpy(changed.data()+DropAt,code.data()+DropAt,DropBytes);
 Check(changed==code);
 std::printf("PASS %u checks (%u opcode/branch mutation refusals); offline only\n",checks,mutations);
}
