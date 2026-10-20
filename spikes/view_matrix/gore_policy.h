#pragma once
#include <cmath>
#include <cstring>
namespace gorepolicy {
inline bool EnemyBase(const char* n) {
    return n && (!std::strcmp(n,"RvAI_PawnSoldierBase") ||
        !std::strcmp(n,"RvAI_PawnRevertBase") || !std::strcmp(n,"RvAI_PawnZombieBase"));
}
inline bool Excluded(const char* n) {
    return n && (!std::strcmp(n,"RvAI_PawnCombatNPC") ||
        !std::strcmp(n,"RvPlayerPawn") || !std::strcmp(n,"RvPlayerPawnSP"));
}
inline float Chance(int percent) { return float(percent<0?0:percent>100?100:percent)/100.0f; }
inline bool Settings(float chance,float threshold) {
    return std::isfinite(chance)&&std::isfinite(threshold)&&
        chance>=-1.0f&&chance<=1.0f&&threshold>=-1.0f&&threshold<=1.0f;
}
}
