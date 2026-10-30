#include "../native_muzzle_code_capture.h"
#include <cassert>
int main(){
    const char* path="native-muzzle-test-private.bin";std::remove(path);
    unsigned char wrapper[197]{},method[846]{};
    wrapper[0]=0x55;wrapper[196]=0xc2;method[0]=0x56;method[845]=8;
    assert(!muzzlecode::Write(path,0,wrapper,0x2000,method));
    assert(muzzlecode::Write(path,0x1000,wrapper,0x2000,method));
    assert(!muzzlecode::Write(path,0x1000,wrapper,0x2000,method));
    FILE* f=nullptr;assert(!fopen_s(&f,path,"rb")&&f);
    unsigned char bytes[1068]{};
    assert(std::fread(bytes,1,sizeof(bytes),f)==1067);assert(!std::fclose(f));
    const std::uint32_t expected[]{0x31434d53,1,0x1000,197,0x2000,846};
    assert(!std::memcmp(bytes,expected,sizeof(expected)));
    assert(!std::memcmp(bytes+24,wrapper,sizeof(wrapper)));
    assert(!std::memcmp(bytes+221,method,sizeof(method)));
    assert(!std::remove(path));
}
