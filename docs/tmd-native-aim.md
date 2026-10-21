# Local native TMD aim candidate (SIDE, 2026-10-21)

## Evidence and hypothesis

Original SIDE redirected shared gun/controller aim whenever TmdRaised. That is
rejected: holding the left-hand TMD is not proof of the owner of a weapon trace.
Instead the owned original binary exposes two **pure output getters**:

- RvTMD GetAimVectors: primary vtable +0x440, two FVector out references, ret8.
  Complete body119/FNV19B6BABB20248999. Gets its owner, requests the owner's view
  point, converts the returned rotator to a unit forward; fallback uses actor
  Location/Rotation. It does not write the camera or shared weapon aim.
- GetTMDOwner: primary +0x43c, body7/FNV940DBEC1332B5EEE, returns this+0x98.
- GetAgingVectors: secondary interface this=TMD+0x1e0, slot+12,
  body59/FNV73C3C6D548B0AF74. Copies its cached source, adjusts this by -0x1e0,
  calls the **same +0x440 getter** for direction. Its output source otherwise
  remains in the original first-person location.

Native wrappers223/E88344532422184A and235/EE4912DC7241017F independently confirm
the out-reference dispatch. Constructor63/F2C5B1B22F49E767 proves the primary
and secondary vtable relationship. No preferred game address is used by the
new runtime resolver. This follows the coordinate-space separation described
in vrframework guides08/09 conceptually; no external framework code is copied.

Hypothesis: returning a ray tied to the actual off-hand render transform at these
TMD-owned producers corrects powers consuming these getters without touching
right-hand gun aim. Falsifiers: getters never run for a power; successful return
edits but endpoint/emitter does not follow; right-hand shots or camera follow
the left hand. There is NO claim that every native emitter consumes them.

## Implementation

`[Input] TmdNativeAim=0` by default; absent key also OFF. MAIN can set1 in a
separate candidate INI, preserving all accepted settings. Shipped INI unchanged.
Startup/lazy installation uses the existing original engine input poll, not a
new polling thread. Requires the existing native VR input hook to have installed
(VR UI native input hook create=0/enable=0); no hook means no TMD correction.

Read local SP pawn through camera ownership, reflected pawn.mTMD, exact ancestor
classes and original owner relationship. Verify all three complete method
fingerprints before installing two getter detours. Only the installation's
engine thread and same concrete TMD class may rewrite results. Other objects,
AI/MP, menu/pause/cinema, invalid tracking, gun-fire nesting, unavailable locks,
stale/future/wrong-pawn snapshots and overlapping references forward unchanged.

BuildTmdC publishes a lock-protected copy of its **actual final matrix**, original
anchor, base view-forward, camera position, pawn and timestamp. No second copy
of Euler/sign/trim logic. Row-vector mapping gives G*C=H; normalize away mesh
scale. Ray origin is the transformed TMD anchor, not a claimed measured palm
socket. The mapping honors TMD trims, GunViewFrame, position/head correction and
uniform mesh scale. Maximum pose age150ms; menus/cinema invalidate the snapshot.

Native getters execute exactly once first. Aging's internal call to the aim
getter is guarded with thread-local depth and SEH-finally unwinding: edit the
outer outputs once, never double-transform. Native effect components, camera,
global gun aim, weapon traces, muzzle mailbox, energy/perks and inventory are
not written. Maximum12 receipts per getter; no per-draw logging or allocation.
On hook enable failure gate goes OFF and callable trampolines remain retained.

## Reproduce offline (private original inputs; no Steam access)

```powershell
python tools/fx/audit_tmd_aim.py --exe <owned-original-exe> --text <archived-complete-text> --report build/effects-review/tmd-aim-audit.json
./tools/fx/test-native-muzzle.ps1
./spikes/view_matrix/build.ps1 -GameBinaries <offline-fixture-Binaries>
```

Auditor emits contracts/hashes, not game code. L1 checks sweep yaw/pitch/scale,
anchor mapping, freshness and refusal gates. Synthetic x86 adapter executes the
real header with fake objects: original forwarding once, adjusted aging this,
nested getter suppression, SEH unwind, foreign/OFF/gun/pause/cinema/tracking/thread
and stale refusal. Python wiring assertions are structural, not native proof.
L2 production build x86 PE32 /O2 plus static-analysis pass; unrelated pre-existing
warnings remain. Final clean commit/SHA is in local d3d9.build.json, not the first
dirty prototype build. No L3, native emitter pixel validation or L4 performed.

## MAIN route (three sentences; user stays in headset)

With an already legitimately unlocked TMD, point the two hands in opposite
directions and use aging/grab/impulse on two different targets, moving only the
left hand; watch the start and endpoint in both eyes.
Keep TMD raised and fire the right-hand gun at a third target, then lower TMD,
pause/resume and verify that the camera and gun direction have not changed.
Exit normally and report which powers followed the left hand, whether their
visible effects agreed with the hit, and any camera/gun/stereo regression.

MAIN must first collect bounded `TMD native aim: verified getters installed`
and aim/aging receipts from the verified candidate. Waiting/contract-refused or
zero getter receipts is an inactive path, NOT success. A power using another
producer remains OPEN and needs its actual native consumer traced; do not
silently broaden this into weapon-fire or global camera edits. No artificial
unlock, save edit or grant is part of this candidate. Test this independently
from effects_remaster, gore or any other rendering experiment.
