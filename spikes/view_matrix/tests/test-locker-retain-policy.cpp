#include "../locker_retain_policy.h"
#include <cassert>

int main() {
    using namespace LockerRetainPolicy;

    assert(kPopMenuDestroyReturn == kPopMenuDestroyCallSite + 5);
    assert(CallerIsPopMenuDestroy(kPopMenuDestroyReturn));
    assert(!CallerIsPopMenuDestroy(kPopMenuDestroyCallSite));
    assert(!CallerIsPopMenuDestroy(0));

    assert(ThreadMatches(0, 1234));
    assert(ThreadMatches(8804, 8804));
    assert(!ThreadMatches(8804, 8805));

    assert(ShouldSuppress(true, true, kPopMenuDestroyReturn, 8804, 8804, true, true, true));
    assert(!ShouldSuppress(false, true, kPopMenuDestroyReturn, 8804, 8804, true, true, true));
    assert(!ShouldSuppress(true, false, kPopMenuDestroyReturn, 8804, 8804, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn + 1, 8804, 8804, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, 8804, 8805, true, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, 8804, 8804, false, true, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, 8804, 8804, true, false, true));
    assert(!ShouldSuppress(true, true, kPopMenuDestroyReturn, 8804, 8804, true, true, false));

    puts("test-locker-retain-policy: PASS");
    return 0;
}
