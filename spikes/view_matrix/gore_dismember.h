#pragma once

// =============================================================================
// Singularity VR - Native Gore & Dismemberment Subsystem
//
// Unlocks and tunes the native limb-severing and gore mechanics built into UE3
// by Raven Software:
//   1. Intercepts AllowGore() -> 1 (true) and UseLowGore() -> 0 (false) in
//      ProcessInternal to unconditionally bypass regional/censorship locks.
//   2. Dynamically resolves RvSharedPawn reflection properties:
//        - mbAutomaticDismembermentOnDeath (auto-sever limbs on lethal damage)
//        - mSeverAllLimbsOnImpulseDeath (complete dismemberment via TMD impulse)
//        - mbPlaySeveredLimbAnimation (flail/reaction animations with missing limbs)
//   3. Dynamically resolves RvLimbSettings properties:
//        - mRandomChanceToSeverLimb (forces chance from -1.0f default to 1.0f)
//        - mHealthPercentToTryRandomSever (forces threshold to 1.0f)
//   4. Patches both Class Default Objects (CDOs) and live pawn instances on TakeDamage.
// =============================================================================

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cmath>

namespace singularity_gore {

// Configuration (loaded from SingularityVR.ini [Gameplay])
static volatile LONG g_goreEnabled            = 1;
static volatile LONG g_goreTmdImpulseSeverAll = 1;
static volatile LONG g_goreAutoOnDeath        = 1;
static volatile LONG g_goreChancePct          = 100;

// Telemetry
static volatile LONG g_allowGoreIntercepts    = 0;
static volatile LONG g_useLowGoreIntercepts   = 0;
static volatile LONG g_pawnsPatched           = 0;
static volatile LONG g_cdoPatched             = 0;
static volatile LONG g_tmdImpulseKills        = 0;

struct GoreReflection {
    bool initialized = false;

    // RvSharedPawn offsets & bitmasks
    int offAutoDismember = -1;
    uint32_t maskAutoDismember = 0;

    int offSeverAllImpulse = -1;
    uint32_t maskSeverAllImpulse = 0;

    int offPlaySeveredAnim = -1;
    uint32_t maskPlaySeveredAnim = 0;

    int offLimbSettings = -1;
    int offAltLimbSettings = -1;

    // RvLimbSettings offsets
    int offRandomChance = -1;
    int offHealthPercent = -1;
};

static GoreReflection g_ref{};

inline uintptr_t FindObjectByName(const char* want) {
    if (!Readable((void*)kGObjectsTArray, 12)) return 0;
    uintptr_t data = *reinterpret_cast<uintptr_t*>(kGObjectsTArray);
    int32_t count = *reinterpret_cast<int32_t*>(kGObjectsTArray + 4);
    if (!data || count <= 0) return 0;
    uintptr_t* objs = reinterpret_cast<uintptr_t*>(data);
    char nm[80]{};
    for (int32_t i = 0; i < count; ++i) {
        uintptr_t o = objs[i];
        if (!o || !Readable((void*)o, 0x40)) continue;
        if (NameOf(o, nm, sizeof(nm)) && !_stricmp(nm, want)) {
            return o;
        }
    }
    return 0;
}

inline bool FindPropInChain(uintptr_t cls, const char* name, int* outOff, uint32_t* outMask = nullptr) {
    if (!cls || !g_upChildren || !g_upNext || !g_upOffset) return false;
    const int32_t wantIdx = WantedIdx(name);
    if (wantIdx < 0) return false;
    uintptr_t c = cls;
    for (int depth = 0; c && depth < 64; ++depth) {
        if (!Readable((void*)(c + g_upChildren), 4)) break;
        uintptr_t p = *reinterpret_cast<uintptr_t*>(c + g_upChildren);
        for (int guard = 0; guard < 400 && p; ++guard) {
            if (!Readable((void*)p, 0x100)) break;
            if (*reinterpret_cast<const int32_t*>(p + OBJ_NAME) == wantIdx) {
                const int off = *reinterpret_cast<const int32_t*>(p + g_upOffset);
                if (off >= 0 && off <= 0x8000) {
                    if (outOff) *outOff = off;
                    if (outMask) *outMask = *reinterpret_cast<const uint32_t*>(p + g_upOffset + 4);
                    return true;
                }
            }
            p = *reinterpret_cast<uintptr_t*>(p + g_upNext);
        }
        if (!Readable((void*)(c + g_upNext - 4), 4)) break;
        c = *reinterpret_cast<uintptr_t*>(c + g_upNext - 4); // SuperField
    }
    return false;
}

inline void PatchClassDefaults() {
    if (!g_goreEnabled) return;

    uintptr_t defPawn = FindObjectByName("Default__RvSharedPawn");
    if (defPawn && Readable((void*)defPawn, 0x400)) {
        if (g_ref.offAutoDismember >= 0 && g_ref.maskAutoDismember && g_goreAutoOnDeath) {
            *reinterpret_cast<uint32_t*>(defPawn + g_ref.offAutoDismember) |= g_ref.maskAutoDismember;
        }
        if (g_ref.offSeverAllImpulse >= 0 && g_ref.maskSeverAllImpulse && g_goreTmdImpulseSeverAll) {
            *reinterpret_cast<uint32_t*>(defPawn + g_ref.offSeverAllImpulse) |= g_ref.maskSeverAllImpulse;
        }
        if (g_ref.offPlaySeveredAnim >= 0 && g_ref.maskPlaySeveredAnim) {
            *reinterpret_cast<uint32_t*>(defPawn + g_ref.offPlaySeveredAnim) |= g_ref.maskPlaySeveredAnim;
        }
        InterlockedIncrement(&g_cdoPatched);
        Log("Gore: Default__RvSharedPawn CDO patched with dismemberment flags!");
    }

    uintptr_t defLimb = FindObjectByName("Default__RvLimbSettings");
    if (defLimb && Readable((void*)defLimb, 0x100)) {
        if (g_ref.offRandomChance >= 0) {
            const float chance = (float)g_goreChancePct / 100.0f;
            *reinterpret_cast<float*>(defLimb + g_ref.offRandomChance) = chance;
        }
        if (g_ref.offHealthPercent >= 0) {
            *reinterpret_cast<float*>(defLimb + g_ref.offHealthPercent) = 1.0f;
        }
        InterlockedIncrement(&g_cdoPatched);
        Log("Gore: Default__RvLimbSettings CDO patched (chance=%.2f, healthPct=1.0)!",
            (float)g_goreChancePct / 100.0f);
    }
}

inline void ResolveReflection(uintptr_t anyPawnClass) {
    if (g_ref.initialized || !g_goreEnabled) return;
    if (!anyPawnClass || !g_upChildren || !g_upNext || !g_upOffset) return;

    Log("Gore: resolving native dismemberment reflection...");

    // Resolve RvSharedPawn properties
    FindPropInChain(anyPawnClass, "mbAutomaticDismembermentOnDeath", &g_ref.offAutoDismember, &g_ref.maskAutoDismember);
    FindPropInChain(anyPawnClass, "mSeverAllLimbsOnImpulseDeath", &g_ref.offSeverAllImpulse, &g_ref.maskSeverAllImpulse);
    FindPropInChain(anyPawnClass, "mbPlaySeveredLimbAnimation", &g_ref.offPlaySeveredAnim, &g_ref.maskPlaySeveredAnim);
    FindPropInChain(anyPawnClass, "mLimbSettings", &g_ref.offLimbSettings);
    FindPropInChain(anyPawnClass, "mAlternateLimbSettings", &g_ref.offAltLimbSettings);

    Log("Gore: RvSharedPawn offsets -> autoDismember +0x%X (mask 0x%X), severAllImpulse +0x%X (mask 0x%X), limbSettings +0x%X",
        g_ref.offAutoDismember, g_ref.maskAutoDismember,
        g_ref.offSeverAllImpulse, g_ref.maskSeverAllImpulse,
        g_ref.offLimbSettings);

    // Resolve RvLimbSettings properties
    uintptr_t limbClass = FindClassByName("RvLimbSettings");
    if (limbClass) {
        FindPropInChain(limbClass, "mRandomChanceToSeverLimb", &g_ref.offRandomChance);
        FindPropInChain(limbClass, "mHealthPercentToTryRandomSever", &g_ref.offHealthPercent);
        Log("Gore: RvLimbSettings offsets -> randomChance +0x%X, healthPercent +0x%X",
            g_ref.offRandomChance, g_ref.offHealthPercent);
    } else {
        Log("Gore: RvLimbSettings class not found yet");
    }

    g_ref.initialized = (g_ref.offAutoDismember >= 0 && g_ref.offSeverAllImpulse >= 0);
    if (g_ref.initialized) {
        Log("Gore: reflection successfully initialized!");
        PatchClassDefaults();
    }
}

inline void ApplyToPawn(uintptr_t pawn) {
    if (!g_goreEnabled || !pawn || !Readable((void*)pawn, 0x400)) return;

    if (!g_ref.initialized) {
        if (Readable((void*)pawn, 0x40)) {
            uintptr_t cls = *reinterpret_cast<uintptr_t*>(pawn + OBJ_CLASS);
            ResolveReflection(cls);
        }
    }

    // Apply dismemberment flags to pawn instance
    if (g_ref.offAutoDismember >= 0 && g_ref.maskAutoDismember && g_goreAutoOnDeath) {
        *reinterpret_cast<uint32_t*>(pawn + g_ref.offAutoDismember) |= g_ref.maskAutoDismember;
    }
    if (g_ref.offSeverAllImpulse >= 0 && g_ref.maskSeverAllImpulse && g_goreTmdImpulseSeverAll) {
        *reinterpret_cast<uint32_t*>(pawn + g_ref.offSeverAllImpulse) |= g_ref.maskSeverAllImpulse;
    }
    if (g_ref.offPlaySeveredAnim >= 0 && g_ref.maskPlaySeveredAnim) {
        *reinterpret_cast<uint32_t*>(pawn + g_ref.offPlaySeveredAnim) |= g_ref.maskPlaySeveredAnim;
    }

    // Apply to attached RvLimbSettings instance if present
    if (g_ref.offLimbSettings >= 0 && Readable((void*)(pawn + g_ref.offLimbSettings), sizeof(uintptr_t))) {
        uintptr_t limb = *reinterpret_cast<uintptr_t*>(pawn + g_ref.offLimbSettings);
        if (limb && Readable((void*)limb, 0x100)) {
            if (g_ref.offRandomChance >= 0) {
                *reinterpret_cast<float*>(limb + g_ref.offRandomChance) = (float)g_goreChancePct / 100.0f;
            }
            if (g_ref.offHealthPercent >= 0) {
                *reinterpret_cast<float*>(limb + g_ref.offHealthPercent) = 1.0f;
            }
        }
    }

    const LONG n = InterlockedIncrement(&g_pawnsPatched);
    if (n <= 5 || (n % 50) == 0) {
        char nm[64]{};
        NameOf(pawn, nm, sizeof(nm));
        Log("Gore: pawn #%ld '%s' patched with dismemberment and TMD sever-all flags", n, nm[0] ? nm : "Pawn");
    }
}

inline void LoadConfig(const char* iniPath) {
    if (!iniPath) return;
    g_goreEnabled = GetPrivateProfileIntA("Gameplay", "GoreDismemberment", 1, iniPath) ? 1 : 0;
    g_goreTmdImpulseSeverAll = GetPrivateProfileIntA("Gameplay", "GoreTmdImpulseSeverAll", 1, iniPath) ? 1 : 0;
    g_goreAutoOnDeath = GetPrivateProfileIntA("Gameplay", "GoreAutoOnDeath", 1, iniPath) ? 1 : 0;
    g_goreChancePct = GetPrivateProfileIntA("Gameplay", "GoreChancePct", 100, iniPath);
    if (g_goreChancePct < 0) g_goreChancePct = 0;
    if (g_goreChancePct > 100) g_goreChancePct = 100;

    Log("ini [Gameplay]: GoreDismemberment=%ld, GoreTmdImpulseSeverAll=%ld, GoreAutoOnDeath=%ld, GoreChancePct=%ld%%",
        g_goreEnabled, g_goreTmdImpulseSeverAll, g_goreAutoOnDeath, g_goreChancePct);
}

} // namespace singularity_gore
