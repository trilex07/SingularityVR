#include "../locker_vr_loadout_policy.h"
#include <cassert>
#include <cstdio>

static bool g_rowFail = false;

static bool RowAt(unsigned index, LockerVrLoadoutPolicy::CatalogRowView* out) {
    if (g_rowFail || !out) return false;
    out->readable = true;
    out->row24 = 0;
    out->row4c = 0;
    if (index == 99) {
        out->row24 = 1;
        return true;
    }
    return true;
}

int main() {
    using namespace LockerVrLoadoutPolicy;
    assert(AvailableOrdinaryMaskFromNative(0x00, RowAt) == 0x0);
    assert(AvailableOrdinaryMaskFromNative(0x01, RowAt) == 0x1);
    assert(AvailableOrdinaryMaskFromNative(0x02, RowAt) == 0x2);
    assert(AvailableOrdinaryMaskFromNative(0x04, RowAt) == 0x4);
    assert(AvailableOrdinaryMaskFromNative(0x20, RowAt) == 0x8);
    assert(AvailableOrdinaryMaskFromNative(0x27, RowAt) == 0xF);
    assert(AvailableOrdinaryMaskFromNative(0x18, RowAt) == 0x0);

    CatalogRowView bad{};
    bad.readable = true;
    bad.row24 = 1;
    bad.row4c = 0;
    auto rowBad24 = [](unsigned index, CatalogRowView* out) -> bool {
        if (!out || index != kCatalogIndexPistol) return false;
        out->readable = true;
        out->row24 = 1;
        out->row4c = 0;
        return true;
    };
    assert(AvailableOrdinaryMaskFromNative(0x01, rowBad24) == 0x0);

    auto rowBad4c = [](unsigned index, CatalogRowView* out) -> bool {
        if (!out || index != kCatalogIndexMg) return false;
        out->readable = true;
        out->row24 = 0;
        out->row4c = 1;
        return true;
    };
    assert(AvailableOrdinaryMaskFromNative(0x02, rowBad4c) == 0x0);

    assert(PendingMask(0xF, 0x1) == 0xE);
    assert(PendingMask(0x7, 0x5) == 0x2);
    assert(PendingMask(0x3, 0xF) == 0x0);

    assert(RevalidateQueuedMask(0x4, 0x3, 0x0) == 0x0);

    g_rowFail = true;
    assert(AvailableOrdinaryMaskFromNative(0x27, RowAt) == 0x0);

    std::puts("test-locker-vr-loadout-policy: PASS");
    return 0;
}
