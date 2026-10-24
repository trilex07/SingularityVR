#pragma once
#include <cstdint>
#include <cmath>

// Read-only draw census. A lower-left orthographic batch is a candidate, not
// proof that it contains a location label. MAIN correlates its timestamp with
// a controlled entry into an area before any placement rule is introduced.
namespace gameplaytextprobe {
inline bool Candidate(bool enabled, bool gameplay, bool menu, bool sceneOrOffscreen,
                      unsigned stride, unsigned primitives, const float* m) {
    if (!enabled || !gameplay || menu || !sceneOrOffscreen || !m ||
        primitives < 2 || primitives > 300 || stride < 4 || stride > 32) return false;
    if (std::fabs(m[3]) > 0.0001f || std::fabs(m[7]) > 0.0001f ||
        std::fabs(m[11]) > 0.0001f || std::fabs(m[15] - 1.f) > 0.001f) return false;
    return std::isfinite(m[12]) && std::isfinite(m[13]) &&
           m[12] < -0.35f && m[13] < -0.35f;
}

inline std::uint64_t Key(unsigned route, unsigned stride, unsigned primitives,
                         int x100, int y100, std::uint32_t texture) {
    std::uint64_t h = 14695981039346656037ull;
    const std::uint32_t words[] = {route, stride, primitives,
        static_cast<std::uint32_t>(x100), static_cast<std::uint32_t>(y100), texture};
    for (auto word : words) for (int i = 0; i < 4; ++i) {
        h ^= static_cast<unsigned char>(word >> (i * 8));
        h *= 1099511628211ull;
    }
    return h;
}

struct UniqueBudget {
    static constexpr unsigned Limit = 96;
    std::uint64_t seen[Limit]{};
    unsigned used = 0;
    bool Admit(std::uint64_t key) {
        for (unsigned i = 0; i < used; ++i) if (seen[i] == key) return false;
        if (used == Limit) return false;
        seen[used++] = key;
        return true;
    }
};
}
