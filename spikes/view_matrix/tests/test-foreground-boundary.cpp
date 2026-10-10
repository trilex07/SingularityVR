// Regression test for the first-person boundary used by arm and muzzle
// transforms. A shadow/reflection pass can clear depth late in a frame; it must
// not turn the rest of the world into controller-ridden geometry.
#include <cassert>
#include <cstdio>

struct Boundary {
    bool scene = true;
    bool inForeground = false;
    bool confirmed = false;
    int drawCount = 0;
    int primeWindow = 16;
    int sanePass = 64;
};

static bool Match(const Boundary& b, bool shadowShader = false) {
    if (!b.scene || shadowShader) return false;
    if (b.inForeground) {
        if (b.drawCount > b.sanePass) return false;
        return b.confirmed || b.drawCount <= 8;
    }
    return b.drawCount <= b.primeWindow;
}

int main() {
    // The real first-person pass may begin before the known gun mesh is seen.
    Boundary early{true, true, false, 4};
    assert(Match(early));

    // Once the gun/arms signature has appeared, the remaining short pass stays
    // eligible for muzzle and arm work.
    Boundary confirmed{true, true, true, 20};
    assert(Match(confirmed));

    // A false shadow/reflection boundary is rejected after the small probe head,
    // and remains rejected after the measured pass limit.
    Boundary shadow{true, true, false, 20};
    assert(!Match(shadow));
    shadow.drawCount = 4000;
    assert(!Match(shadow));

    // The near-plane depth prime remains available only in its measured window.
    Boundary prime{true, false, false, 12};
    assert(Match(prime));
    prime.drawCount = 17;
    assert(!Match(prime));

    // Shadow-map vertex layouts are never eligible for foreground transforms.
    assert(!Match(confirmed, true));
    std::puts("PASS: confirmed foreground boundary rejects false shadow/reflection spans");
    return 0;
}
