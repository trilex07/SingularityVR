# Three weapons: native refill, not full native reload

## Centurion-proven follow-up / 2026-10-17

User now confirms Centurion works including first-shot animation. Its fixes
were PR48 source56b3f16 (stop required binding discovery before unrelated tail)
and4962b7a (release native pose after close). Frozen Centurion14:16 preflight
matchedF/stagefield-name and14:30 ready1/two conserved transfers are recorded in
that branch's docs/centurion-refill-resolution.md and post-refill-recoil.md.
This is NOT a physical result for Valkyrie/Volk/Kasimov.

Observed here: SAME generic resolver still walks beyond all4 bindings; native
root and scope are held for180ms after insertion, and the seat pass overwrites
ALL named reload tracks rather than just the magazine. The existing original
native evidence identifies all12 getters/refill targets as identical inherited
methods. There is no reason to preserve a full-reload pose when only native
RefillClip runs.

Hypothesis / controlled port: stop discovery on4 distinct ABI-checked bindings,
keep every body/cast/context/conservation guard. Port bounded stage diagnostics
and3 read-only preflights PER profile. At insertion release gun/root and scope
immediately; keep ONLY the magazine's original180ms seating track on the LIVE
root, then stop replacing native child poses. No new hook, guessed engine calls,
timers, input mapping, engine state or inventory writes. Existing flag OFF path
unchanged; Circle/physical-left shoulder/insert-release, NO flick, unchanged.

Falsifiers: a profile rejects exact live methods, transfer is not conserved,
old native Reload/fire lock/camera jerk remains, first-shot/scope recoil still
missing, seating/stock magazine or material/depth regresses. Offline tests must
exercise EACH original profile, preserve non-magazine matrices byte-for-byte
during seating, flag0 legacy hold, and all prior ABI/asset/Reset gates. MAIN
still owns new L3/L4 separately for each weapon; SIDE does ZERO Steam.

The HOLD and older receipts below are historical; the measured Centurion cause
is now known, but these three profiles are not physically accepted by analogy.

### Port implementation / L1 and clean L2 receipt

Source `2128a4df8b6731d8d25b8e73858b4ed1690f510a` on PR45 branch. Generic
resolver retains the exact wrapper/body/cast checks and publishes no result
before the entire audit succeeds. All3 classes inherit the same audited methods,
but each live profile still must independently pass. Auto-block preflight has
3 attempts/profile at least1s apart, stops on ready, no permit/getter/refill call.
Manual permits report their exact stage within the existing32-record/profile
budget. No additional hook or full-reload fallback.

Root/scope release now starts AT insertion, including WaitingNative, rather
than waiting180ms or a HUD ACK. Body seating modifies ONLY ClipSlot(kind), every
other bone remains byte-identical to the current native palette. After180ms
that override also ends even if HUD ACK is late. Native dropped-magazine replay
and the existing original material/shared-depth/Reset paths are unchanged.

L1_PASS19 CPU/ABI +2 own-process D3D9Ex GPU programs,67 Python checks. All FOUR
actual original wrapper decoders now tested by C++ against the archived text;
all original body/every-byte mutation,60000 fake native transaction, duplicate/
NPC/Centurion/holster isolation and live-count conservation tests still pass.
Each of the3 original SVWA caches is tested over5001 samples, including immediate
root/scope release, old flag0 behavior, late ACK and180ms boundary. First3
synthetic native shot poses preserve translation/rotation of EVERY non-magazine
bone byte-for-byte during seating; invalid palette cannot publish output.
Structural guards pin lookup completion before Next, later full validation,
bounded no-transfer preflight and actual body/attachment dispatcher wiring.
The same3 original assets and synthetic native replay pass stereo/Reset GPU
checks; this is NOT real game lighting, firing, stereo or physical acceptance.
Log: `build/next-weapon-reload/refill-only/complete-lessons-tests.log`.

L2_PASS clean source2128a4d, x86 PE32 014C/010B, `/O2`, performance
instrumentation OFF. Existing unrelated warnings remain. DLL3203584 bytes:
`90B6AB1D84E2DBAC3A29B072F7CAFF38E30BD867A3930010A2C57CA2E0A0E402`.
Build receipt SHA256
`937A005DCE8012E7DC555E6B134760D5ACBF2A9CB0AFF5FD15AA66A79575A296`.
Log: `build/next-weapon-reload/refill-only/complete-lessons-build.log`.

Private `build/next-weapon-reload/candidate-2128a4d/` prepared with the existing
checked-in generator:11 payload hashes independently verified,9 original caches
unchanged, DLL/receipt match source2128a4d, settings proposal ALL opt-ins OFF,
NO installed INI/assets touched. Manifest SHA256
`DBACD5D6AD619CEF0CDCA6D394D9B1046BBC693648B5453F76DFB5E6BF65F886`.
Packaging log: `build/next-weapon-reload/refill-only/complete-lessons-package.log`.
ZERO Steam by SIDE. Integrate source delta onto CURRENT MAIN; do not overwrite
the physically accepted Centurion refill-only branch with this older shared
hook's forwarding section. Keep BOTH separate scoped permit paths and ONE hook.

Next MAIN gate: one weapon/session, same manual/native-magazine dependencies
plus ReloadRefillOnly=1. Circle -> fresh ammo behind physical left shoulder ->
insert/release (NO closing flick) -> fire first shots promptly. Check magazine
seat, receiver/scope recoil, conserved native transfer/HUD ACK and no native
ReloadFinished/camera shake/fire lock. Repeat empty/partial reserve as available,
store without completing reload gives no ammo, pause/Options and normal exit.
User stays in PSVR2 until exit; no mid-test chat. Read `next refill resolver`
and `next refill-only` per profile. L3/L4 UNKNOWN for these3; no inferred PASS
from Centurion acceptance. Fine carried-magazine hand fit/world occlusion remains
the prior explicitly unverified visual scope, not silently declared accepted.

HOLD after Centurion physical feedback13:52: its SAME resolver returned ready0
before every transfer, despite offline PASS. These THREE weapons have no new
physical result; do not infer they are broken in the same exact stage, but keep
ReloadRefillOnly OFF/not promoted until PR48's bounded resolver preflight
(source9c38667, docs/centurion-refill-resolution.md there) identifies the cause.
Apply any resulting shared resolver correction here before headset acceptance.

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

## Clean L2 receipt / handoff

Implementation/source `41ec7929cb6b5c78a8c718976451899b9f768825`, clean tree.
L2_PASS x86 PE32 (014C/010B), `/O2`, performance instrumentation OFF. DLL3201024
bytes, SHA256
`43201D880817288416F1BD75D92DCE146FC36F07374B95FABF35AD91CEB91F5B`.
Build receipt SHA256
`91025A36752FD4A73D41C2D2A239B3C1A0D6C1A48BBC396085FBEE9C291E135F`.
Existing unrelated analyzer/compiler warnings remain; not warning-free.

Private candidate `build/next-weapon-reload/candidate-41ec792/`: DLL/build receipt,
the same9 original local-only asset caches and settings proposal with ALL new
opt-ins OFF, no INI. All11 payload size/SHA pairs independently verified; all21
retained fixture executables verified x86 PE32. Candidate manifest SHA256
`10384B155350DEFC22D2F7D7AD2D3E7C156C7F5DF3DBEF6F76F6170EAD2529EF`.
Candidate preparation/build logs are in `build/next-weapon-reload/refill-only/`.
Earlier candidate8896ff9 and other folders describe older flag0 behavior and
are NOT safe restore baselines for a newer MAIN integration.

Delivery stays on `feature/next-weapon-reload-assets`, existing draft PR45,
without merge. Integrate source41ec792 onto CURRENT MAIN that already has the
required PR45 runtime/material path; do not deploy this SIDE DLL over newer
accepted features. Documentary receipts after41ec792 do not change its binary
provenance. Installed/accepted/restore identities were not inspected/modified.
ZERO Steam launch, deploy, restore or installed assets/config changes.
L3/L4 UNKNOWN: physical elimination of the native fire-lock/camera tail is not
declared from these offline passes.
