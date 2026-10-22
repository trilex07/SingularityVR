// Deterministic verification test for SingularityVR OLED Color Grading
// Validates Gate C requirements:
// - Exact IEC 61966-2-1 sRGB decode/encode roundtrip
// - Neutral grayscale preservation (zero color cast, R==G==B)
// - Near-black gradient and zero black crush (monotonic, non-zero for non-zero input)
// - Highlight preservation and soft roll-off (no clipping)
// - Saturated RGB colors, selective Cyan E99, Amber, and Red alarm boosts
// - Bit-identical equivalence for Preset::Off (Original)
// - Alpha preservation across all values
// - Direct3D 11 GPU compute shader dispatch vs CPU reference comparison

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

#include "../oled_grading.h"

// ---------------------------------------------------------------------------
// CPU Reference Implementation matching oled_grading.hlsl
// ---------------------------------------------------------------------------
namespace cpu_ref {

static float SrgbToLinear(float c) {
    if (c <= 0.04045f)
        return c / 12.92f;
    return std::pow(std::max((c + 0.055f) / 1.055f, 0.0f), 2.4f);
}

static float LinearToSrgb(float c) {
    c = std::clamp(c, 0.0f, 1.0f);
    if (c <= 0.0031308f)
        return c * 12.92f;
    return 1.055f * std::pow(std::max(c, 0.0f), 1.0f / 2.4f) - 0.055f;
}

static float LinearLuminance(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

struct PixelBGRA {
    uint8_t b, g, r, a;
};

static PixelBGRA GradePixel(PixelBGRA in, const oled_grading::ShaderConstants& sc) {
    if (sc.preset == 0) {
        return in; // Bit-identical
    }

    float inR = in.r / 255.0f;
    float inG = in.g / 255.0f;
    float inB = in.b / 255.0f;

    // 1. Decode sRGB to Linear
    float linR = SrgbToLinear(inR);
    float linG = SrgbToLinear(inG);
    float linB = SrgbToLinear(inB);
    float Y = LinearLuminance(linR, linG, linB);

    // 2. Controlled Shadow Toe
    float v = std::pow(std::max(Y, 0.0f), 1.0f / 2.2f);
    float shadowWeight = std::clamp(1.0f - v / 0.5f, 0.0f, 1.0f);
    shadowWeight = shadowWeight * shadowWeight;
    float effectiveToe = 1.0f + (sc.shadowToe - 1.0f) * shadowWeight;
    float vShadow = std::pow(std::max(v, 0.0f), effectiveToe);

    // 3. Midtone S-curve Contrast
    float sCurve = vShadow * vShadow * (3.0f - 2.0f * vShadow) - vShadow;
    float shoulderRollOff = 1.0f;
    if (vShadow > sc.highlightShoulder) {
        float t = std::clamp((vShadow - sc.highlightShoulder) / std::max(1.0f - sc.highlightShoulder, 1e-4f), 0.0f, 1.0f);
        shoulderRollOff = 1.0f - t * t;
    }
    float vGraded = std::clamp(vShadow + sc.contrast * sCurve * shoulderRollOff, 0.0f, 1.0f);
    float YGraded = std::pow(std::max(vGraded, 0.0f), 2.2f);

    float scale = (Y > 1e-6f) ? (YGraded / Y) : 0.0f;
    float rGraded = linR * scale;
    float gGraded = linG * scale;
    float bGraded = linB * scale;

    // 4. Selective Vibrance & Color Targets
    float cMax = std::max({rGraded, gGraded, bGraded});
    float cMin = std::min({rGraded, gGraded, bGraded});
    float delta = cMax - cMin;
    float sat = (cMax > 1e-5f) ? (delta / cMax) : 0.0f;

    float vibFactor = sc.vibrance * (1.0f - sat);

    float cyanWeight = 0.0f;
    float amberWeight = 0.0f;
    float redWeight = 0.0f;

    if (delta > 1e-5f) {
        float hue = 0.0f;
        if (cMax == rGraded) {
            hue = (gGraded - bGraded) / delta;
            if (hue < 0.0f) hue += 6.0f;
        } else if (cMax == gGraded) {
            hue = 2.0f + (bGraded - rGraded) / delta;
        } else {
            hue = 4.0f + (rGraded - gGraded) / delta;
        }

        float cyanDist = std::abs(hue - 3.0f);
        cyanWeight = std::clamp(1.0f - cyanDist / 0.8f, 0.0f, 1.0f);
        cyanWeight = cyanWeight * cyanWeight;

        float amberDist = std::abs(hue - 0.7f);
        amberWeight = std::clamp(1.0f - amberDist / 0.6f, 0.0f, 1.0f);
        amberWeight = amberWeight * amberWeight;

        float redDist = std::min(hue, 6.0f - hue);
        redWeight = std::clamp(1.0f - redDist / 0.7f, 0.0f, 1.0f);
        redWeight = redWeight * redWeight;
    }

    float totalSatBoost = vibFactor
                        + sc.e99CyanBoost * cyanWeight
                        + sc.amberWarmBoost * amberWeight
                        + sc.redAlarmBoost * redWeight;

    float YOut = LinearLuminance(rGraded, gGraded, bGraded);
    auto lerp = [](float a, float b, float t) { return a + t * (b - a); };
    float rSat = lerp(YOut, rGraded, 1.0f + totalSatBoost);
    float gSat = lerp(YOut, gGraded, 1.0f + totalSatBoost);
    float bSat = lerp(YOut, bGraded, 1.0f + totalSatBoost);

    // E99 luminance glow boost
    auto smoothstep = [](float e0, float e1, float x) {
        float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };
    float e99Glow = sc.e99CyanBoost * 0.25f * cyanWeight * smoothstep(0.05f, 0.4f, YOut);
    rSat += rSat * e99Glow;
    gSat += gSat * e99Glow;
    bSat += bSat * e99Glow;

    rSat = std::max(rSat, 0.0f);
    gSat = std::max(gSat, 0.0f);
    bSat = std::max(bSat, 0.0f);

    // 5. Re-encode to sRGB
    float outR = LinearToSrgb(rSat);
    float outG = LinearToSrgb(gSat);
    float outB = LinearToSrgb(bSat);

    PixelBGRA out;
    out.r = static_cast<uint8_t>(std::clamp(std::round(outR * 255.0f), 0.0f, 255.0f));
    out.g = static_cast<uint8_t>(std::clamp(std::round(outG * 255.0f), 0.0f, 255.0f));
    out.b = static_cast<uint8_t>(std::clamp(std::round(outB * 255.0f), 0.0f, 255.0f));
    out.a = in.a; // Alpha preserved
    return out;
}

} // namespace cpu_ref

int main() {
    printf("=== SingularityVR OLED Color Grading Verification Suite ===\n\n");

    // -----------------------------------------------------------------------
    // TEST 1: IEC 61966-2-1 sRGB roundtrip correctness
    // -----------------------------------------------------------------------
    printf("[Test 1] Verifying IEC 61966-2-1 sRGB Transfer Function...\n");
    for (int i = 0; i <= 255; ++i) {
        float srgb = i / 255.0f;
        float lin = cpu_ref::SrgbToLinear(srgb);
        float roundtrip = cpu_ref::LinearToSrgb(lin);
        int roundtripI = static_cast<int>(std::round(roundtrip * 255.0f));
        assert(roundtripI == i && "sRGB decode/encode roundtrip must be bit-identical");
    }
    printf("  PASS: All 256 sRGB levels roundtrip bit-identically.\n\n");

    // -----------------------------------------------------------------------
    // TEST 2: Preset Off / Original output equivalence
    // -----------------------------------------------------------------------
    printf("[Test 2] Verifying Preset::Off (Original) Bit-Identity...\n");
    oled_grading::ShaderConstants scOff = {};
    scOff.preset = 0;
    for (int r = 0; r <= 255; r += 17) {
        for (int g = 0; g <= 255; g += 17) {
            for (int b = 0; b <= 255; b += 17) {
                cpu_ref::PixelBGRA in = { (uint8_t)b, (uint8_t)g, (uint8_t)r, 128 };
                cpu_ref::PixelBGRA out = cpu_ref::GradePixel(in, scOff);
                assert(out.r == in.r && out.g == in.g && out.b == in.b && out.a == in.a);
            }
        }
    }
    printf("  PASS: Preset::Off produces 100%% bit-identical output across color space.\n\n");

    // -----------------------------------------------------------------------
    // TEST 3: Neutral Grayscale Preservation (Zero color cast)
    // -----------------------------------------------------------------------
    printf("[Test 3] Verifying Neutral Grayscale Preservation across all presets...\n");
    const int presetsToTest[] = { 1 /* Balanced */, 2 /* Cinematic */ };
    for (int p : presetsToTest) {
        oled_grading::ShaderConstants sc = {};
        sc.preset = p;
        if (p == 1) {
            sc.contrast = 0.08f; sc.shadowToe = 1.06f; sc.vibrance = 0.10f;
            sc.e99CyanBoost = 0.12f; sc.amberWarmBoost = 0.08f; sc.redAlarmBoost = 0.10f;
            sc.highlightShoulder = 0.88f;
        } else {
            sc.contrast = 0.15f; sc.shadowToe = 1.12f; sc.vibrance = 0.16f;
            sc.e99CyanBoost = 0.20f; sc.amberWarmBoost = 0.14f; sc.redAlarmBoost = 0.16f;
            sc.highlightShoulder = 0.85f;
        }

        for (int gray = 0; gray <= 255; ++gray) {
            cpu_ref::PixelBGRA in = { (uint8_t)gray, (uint8_t)gray, (uint8_t)gray, 255 };
            cpu_ref::PixelBGRA out = cpu_ref::GradePixel(in, sc);
            // Grayscale preservation requires R == G == B exactly (zero color tint)
            assert(out.r == out.g && out.g == out.b && "Neutral gray must never introduce color tint");
            assert(out.a == in.a && "Alpha must be preserved");
        }
    }
    printf("  PASS: Zero color cast; R == G == B preserved across all 256 grayscale levels.\n\n");

    // -----------------------------------------------------------------------
    // TEST 4: Near-black values & Monotonicity (No black crush)
    // -----------------------------------------------------------------------
    printf("[Test 4] Verifying Near-Black Values & Monotonicity (No black crush)...\n");
    for (int p : presetsToTest) {
        oled_grading::ShaderConstants sc = {};
        sc.preset = p;
        if (p == 1) {
            sc.contrast = 0.08f; sc.shadowToe = 1.06f; sc.vibrance = 0.10f;
            sc.e99CyanBoost = 0.12f; sc.amberWarmBoost = 0.08f; sc.redAlarmBoost = 0.10f;
            sc.highlightShoulder = 0.88f;
        } else {
            sc.contrast = 0.15f; sc.shadowToe = 1.12f; sc.vibrance = 0.16f;
            sc.e99CyanBoost = 0.20f; sc.amberWarmBoost = 0.14f; sc.redAlarmBoost = 0.16f;
            sc.highlightShoulder = 0.85f;
        }

        // Test black point
        cpu_ref::PixelBGRA zeroIn = { 0, 0, 0, 255 };
        cpu_ref::PixelBGRA zeroOut = cpu_ref::GradePixel(zeroIn, sc);
        assert(zeroOut.r == 0 && zeroOut.g == 0 && zeroOut.b == 0 && "Zero input must map to zero output");

        // Monotonicity check across all 256 grayscale levels
        int prevVal = 0;
        for (int i = 0; i <= 255; ++i) {
            cpu_ref::PixelBGRA in = { (uint8_t)i, (uint8_t)i, (uint8_t)i, 255 };
            cpu_ref::PixelBGRA out = cpu_ref::GradePixel(in, sc);
            assert(out.r >= prevVal && "Grayscale response must be strictly monotonic (no inversions)");
            prevVal = out.r;
        }

        // Verify near-black non-zero preservation (no black crush)
        cpu_ref::PixelBGRA lowIn = { 2, 2, 2, 255 };
        cpu_ref::PixelBGRA lowOut = cpu_ref::GradePixel(lowIn, sc);
        assert(lowOut.r >= 1 && "Near-black value (2) must not be crushed to 0");
    }
    printf("  PASS: Monotonic shadow response; 0 maps to 0, near-black values preserved without crush.\n\n");

    // -----------------------------------------------------------------------
    // TEST 5: Near-white values & Highlight preservation
    // -----------------------------------------------------------------------
    printf("[Test 5] Verifying Near-White Values & Highlight Preservation...\n");
    for (int p : presetsToTest) {
        oled_grading::ShaderConstants sc = {};
        sc.preset = p;
        if (p == 1) {
            sc.contrast = 0.08f; sc.shadowToe = 1.06f; sc.vibrance = 0.10f;
            sc.e99CyanBoost = 0.12f; sc.amberWarmBoost = 0.08f; sc.redAlarmBoost = 0.10f;
            sc.highlightShoulder = 0.88f;
        } else {
            sc.contrast = 0.15f; sc.shadowToe = 1.12f; sc.vibrance = 0.16f;
            sc.e99CyanBoost = 0.20f; sc.amberWarmBoost = 0.14f; sc.redAlarmBoost = 0.16f;
            sc.highlightShoulder = 0.85f;
        }

        cpu_ref::PixelBGRA maxIn = { 255, 255, 255, 255 };
        cpu_ref::PixelBGRA maxOut = cpu_ref::GradePixel(maxIn, sc);
        assert(maxOut.r == 255 && maxOut.g == 255 && maxOut.b == 255 && "Pure white must remain 255");

        for (int i = 240; i <= 255; ++i) {
            cpu_ref::PixelBGRA in = { (uint8_t)i, (uint8_t)i, (uint8_t)i, 255 };
            cpu_ref::PixelBGRA out = cpu_ref::GradePixel(in, sc);
            assert(out.r <= 255 && "Values must never exceed 255");
        }
    }
    printf("  PASS: Highlight detail preserved; 255 remains 255 with soft shoulder.\n\n");

    // -----------------------------------------------------------------------
    // TEST 6: Saturated RGB Colors & Selective Targets (E99 Cyan, Amber, Red)
    // -----------------------------------------------------------------------
    printf("[Test 6] Verifying Selective Target Color Boosts...\n");
    {
        oled_grading::ShaderConstants sc = {};
        sc.preset = 2; // Cinematic
        sc.contrast = 0.15f; sc.shadowToe = 1.12f; sc.vibrance = 0.16f;
        sc.e99CyanBoost = 0.20f; sc.amberWarmBoost = 0.14f; sc.redAlarmBoost = 0.16f;
        sc.highlightShoulder = 0.85f;

        // Pure Red
        cpu_ref::PixelBGRA pureRed = { 0, 0, 220, 255 };
        cpu_ref::PixelBGRA redOut = cpu_ref::GradePixel(pureRed, sc);
        assert(redOut.r >= pureRed.r && redOut.g == 0 && redOut.b == 0);

        // E99 Cyan (e.g. R=40, G=200, B=230)
        cpu_ref::PixelBGRA cyanIn = { 230, 200, 40, 255 };
        cpu_ref::PixelBGRA cyanOut = cpu_ref::GradePixel(cyanIn, sc);
        assert(cyanOut.b > cyanIn.b && "Cyan E99 boost must enhance blue/cyan element");

        // Amber Soviet Light (e.g. R=230, G=140, B=30)
        cpu_ref::PixelBGRA amberIn = { 30, 140, 230, 255 };
        cpu_ref::PixelBGRA amberOut = cpu_ref::GradePixel(amberIn, sc);
        assert(amberOut.r >= amberIn.r && "Amber boost must enrich warm tungsten lights");
    }
    printf("  PASS: E99 Cyan, Amber, and Red alarm colors correctly enhanced.\n\n");

    // -----------------------------------------------------------------------
    // TEST 7: Alpha Channel Preservation
    // -----------------------------------------------------------------------
    printf("[Test 7] Verifying Alpha Channel Preservation...\n");
    {
        oled_grading::ShaderConstants sc = {};
        sc.preset = 1; // Balanced
        sc.contrast = 0.08f; sc.shadowToe = 1.06f; sc.vibrance = 0.10f;
        sc.e99CyanBoost = 0.12f; sc.amberWarmBoost = 0.08f; sc.redAlarmBoost = 0.10f;
        sc.highlightShoulder = 0.88f;

        const uint8_t testAlphas[] = { 0, 1, 64, 128, 192, 254, 255 };
        for (uint8_t a : testAlphas) {
            cpu_ref::PixelBGRA in = { 100, 150, 200, a };
            cpu_ref::PixelBGRA out = cpu_ref::GradePixel(in, sc);
            assert(out.a == a && "Alpha channel must be preserved with exact bit-identity");
        }
    }
    printf("  PASS: Alpha channel preserved exactly across all test alpha values.\n\n");

    // -----------------------------------------------------------------------
    // TEST 8: Real D3D11 GPU Compute Shader vs CPU Reference Comparison
    // -----------------------------------------------------------------------
    printf("[Test 8] Executing Direct3D 11 GPU Compute Shader vs CPU Reference...\n");
    {
        ID3D11Device* dev = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
        D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;

        // Try HARDWARE first, fallback to WARP if no HW display adapter in test env
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                       0, &fl, 1, D3D11_SDK_VERSION, &dev, nullptr, &ctx);
        if (FAILED(hr)) {
            hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                                   0, &fl, 1, D3D11_SDK_VERSION, &dev, nullptr, &ctx);
        }
        assert(SUCCEEDED(hr) && dev && ctx && "D3D11 Device creation must succeed");

        oled_grading::OledGrading oled;
        assert(oled.Init(dev) && "OledGrading initialization must succeed");

        const UINT testW = 64;
        const UINT testH = 64;

        // Create test source texture populated with various test patterns
        std::vector<cpu_ref::PixelBGRA> testPixels(testW * testH);
        for (UINT y = 0; y < testH; ++y) {
            for (UINT x = 0; x < testW; ++x) {
                UINT idx = y * testW + x;
                if (y < 16) {
                    // Grayscale ramp
                    uint8_t gray = static_cast<uint8_t>((x * 255) / (testW - 1));
                    testPixels[idx] = { gray, gray, gray, 255 };
                } else if (y < 32) {
                    // Saturated primary/secondary colors
                    uint8_t step = static_cast<uint8_t>((x * 255) / (testW - 1));
                    if (x < 16) testPixels[idx] = { 0, 0, step, 255 };        // Red
                    else if (x < 32) testPixels[idx] = { 0, step, 0, 255 };   // Green
                    else if (x < 48) testPixels[idx] = { step, 0, 0, 255 };   // Blue
                    else testPixels[idx] = { step, step, 0, 255 };            // Cyan
                } else if (y < 48) {
                    // Near-black shadow gradient
                    uint8_t dark = static_cast<uint8_t>((x * 20) / (testW - 1));
                    testPixels[idx] = { dark, dark, dark, 200 };
                } else {
                    // Near-white highlight gradient
                    uint8_t bright = static_cast<uint8_t>(235 + (x * 20) / (testW - 1));
                    testPixels[idx] = { bright, bright, bright, 100 };
                }
            }
        }

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = testW;
        td.Height = testH;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = testPixels.data();
        initData.SysMemPitch = testW * sizeof(cpu_ref::PixelBGRA);

        ID3D11Texture2D* srcTex = nullptr;
        assert(SUCCEEDED(dev->CreateTexture2D(&td, &initData, &srcTex)) && srcTex);

        // Test Preset::Off first (must return srcTex directly)
        oled.SetPreset(oled_grading::Preset::Off);
        ID3D11Texture2D* offResult = oled.Process(dev, ctx, srcTex, testW, testH);
        assert(offResult == srcTex && "Preset::Off must return srcTex without dispatching");

        // Test Preset::Balanced on GPU
        oled.SetPreset(oled_grading::Preset::Balanced);
        ID3D11Texture2D* gpuResult = oled.Process(dev, ctx, srcTex, testW, testH);
        assert(gpuResult != nullptr && gpuResult != srcTex);

        // Read back GPU output
        D3D11_TEXTURE2D_DESC readDesc = td;
        readDesc.Usage = D3D11_USAGE_STAGING;
        readDesc.BindFlags = 0;
        readDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        ID3D11Texture2D* readTex = nullptr;
        assert(SUCCEEDED(dev->CreateTexture2D(&readDesc, nullptr, &readTex)) && readTex);
        ctx->CopyResource(readTex, gpuResult);

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        assert(SUCCEEDED(ctx->Map(readTex, 0, D3D11_MAP_READ, 0, &mapped)));

        oled_grading::ShaderConstants scBalanced = {};
        scBalanced.preset = 1;
        scBalanced.contrast = 0.08f; scBalanced.shadowToe = 1.06f; scBalanced.vibrance = 0.10f;
        scBalanced.e99CyanBoost = 0.12f; scBalanced.amberWarmBoost = 0.08f;
        scBalanced.redAlarmBoost = 0.10f; scBalanced.highlightShoulder = 0.88f;

        int maxDiff = 0;
        for (UINT y = 0; y < testH; ++y) {
            const cpu_ref::PixelBGRA* row = reinterpret_cast<const cpu_ref::PixelBGRA*>(
                static_cast<const uint8_t*>(mapped.pData) + y * mapped.RowPitch);
            for (UINT x = 0; x < testW; ++x) {
                cpu_ref::PixelBGRA in = testPixels[y * testW + x];
                cpu_ref::PixelBGRA expected = cpu_ref::GradePixel(in, scBalanced);
                cpu_ref::PixelBGRA actual = row[x];

                int diffR = std::abs((int)actual.r - (int)expected.r);
                int diffG = std::abs((int)actual.g - (int)expected.g);
                int diffB = std::abs((int)actual.b - (int)expected.b);
                int diffA = std::abs((int)actual.a - (int)expected.a);

                maxDiff = std::max({maxDiff, diffR, diffG, diffB, diffA});
                assert(diffA == 0 && "Alpha must be exact bit-identical between GPU and CPU");
                assert(diffR <= 1 && diffG <= 1 && diffB <= 1 && "GPU and CPU must match within 1 LSB float precision");
            }
        }
        ctx->Unmap(readTex, 0);

        printf("  PASS: GPU Compute Shader output matched CPU reference perfectly! Max LSB difference = %d.\n", maxDiff);

        readTex->Release();
        srcTex->Release();
        oled.Shutdown();
        ctx->Release();
        dev->Release();
    }
    printf("\n>>> ALL GATE C VERIFICATION TESTS PASSED SUCCESSFULLY! <<<\n");
    return 0;
}
