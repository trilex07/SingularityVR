#include "../locker_native_trace_witness.h"
#include <cassert>

int main() {
    using namespace LockerNativeTraceWitness;

    assert(AllowOuterExecWitness(1));
    assert(LogEnterPermitted(1));
    assert(BuggyLogEnterRefused(1));

    assert(!AllowOuterExecWitness(2));
    assert(!LogEnterPermitted(2));
    assert(RefuseNestedExecWitness(2));

    puts("test-locker-native-trace-witness: PASS");
    return 0;
}
