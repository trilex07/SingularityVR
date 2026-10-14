#include "../ui_asset_witness.h"
#include <cassert>
#include <cstdio>
int main(){
 assert(uiwitness::ArrayBounds(8,8));
 assert(uiwitness::ArrayBounds(100,128));
 assert(!uiwitness::ArrayBounds(-1,128));
 assert(!uiwitness::ArrayBounds(7,128));
 assert(!uiwitness::ArrayBounds(100,99));
 assert(!uiwitness::ArrayBounds(100,uiwitness::kMaxMovieBytes+1));
 const unsigned char hello[]="hello";
 assert(uiwitness::Fingerprint(hello,5)==0xa430d84680aabd0bull);
 assert(uiwitness::Fingerprint(hello,4)!=uiwitness::Fingerprint(hello,5));
 std::puts("PASS: bounded cold GFx asset identity hash; invalid arrays refuse");
}
