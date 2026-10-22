"""Guard-placement regression for every mod-owned acquisition entry point."""
from pathlib import Path

root = Path(__file__).parents[1]
loadout = (root / "locker_vr_loadout.h").read_text(encoding="utf-8")
acquire = (root / "locker_vr_loadout_acquire.h").read_text(encoding="utf-8")
cpp = (root / "d3d9.cpp").read_text(encoding="utf-8")


def body(text: str, start: str, end: str) -> str:
    return text.split(start, 1)[1].split(end, 1)[0]


assert 'GetPrivateProfileIntA("Input", "LockerVrLoadoutPreviewOnly", 0, path)' in cpp
assert "LockerVrLoadoutPreviewOnly=0" in (root / "SingularityVR.ini").read_text(encoding="utf-8")
assert "ClearQueuedMasks(&g_queuedMask, &g_uiQueuedMask," in loadout
assert "if (PreviewOnly()) ClearPreviewQueue();" in body(loadout, "inline void PublishFromEngine()", "inline void OnPopMenuScheduleAcquire()")
assert "if (PreviewOnly())" in body(loadout, "inline void OnPopMenuScheduleAcquire()", "inline void TryRunDeferredAcquire()")
deferred = body(loadout, "inline void TryRunDeferredAcquire()", "inline void OnDefocus()")
assert deferred.count("if (PreviewOnly())") == 2
assert deferred.rfind("if (PreviewOnly())") < deferred.index("LockerVrLoadoutAcquire::ApplyPendingMask(")
click = body(loadout, "inline bool OnPointerSelect(", "inline void PollEngine()")
assert click.count("if (PreviewOnly()) { ClearPreviewQueue(); return true; }") == 2
pickup = body(acquire, "inline bool TryPickupOrdinary(", "inline void ApplyPendingMask(")
assert pickup.count("BlocksAcquisition(") == 3
assert pickup.rfind("BlocksAcquisition(") < pickup.index("RunAuditedPickupRestore(")
assert "BlocksAcquisition(" in body(acquire, "inline void ApplyPendingMask(", "} // namespace LockerVrLoadoutAcquire")
print("preview config, queue, PopMenu, deferred, factory and pickup guards: PASS")
