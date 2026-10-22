#pragma once
// Pure witness gating (offline-testable). Keeps first-level exec logging distinct from nested reentry.

namespace LockerNativeTraceWitness {

// Detour calls LogEnter/LogExit only on the outermost native invocation.
inline constexpr bool AllowOuterExecWitness(int reenterDepthAfterIncrement) {
    return reenterDepthAfterIncrement == 1;
}

// Previous bug: refusing whenever reenterDepth > 0 blocked the first level (depth==1).
inline constexpr bool BuggyLogEnterRefused(int reenterDepthAfterIncrement) {
    return reenterDepthAfterIncrement > 0;
}

inline constexpr bool LogEnterPermitted(int reenterDepthAfterIncrement) {
    return reenterDepthAfterIncrement == 1;
}

// Nested native reentry (depth >= 2) must not allocate budget or emit ENTER/EXIT pairs.
inline constexpr bool RefuseNestedExecWitness(int reenterDepthAfterIncrement) {
    return reenterDepthAfterIncrement > 1;
}

} // namespace LockerNativeTraceWitness
