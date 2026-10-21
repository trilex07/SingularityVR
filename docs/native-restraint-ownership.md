# Native bound-hand ownership — 2026-10-21

Status: IMPLEMENTED_NOT_VERIFIED; offline CPU/source checks and x86 PE32 `/O2`
build pass. No Steam deployment, runtime identity match or physical acceptance.
Audio belongs to SIDE and was not changed.

## Observed failure and falsifiable mechanism

User confirms original animated hands during the cutscene, then freely tracked
hands during escape while the character should still be bound. The failed
SetPlayerCinematicMode bridge had zero invocations. Do not repeat that bridge as
a fix, or classify restraint from PlayerWalking/Startup/cinema OFF.

Hypothesis: independent foreground/hand policies reclaim the native rope mesh
after cinema OFF even though the game still owns its animation. Preserve native
ownership while the actual local pawn uses the exact authored restraint mesh.
Falsifier: the runtime pawn's current Mesh.SkeletalMesh is not that asset during
the bad escape, or ownership is active but the original hand/rope pixels remain
wrong. Neither case justifies broadening to all unarmed meshes or a timer.

## Static producer evidence (private owned-game inputs)

Decoded SP_VI6_kismet01.u SHA256
67EC9ED72B29E9C135D5618131ED93089AAC71490CD741E689427A280FD80472:
RvSeqAct_SetPlayerSkeletalMesh exports6177/6178 in Cine_GhostDeath target
`wp_rope.Skeletal.ch_inview_noTMD_rope_mesh` through mNewSkeletalWeaponMesh.
Import-1052 is SkeletalMesh, outer-807 Skeletal, outer-806 wp_rope.
The package also contains player_fs_tied, not just a suggestive scene name.

Decoded SP_VI6_kismet05.u SHA256
B8FF568CE22CEBE364FF0202C7E79227538FD4852C1E812E6C3E98629C5A3F89:
export1542 in Katya_cuts_bonds targets the same asset. Merely seeing this action
does not prove its runtime timing or that hands remain bound after it completes.
Its native mesh replacement/restoration is authoritative; do not hold ownership
until a guessed time or level name. Intervening kismet02..04 had no matching
mesh/action names. This absence alone is not proof of runtime persistence.

Owned script RvGame.u SHA256
09DE450D9F620952C3701D874BEB90A8BD9235263975157A641B9E3441336C92:
RvPlayerPawn.OnSetPlayerSkeletalMesh export12230 consumes
mNewSkeletalWeaponMesh through inherited Pawn.Mesh.SetSkeletalMesh. It handles
mNewSkeletalArmsMesh separately via mArmMeshComponent. Thus observing only
mArmMeshComponent/mInviewMesh missed the weapon/rope producer. The bounded
scene witness now also observes Mesh. No copyrighted package/assets are added.

The Katya_cuts_bonds graph has five separate Interp actions, including laptop
tracks, so an arbitrary active Matinee is NOT a safe cinema classifier. Its
ToggleCinematicMode1570 receives Disable from CombatEvents161; the latter is
annotated by the level author as giving the player a weapon. This does not
identify the earlier flickering camera segment. Camera classification remains
OPEN; this patch deliberately does not set g_inCinematic or change HMD/camera.

## Small controlled correction

`[Render] NativeRestraintHands=1`, default OFF. At Present, before animation
policy refresh, guarded reads resolve named, typed ObjectProperty fields Mesh
and SkeletalMesh through the existing validated reflection layout. Offsets are
cached per class. No GObjects walk, GPU readback, engine function call, save or
UObject write. The mesh must be SkeletalMesh with exactly the name above,
Package Skeletal, Package wp_rope, and null root Outer. Failed/null/cyclic/wrong
identities refuse. Current live pawn is required; no sticky ON event.

While proven active, native-animation ownership excludes independent tracking,
foreground suppression, TMD palette/index surgery, independent glove rendering
and knife overlay/cues. Cinema hands and healing remain separate. Mesh changes,
pawn changes, invalid reads, VR OFF and flag OFF release the policy. Render
decisions use the per-frame cached result; transition logs are capped at32.
The cache can lag a native asset transition by one completed frame, not seconds.

## Gates and next observation

47 deterministic CPU identity/refusal/release cases,9 source guards PASS.
Optimized x86 PE32 build PASS; existing analysis warnings remain (not warning-free).
No installed DLL/INI/UI/audio/assets changed. No L3/L4 claimed.

Clean-source build receipt: source b6ba0dba4971471af7564ec0ddf1bc1fe99d0762,
DLL SHA256 2C937E58CD785AB857DA71B3464E9FB9292146569B0EB5762D5E2AAD6E841283,
no PERFORMANCE define. Private artifact build/combined-followups/native-restraint-b6ba0db
contains DLL and build provenance. This is not a deployed test package.
Independent post-build installed DLL EF3A1F3B / INI55A3ED4D match accepted hashes;
no Singularity process was observed. Only untracked private deployment/leases
remained in worktree at build. Existing failed bridge remains default OFF;
do not enable it as though it had new supporting evidence.

After a fresh user start and normal deployment/hash/backup preflight, collect
`native-restraint: active=1/0` plus Mesh/SkeletalMesh witness for the same escape.
Physical route (no mid-headset chat): load captive save; observe bound hands
through escape and the release; finish and close normally. Check native freedom
returns only after actual release. Do not advertise Kathryn cinema as repaired
by this hand-only correction; continue proving its actual camera producer.
