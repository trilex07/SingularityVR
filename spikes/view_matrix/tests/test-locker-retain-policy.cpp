#include "../locker_retain_policy.h"
#include <cassert>

int main() {
    using namespace LockerRetainPolicy;

    constexpr DWORD kTid = 8804u;
    constexpr uintptr_t kMgr = 0x1BD77000u;
    constexpr uintptr_t kOtherMgr = 0x1BD78000u;

    assert(kPopMenuDestroyReturn == kPopMenuDestroyCallSite + 5);
    assert(CallerIsPopMenuDestroy(kPopMenuDestroyReturn));
    assert(!CallerIsPopMenuDestroy(kPopMenuDestroyCallSite));
    assert(!CallerIsPopMenuDestroy(0));

    assert(!EstablishedThreadMatches(0, kTid));
    assert(EstablishedThreadMatches(kTid, kTid));
    assert(!EstablishedThreadMatches(kTid, kTid + 1));

    assert(!ManagerMatches(0, kMgr));
    assert(!ManagerMatches(kMgr, 0));
    assert(ManagerMatches(kMgr, kMgr));
    assert(!ManagerMatches(kOtherMgr, kMgr));

    auto ok = [&]() {
        return ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, true, true, true);
    };
    assert(ok());
    assert(!ShouldSuppress(false, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, true, true, true));
    assert(!ShouldSuppress(true, false, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn + 1, kTid, kTid, kMgr, kMgr, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, 0, kTid, kMgr, kMgr, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid + 1, kMgr, kMgr, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, 0, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kOtherMgr, kMgr, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, false, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, true, false, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, kTid, kTid, kMgr, kMgr, true, true, false));

    puts("test-locker-retain-policy: PASS");
    return 0;
}
