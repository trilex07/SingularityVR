#pragma once

namespace LockerLoadoutScreen {

enum class PointerHit { None, EquipAll, Tile };

// Canvas-normalized pointer (same space as ReadVrUiPair / native GFx UV).
PointerHit HitCanvas(float canvasU, float canvasV, int* tileIndex);

} // namespace LockerLoadoutScreen
