// L1: uniform letterbox fit math (no D3D).
#include <cmath>
#include <cstdio>

static void Fit(long halfW, long fullH, float margin,
                long& dstW, long& dstH, long& offX, long& offY) {
    const float srcW = (float)(halfW * 2);
    const float srcH = (float)fullH;
    const float availW = (float)halfW * (1.0f - 2.0f * margin);
    const float availH = (float)fullH * (1.0f - 2.0f * margin);
    const float scale = (availW / srcW < availH / srcH) ? (availW / srcW) : (availH / srcH);
    dstW = (long)(srcW * scale + 0.5f);
    dstH = (long)(srcH * scale + 0.5f);
    offX = (halfW - dstW) / 2;
    offY = (fullH - dstH) / 2;
}

int main() {
    long dw, dh, ox, oy;
    Fit(2632, 2684, 0.06f, dw, dh, ox, oy);
    const float aspectIn = (2632.f * 2.f) / 2684.f;
    const float aspectOut = (float)dw / (float)dh;
    if (std::fabs(aspectIn - aspectOut) > 0.02f) {
        std::fprintf(stderr, "FAIL aspect preserved in=%.4f out=%.4f\n", aspectIn, aspectOut);
        return 1;
    }
    if (ox < 0 || oy < 0 || ox + dw > 2632 || oy + dh > 2684) {
        std::fprintf(stderr, "FAIL dst outside eye half\n");
        return 1;
    }
    if (dw >= 2632 || dh >= 2684) {
        std::fprintf(stderr, "FAIL no margin\n");
        return 1;
    }
    std::puts("PASS: cinema letterbox uniform fit");
    return 0;
}
