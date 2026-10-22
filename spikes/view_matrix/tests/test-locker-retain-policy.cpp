#include "../locker_retain_policy.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace LockerRetainPolicy;

    constexpr DWORD kTid = 8804u;
    constexpr uintptr_t kMgr = 0x1BD77000u;
    constexpr uintptr_t kOtherMgr = 0x1BD78000u;

    assert(kPopMenuDestroyReturn == kPopMenuDestroyCallSite + 5);
    assert(kPopMenuInventoryGlobal == 0x01CF74E4u);

    auto acceptPopMenuShape = [&]() {
        return ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, 0, 1, true, true, true);
    };
    assert(acceptPopMenuShape());

    assert(!ShouldSuppress(false, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, 0, 1, true, true, true));
    assert(!ShouldSuppress(true, false, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, 0, 1, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn + 1, kTid, kTid, kMgr, kMgr, 0, 1, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, 0, kTid, kMgr, kMgr, 0, 1, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid + 1, kMgr, kMgr, 0, 1, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, 0, kMgr, 0, 1, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, 0, 0, 1, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kOtherMgr, kMgr, 0, 1, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, 1, 1, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, 0, 0, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, 0, 1, false, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, 0, 1, true, false, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, 0, 1, true, true, false));

    assert(PopMenuDestroyArgsMatch(0, 1));
    assert(!PopMenuDestroyArgsMatch(1, 1));
    assert(!PopMenuDestroyArgsMatch(0, 0));

    puts("test-locker-retain-policy: PASS");
    return 0;
}
