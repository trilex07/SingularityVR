#pragma once
#include <cmath>

// Weapon-specific support points in the native UE3 gun frame (X forward, Y right, Z up).
// These are deliberately keyed by the same shipped vertex fingerprints as GunAlignSlot; there
// is no universal hand offset. Values are conservative starting points until a consolidated
// headset pass can refine them against the actual mesh bounds.
enum class SupportGripMode { AuthoredPair, ForeEnd };
struct SupportGripSpec { long verts, fwd, right, up; SupportGripMode mode; };
static const SupportGripSpec kSupportGripSpecs[] = {
    // The shipped pistol pose already contains an authored two-hand relationship. Applying the
    // primary rigid delta to both weighted regions preserves that relationship exactly; an added
    // universal offset was what put the support hand below the pistol in the headset.
    { 5799,  0,  0,  0, SupportGripMode::AuthoredPair },
    { 8297, 24, -5, -2, SupportGripMode::ForeEnd }, // shotgun fore-end
    {11387, 32, -5, -2, SupportGripMode::ForeEnd }, // assault-rifle fore-end
    {11389, 38, -5, -2, SupportGripMode::ForeEnd }, // sniper fore-end
};
inline SupportGripSpec SupportGripFor(long verts) {
    for (const auto& s : kSupportGripSpecs) if (s.verts==verts) return s;
    return SupportGripSpec{verts,8,-5,-3,SupportGripMode::ForeEnd};
}

// Compose a support-hand target from the weapon's already calibrated transform. The target uses
// the weapon rotation plus a local support offset, so it remains attached while the primary hand
// turns. The caller supplies the row-vector weapon transform used by BuildGunC.
inline bool ApplySupportGripTarget(float off[4][4], const float primary[4][4], long verts) {
    const SupportGripSpec s=SupportGripFor(verts);
    for(int i=0;i<3;++i) for(int j=0;j<3;++j) off[i][j]=primary[i][j];
    for(int j=0;j<3;++j) {
        const float p = (float)s.fwd*primary[0][j] + (float)s.right*primary[1][j] + (float)s.up*primary[2][j];
        off[3][j]=primary[3][j]+p;
    }
    off[3][3]=1.0f;
    return std::isfinite(off[3][0])&&std::isfinite(off[3][1])&&std::isfinite(off[3][2]);
}
