# Transition and hitch status — 2026-10-08

The current Steam build was intentionally safe: `VfxHost=0`, `VrShadows=0`,
and no VFX/shadow plugin work is running. The source still contains an
opt-in muzzle candidate and ShadowMode 3 candidate; neither is evidence that
the headset currently shows a correct shadow.

Desktop evidence and code audit found two mod-side transition hazards:

* a pistol support grip could never acquire in the close side-by-side pose;
* a dead/replaced UE3 camera could leave the previous camera matrix, recenter,
  hand pose and grip latch alive through death/load.

The production changes now use a pistol-only near-grip acquisition gate and a
bounded camera liveness check. On camera loss or replacement the VR caches are
invalidated, hands and support latch are cleared, and the next valid camera must
reacquire centre/poses before rendering them.

The deterministic `test-vr-transition-sim.exe` covers pistol acquisition,
hold-until-release, camera-loss clearing and reload reacquisition. It does not
measure the game's own enemy-hit or melee cost. Existing evidence shows XR pose
polling is normally sub-millisecond; a physical run with `HotPathSampler=1`
and `FrameSpikeReport=1` is still needed to attribute any remaining hit hitch
to the game, GPU, or a mod hook before disabling a feature blindly.
