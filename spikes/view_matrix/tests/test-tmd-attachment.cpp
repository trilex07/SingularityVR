#include "../tmd_attachment_identity.h"
#include <cassert>
#include <cstdio>
int main(){
 const unsigned char vector[]="123456789";
 assert(tmdattachment::Crc(vector,9)==0xcbf43926u);
 for(auto s:tmdattachment::signatures){
  assert(tmdattachment::Candidate(s.vertices,s.faces));
  assert(tmdattachment::Verified(s.vertices,s.faces,s.crc));
  assert(!tmdattachment::Verified(s.vertices+1,s.faces,s.crc));
  assert(!tmdattachment::Verified(s.vertices,s.faces,s.crc^1));
 }
 assert(!tmdattachment::Candidate(7444,7704));
 puts("PASS: 11 cooked identities, CRC, signature collision rejection, native body excluded");
}
