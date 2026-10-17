# Three weapons: native refill, not full native reload

2026-10-17 SIDE. Applies the Centurion backend lesson from source6d7b201,
NOT an assertion that its new camera/fire-delay experiment passed PSVR2.

## Observe -> hypothesis -> falsifier BEFORE implementation

PR45 currently issues a single native reload after magazine insertion, then
holds the root through the entire original animation. Centurion feedback proved
that hiding this animation does not remove its firing lock or camera motion:
1563ms request-to-HUD refill,2078ms to ReloadFinished. The same risk exists here;
no user has yet physically reported the new variant on these three weapons.

Offline original PE/text inspection: RvWeaponShared, RvWeaponShared_Shotgun and
RvWeaponShared_SniperRifle ALL share the exact native RefillClip and three ammo
getter methods. No derived override in those four virtual entries. These are
no-parameter void/int32 thiscall functions derived from native registration and
wrappers, not guessed ProcessEvent calls. RefillClip transfers through native
inventory methods and contains no Reload state/animation/camera dispatch.

Hypothesis: consume one completed insertion permit on the natural engine thread,
call ONLY audited native refill after matching live counts to the frozen manual
snapshot, verify conserved live clip/bin transfer, then independently await HUD
ACK. Do not invoke original AttemptReload in that branch. Preserve original
remaining-round accounting, Circle/left-shoulder/insert and NO closing flick.

Also retain a fresh block-only context while holstered and revoke the pending
tap/token immediately on store: cancellation must not accidentally enable an
ordinary native reload. Use measured mid-eye/head-yaw left/back shoulder, latched
for the transaction; stop holding the native root for several seconds when no
native reload was started. Native magazine material/depth path stays unchanged.

Falsifiers: wrong class/body/ABI, live snapshot mismatch, nonconserved transfer,
duplicate refill, native ReloadFinished tail, continued fire lock/camera jerk,
store grants ammo, shoulder moves to the right or stale render witness inserts.
Unknowns reject the new permit, NEVER fall back to full reload after a transfer.
Do not force engine Active, edit inventory/HUD, shorten arbitrary engine timers
or globally suppress camera. This is a separate default-OFF ReloadRefillOnly
experiment, one weapon/session; prior behavior remains when OFF. L3/L4 UNKNOWN.

## Implementation and offline gates

`native_refill_only.h` reuses the exact four fingerprints and no-parameter
int/void wrapper decoders from Centurion source6d7b201; counts accept finite
capacity1..1000/reserve0..1000000 (same bounds as the existing HUD observer),
not an assumed six-round magazine. `native_refill_engine.inl` reuses its bounded
reflection/vtable resolver, with exact class/name, full normalized-body and
common-inventory-cast checks. No extra hook: the existing shared AttemptReload
owner routes only the next-weapon PERMIT branch here. Centurion's branch is
retained. The shared Gate additions are backwards-compatible defaults and match
the Centurion change, rather than a competing permit implementation.

The snapshot is frozen at Arm, not overwritten by later HUD polls. Native live
counts must match it before the call; afterwards only exact min(capacity-clip,
reserve) conservation returns success. Unknown/changed state rejects the permit;
postcondition failure latches a per-profile fault. Neither case forwards into a
full reload or retries the transfer. Existing native ammo ACK still additionally
requires that transaction's consumed permit and conserved read-only HUD change.
The latter may wait for the next10Hz HUD sample; no zero-frame latency claim.

New-mode store cancels the tap and revokes the permit immediately, then keeps
fresh owned holstered contexts block-only without requiring a hidden gun draw.
Unknown/stale-context fail-open forwarding remains the prior documented policy;
this is NOT a guarantee of suppression under arbitrary tracking loss. An already
running native reload is not forcibly cancelled. Unexpected native tail/camera
motion remains a falsifier to investigate, not permission to force Active.

The shoulder uses validated actual mid-eye positions and physical head yaw
latched at Circle. Existing body/stick yaw is not substituted for physical left.
No user INI calibration or hip anchors are changed. After insertion only the
already-seated original child return segment is compressed to180ms in renderer
math; the parent/scope hold ends with it once ACK arrives, not seconds later.
No engine timer/animation-tree write; prior long native path remains with flag0.
Original native material/shared depth and old private caches remain unchanged.
This does NOT prove fitted carried-magazine grip or world/hand occlusion in VR.

L1_PASS:19 CPU/ABI +2 OWN-process D3D9Ex GPU programs (21 total),64 Python
tests. Includes60000 OFF/ON native-detour transactions across THREE fake weapon
objects, partial clips and limited reserves, void/int thiscall ABI, no original
Attempt call on refill-only permit, holster rejection, duplicate/NPC/Centurion
isolation. Four archived ORIGINAL bodies checked with every-byte mutation;
separate mapper proves all12 inherited bindings plus no relevant script override.
15003 original child-pose samples and333 physical-left mid-eye shoulder poses
include floor-seated heights, varied yaw and IPD. Original palette/socket/root,
magazine material/depth/two-eye/Reset and all prior reload regressions pass.
These are NOT original engine execution, actual camera behavior or headset PASS.

Private evidence `build/next-weapon-reload/refill-only/evidence.json`, SHA256:
`56D2DF5AB9CD5A8CAE9A67F5ADCCA61638A91F433F893EFEDE2B83C360BEBAC3`.
Original PE SHA256 BD459ACD87D4B82693928DEE4ED437C086D5928A8E0428F3AD54B6FC114C0F51;
archived text858F2FC33B1C99B3A4A27A817A11205DA66414EE1B030A03DF821F1AD6A0C78A.
Tools output metadata only; no original engine bodies or assets committed.

Reproduce: `tools/reload/inspect_next_refill.py` with the private original PE,
archived text, RvGame.decompressed.xxx and original text base; run inspected
`tools/reload/test-centurion.ps1 -NativeEvidence <original-evidence> -PreparedRoot
build/next-weapon-reload/prepared -Gpu -RefillEvidence
build/next-weapon-reload/refill-only/evidence.json`. Full final log:
`build/next-weapon-reload/refill-only/tests-final.log`.

## MAIN integration / physical gate

Integrate this source delta onto CURRENT MAIN which may already include newer
Centurion, language/hands/knife/UI/VFX fixes. Do NOT replace d3d9.cpp wholesale,
duplicate the AttemptReload hook, or deploy an older SIDE full DLL over MAIN.
Keep BOTH Centurion's own refill-only branch and this separate three-weapon
permit branch. The generic resolver does not add another MinHook owner.

Keep the accepted dependency profile and exact private asset hashes. One weapon
per session: existing manual flag/layout/native-magazine settings unchanged,
add ONLY `[Input] ReloadRefillOnly=1`. This does not enable manual reload by
itself or alter Centurion settings. The candidate generator proposes flag0,
never writes an installed INI. MAIN rebuilds/verifies hashes, backups, exit and
bounded restore lease; SIDE does no Steam launch/deploy/restore.

User stays in PSVR2: fire several rounds -> Circle -> fresh magazine behind
physical LEFT shoulder -> insert/release (NO flick) -> release/press trigger
promptly. Repeat from empty and with limited reserve if the save allows it.
Circle/store without completing reload -> wait3s -> draw: no free refill.
Pause/Options/resume, check stereo/hand/gun/scope and normal exit. Stop on
discomfort. MAIN checks `next refill-only:` result2/ready1 with live before/after,
independent consumed/HUD ACK and no following native ReloadFinished tail from
that operation; per-profile budget32. Results0=reject,1=counts changed,2=conserved,
3=fault (latched, no fallback). A counter is not pixel or PSVR2 proof.

L2 receipt follows; L3/L4 UNKNOWN. PR45 remains unmerged.
