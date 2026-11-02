#pragma once
#include <cstdint>
#include <cstring>
// Shared production registry and lookup; runtime resolution remains one-shot.
struct WantedName { const char* s; int32_t idx; };
inline WantedName g_wantNames[] = {
    { "mCombatNode", -1 }, { "mNodeName", -1 },
    { "mCurrentNodeInstigator", -1 }, { "mTarget", -1 },
    { "Location", -1 }, { "Rotation", -1 },                     // the controls
    { "mMuzzleFlash1stPerson", -1 }, { "mLoopingMuzzleFlash1stPerson", -1 },
    { "mMuzzleFlashIronsights", -1 }, { "mLoopingMuzzleFlashIronsights", -1 },
    { "mLoopingMuzzleFXEmitter", -1 },
    // Run 199: the heal window asks "can this heal do anything" before showing the arms.
    { "Health", -1 }, { "HealthMax", -1 },
    // Run 200: ...and "do I even have an item". See the note at FindHudObject.
    { "mHealthPackCount", -1 }, { "mTMDEnergy", -1 },
    { "mE99TechCount", -1 }, { "mE99VialCount", -1 },
    { "GetTMDEnergyMax", -1 }, { "sharedHUD", -1 }, { "myHUD", -1 },
    // Run 201: READ ONLY - what does the game think the player's eye height is? See ReportEyeHeight.
    { "BaseEyeHeight", -1 }, { "EyeHeight", -1 },
    // ⭐ run 227: the one question every mesh-latch heuristic has been guessing at: is a weapon EQUIPPED.
    { "Weapon", -1 },
    { "Pawn", -1 }, { "Controller", -1 }, // startup: reflected reciprocal ownership, no guessed offsets
    { "PCOwner", -1 }, // exact local Camera owner; unregistered names cannot resolve
    { "PlayerCamera", -1 }, // reciprocal local controller -> current camera
    // 💥 run 236b: THESE HAVE TO BE HERE. `WantedIdx` searches THIS TABLE and nothing else - a name
    // absent from it resolves to -1 and `PropOffsetUnder` bails before walking a single class. Run 236
    // added the lookups and not the registrations, so both reported "+0xFFFFFFFF (depth 13) NOT FOUND"
    // after a full-chain walk that never actually compared anything, and the failure message blamed the
    // class chain. The run-235 dump had already listed both at `RvPlayerPawn` depth 1.
    //
    // ⚠ The dump and the lookup do NOT share this step: the dump reads names straight off each
    // property with `NameOf`, so it can see names the lookup is structurally blind to. That is why they
    // disagreed, and it is worth knowing before trusting a "not found" from this resolver again.
    { "mPlayerHandsStatus", -1 }, { "mTMD", -1 },
    // Read-only scene witness: actual native hand components and their assets.
    { "mArmMeshComponent", -1 }, { "mInviewMesh", -1 }, { "Mesh", -1 },
    { "SkeletalMesh", -1 }, { "Animations", -1 },
    // Native dual-TMD gun_scale resolves these by PropOffsetInChain, whose
    // WantedIdx gate cannot inspect any name omitted from this registry.
    { "mGunScaleCtrl", -1 }, { "ControlName", -1 }, { "BoneScale", -1 },
    // Ammo probe round 4: UT-pattern firemode object (WeaponFireTypes[0])
    // carries the live counters. Resolved per weapon class, never assumed.
    { "WeaponFireTypes", -1 },
    // Ammo probe round 7: pawn InvManager (live manager object).
    { "InvManager", -1 },
    // 2026-10-19: the owned-inventory chain (manager -> item -> item). The
    // audited native offsets are the source of truth; these two names are the
    // CROSS-CHECK that the layout has not drifted.
    { "InventoryChain", -1 }, { "Inventory", -1 },
    { "Instigator", -1 }, { "mWeaponName", -1 }, // Seeker ownership cross-checks
    // Ammo probe round 5: shots-fired counter (fire proof per pull) and the
    // pawn baseline below needs no new names.
    { "mShotCount", -1 },
    // Ammo probe round 9: HUD display cache (mag/reserve as shown on screen).
    { "mClipAmmo", -1 }, { "mBinAmmo", -1 }, { "mClipSize", -1 },
};
const int kWantNameCount = (int)(sizeof(g_wantNames) / sizeof(g_wantNames[0]));

inline int32_t WantedIdx(const char* s) {
    for (int w = 0; w < kWantNameCount; ++w) if (!_stricmp(s, g_wantNames[w].s)) return g_wantNames[w].idx;
    return -1;
}

