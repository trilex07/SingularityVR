#pragma once
// Station-scoped ordinary weapon grant (native pickup ABI). Wheel-only test path;
// does not modify WeaponLocker GFx commit (no LockerVirtualApply).
extern volatile LONG g_lockerStationGrantOrdinary;
void LockerStationGrantOrdinaryTry(uintptr_t movie = 0, uintptr_t cls = 0);
unsigned LockerStationOrdinaryWheelMask();
