// SingularityVR OLED Color Grading Compute Shader
// Display-referred SDR color enhancement optimized for PSVR2 OLED displays.
// Preserves stereo symmetry, alpha, neutral grays, and highlight detail.

Texture2D<float4> g_Input : register(t0);
RWTexture2D<float4> g_Output : register(u0);

cbuffer OledParams : register(b0)
{
    uint  g_Preset;
    float g_Contrast;
    float g_ShadowToe;
    float g_Vibrance;

    float g_E99CyanBoost;
    float g_AmberWarmBoost;
    float g_RedAlarmBoost;
    float g_HighlightShoulder;

    uint  g_Width;
    uint  g_Height;
    float g_RcpWidth;
    float g_RcpHeight;
};

// Exact IEC 61966-2-1 sRGB to Linear conversion
float3 SrgbToLinear(float3 c)
{
    float3 low = c / 12.92f;
    float3 high = pow(max((c + 0.055f) / 1.055f, 0.0f), 2.4f);
    return (c <= 0.04045f) ? low : high;
}

// Exact IEC 61966-2-1 Linear to sRGB conversion
float3 LinearToSrgb(float3 c)
{
    c = saturate(c);
    float3 low = c * 12.92f;
    float3 high = 1.055f * pow(max(c, 0.0f), 1.0f / 2.4f) - 0.055f;
    return (c <= 0.0031308f) ? low : high;
}

// Standard Rec.709 Linear Luminance weights
float LinearLuminance(float3 c)
{
    return dot(c, float3(0.2126f, 0.7152f, 0.0722f));
}

[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= g_Width || dispatchThreadId.y >= g_Height)
        return;

    float4 inPixel = g_Input[dispatchThreadId.xy];
    float alpha = inPixel.a;
    float3 inSrgb = saturate(inPixel.rgb);

    // If preset is Off / Original, bit-identical identity pass
    if (g_Preset == 0)
    {
        g_Output[dispatchThreadId.xy] = float4(inSrgb, alpha);
        return;
    }

    // 1. Decode sRGB to Linear Light
    float3 lin = SrgbToLinear(inSrgb);
    float Y = LinearLuminance(lin);

    // 2. Controlled Shadow Toe (OLED Black level enhancement without black crush)
    // Applied in perceptual gamma space (~2.2) to match human visual response
    float v = pow(max(Y, 0.0f), 1.0f / 2.2f);

    // Shadow weight: smooth taper from deep shadows to midtones (v ~ 0.5)
    float shadowWeight = saturate(1.0f - v / 0.5f);
    shadowWeight = shadowWeight * shadowWeight;
    float effectiveToe = 1.0f + (g_ShadowToe - 1.0f) * shadowWeight;
    float vShadow = pow(max(v, 0.0f), effectiveToe);

    // 3. Midtone S-curve Contrast with Highlight Preservation
    // Monotonic cubic S-curve around midtone 0.5
    float sCurve = vShadow * vShadow * (3.0f - 2.0f * vShadow) - vShadow;

    // Highlight shoulder roll-off to prevent clipping
    float shoulderRollOff = 1.0f;
    if (vShadow > g_HighlightShoulder)
    {
        float t = saturate((vShadow - g_HighlightShoulder) / max(1.0f - g_HighlightShoulder, 1e-4f));
        shoulderRollOff = 1.0f - t * t;
    }
    float vGraded = vShadow + g_Contrast * sCurve * shoulderRollOff;
    vGraded = saturate(vGraded);

    // Convert graded perceptual luminance back to linear
    float YGraded = pow(max(vGraded, 0.0f), 2.2f);

    // Apply luminance adjustment while strictly preserving chromaticity ratios
    float3 linGraded = (Y > 1e-6f) ? (lin * (YGraded / Y)) : float3(0.0f, 0.0f, 0.0f);

    // 4. Selective Vibrance & Target Color Boosts
    float cMax = max(linGraded.r, max(linGraded.g, linGraded.b));
    float cMin = min(linGraded.r, min(linGraded.g, linGraded.b));
    float delta = cMax - cMin;
    float sat = (cMax > 1e-5f) ? (delta / cMax) : 0.0f;

    // Vibrance factor: boosts muted colors, protects saturated colors from neon blowout
    float vibFactor = g_Vibrance * (1.0f - sat);

    // Hue detection for selective targets (E99 cyan, amber lighting, red alarms)
    float cyanWeight = 0.0f;
    float amberWeight = 0.0f;
    float redWeight = 0.0f;

    if (delta > 1e-5f)
    {
        float hue = 0.0f;
        if (cMax == linGraded.r)
        {
            hue = (linGraded.g - linGraded.b) / delta;
            if (hue < 0.0f) hue += 6.0f;
        }
        else if (cMax == linGraded.g)
        {
            hue = 2.0f + (linGraded.b - linGraded.r) / delta;
        }
        else // cMax == linGraded.b
        {
            hue = 4.0f + (linGraded.r - linGraded.g) / delta;
        }

        // Cyan E99: peak at hue ~ 3.0
        float cyanDist = abs(hue - 3.0f);
        cyanWeight = saturate(1.0f - cyanDist / 0.8f);
        cyanWeight = cyanWeight * cyanWeight;

        // Amber warm lighting: peak at hue ~ 0.7
        float amberDist = abs(hue - 0.7f);
        amberWeight = saturate(1.0f - amberDist / 0.6f);
        amberWeight = amberWeight * amberWeight;

        // Red alarm: peak at hue ~ 0.0 or 6.0
        float redDist = min(hue, 6.0f - hue);
        redWeight = saturate(1.0f - redDist / 0.7f);
        redWeight = redWeight * redWeight;
    }

    float totalSatBoost = vibFactor
                        + g_E99CyanBoost * cyanWeight
                        + g_AmberWarmBoost * amberWeight
                        + g_RedAlarmBoost * redWeight;

    float YOut = LinearLuminance(linGraded);
    float3 satAdjusted = lerp(float3(YOut, YOut, YOut), linGraded, 1.0f + totalSatBoost);

    // Subtle E99 luminance glow boost for cyan highlights
    satAdjusted += satAdjusted * (g_E99CyanBoost * 0.25f * cyanWeight * smoothstep(0.05f, 0.4f, YOut));

    satAdjusted = max(satAdjusted, 0.0f);

    // 5. Re-encode to sRGB and write output
    float3 outSrgb = LinearToSrgb(satAdjusted);
    g_Output[dispatchThreadId.xy] = float4(saturate(outSrgb), alpha);
}
