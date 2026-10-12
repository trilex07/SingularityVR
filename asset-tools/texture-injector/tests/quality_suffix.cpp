#include <cstdio>
#include "texq_naming.h"

// Kept intentionally tiny: it prints the suffix the injector will look for so
// the Python tests can prove the deploy naming matches it.
int main() {
    const char* tiers[] = {"2x", "4K", "8K", "", "bogus"};
    for (const char* t : tiers) {
        std::printf("%s\n", texq::QualitySuffix(t));
    }
    return 0;
}
