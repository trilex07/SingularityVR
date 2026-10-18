#pragma once
#include <cmath>
#include <cstdint>

namespace menupointer {
struct Style {
    float outerRadius = 0;
    float ringThickness = 0;
    float coreRadius = 0;
    float pulse = 1;
};

inline float PulseScale(std::uint64_t milliseconds) {
    const float phase = (float)(milliseconds % 1400u) * (6.28318530718f / 1400.0f);
    return 1.0f + 0.08f * std::sin(phase);
}

inline Style AtTime(std::uint64_t milliseconds, bool pressed, float height) {
    const float pulse = PulseScale(milliseconds);
    const float base = height > 0 ? (height / 190.0f) : 0.0f;
    const float radius = base < 9.0f ? 9.0f : (base > 19.0f ? 19.0f : base);
    const float pressScale = pressed ? 0.82f : 1.0f;
    return {radius * pulse * pressScale, 2.6f * pressScale,
            3.2f * pressScale, pulse};
}
}
