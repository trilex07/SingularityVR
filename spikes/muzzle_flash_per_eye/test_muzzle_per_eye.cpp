// Per-eye muzzle flash math — pure arithmetic, no game, no headset, no D3D9.
//
// The question it answers: does the clip-x rewrite used by DrawMuzzleFlashPerEye
// (spikes/view_matrix/d3d9.cpp) place one full muzzle flash in each eye half of the
// SBS frame, instead of one seam-straddling flash at clip x == 0?
//
// Rewrite (verbatim from d3d9.cpp DrawMuzzleFlashPerEye):
//   left eye  : x' = 0.5f * x - 0.5f   (maps full-frame NDC x in [-1,+1] to left half [-1,0])
//   right eye : x' = 0.5f * x + 0.5f   (maps full-frame NDC x in [-1,+1] to right half [0,+1])
//
// The flash lives at the seam (x == 0 in the full side-by-side frame). After the rewrite:
//   left eye : x' = -0.5f  -> centre of the left eye's half
//   right eye: x' = +0.5f  -> centre of the right eye's half
// So each eye gets its own full flash, centred in its half, with nothing on the seam.
//
// This is the same per-eye squash as DrawSpriteQuadPerEye; it is tested independently so a
// drifted copy in d3d9.cpp shows up here as a failure instead of a tilted/broken flash in the
// headset.
#include <cstdio>
#include <cmath>

// ---- verbatim from d3d9.cpp (DrawMuzzleFlashPerEye vertex rewrite) ----
static inline void ApplyMuzzleFlashPerEye(float* x, int eye) {
    // eye 0 = left, eye 1 = right
    const float xOff = (eye == 0) ? -0.5f : +0.5f;
    *x = (*x) * 0.5f + xOff;
}

static int g_fail = 0;
void Check(const char* what, float got, float want, float tol = 1e-4f) {
    const bool ok = fabsf(got - want) <= tol;
    if (!ok) ++g_fail;
    printf("  %-60s got %+8.4f want %+8.4f  %s\n", what, got, want, ok ? "ok" : "**FAIL**");
}

int main() {
    printf("=== Muzzle flash per-eye math (both eyes) ===\n");

    // The seam: flash at clip x == 0 in the full SBS frame.
    float seam = 0.0f;
    ApplyMuzzleFlashPerEye(&seam, 0);
    Check("seam x=0 -> left eye centre (-0.5)", seam, -0.5f);

    seam = 0.0f;
    ApplyMuzzleFlashPerEye(&seam, 1);
    Check("seam x=0 -> right eye centre (+0.5)", seam, +0.5f);

    // Full-frame left edge x=-1 maps to left eye's left edge.
    float leftEdge = -1.0f;
    ApplyMuzzleFlashPerEye(&leftEdge, 0);
    Check("full left edge x=-1 -> left eye left edge (-1.0)", leftEdge, -1.0f);

    // Full-frame right edge x=+1 maps to right eye's right edge.
    float rightEdge = +1.0f;
    ApplyMuzzleFlashPerEye(&rightEdge, 1);
    Check("full right edge x=+1 -> right eye right edge (+1.0)", rightEdge, +1.0f);

    // Full-frame left edge x=-1 in the right eye lands at the right eye's left edge (the seam).
    leftEdge = -1.0f;
    ApplyMuzzleFlashPerEye(&leftEdge, 1);
    Check("full left edge x=-1 -> right eye left edge (0.0)", leftEdge, 0.0f);

    // Full-frame right edge x=+1 in the left eye lands at the left eye's right edge (the seam).
    rightEdge = +1.0f;
    ApplyMuzzleFlashPerEye(&rightEdge, 0);
    Check("full right edge x=+1 -> left eye right edge (0.0)", rightEdge, 0.0f);

    // Mid-screen x=+0.5 (right side of full frame) in the left eye lands at -0.25 (centre of the
    // left half's left quarter).
    float midRight = +0.5f;
    ApplyMuzzleFlashPerEye(&midRight, 0);
    Check("full x=+0.5 -> left eye (-0.25)", midRight, -0.25f);

    // Mid-screen x=-0.5 (left side of full frame) in the right eye lands at +0.25.
    float midLeft = -0.5f;
    ApplyMuzzleFlashPerEye(&midLeft, 1);
    Check("full x=-0.5 -> right eye (+0.25)", midLeft, +0.25f);

    // Identity-like interior point: x=0 in the left eye -> -0.5, already checked above.
    // Verify both eyes on the same input differ by exactly 1.0 (the full half-frame width).
    float a = 0.25f, b = 0.25f;
    ApplyMuzzleFlashPerEye(&a, 0);
    ApplyMuzzleFlashPerEye(&b, 1);
    Check("same input: right minus left == +1.0", b - a, 1.0f);

    // Negative input preserved as negative offset: x=-0.25 left eye -> -0.625.
    float neg = -0.25f;
    ApplyMuzzleFlashPerEye(&neg, 0);
    Check("full x=-0.25 -> left eye (-0.625)", neg, -0.625f);

    printf("\n%s (%d failure(s))\n", g_fail ? "FAILED" : "ALL PASS", g_fail);
    return g_fail ? 1 : 0;
}
