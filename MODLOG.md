# Reverse-engineering journal

2026-10-16 native continuation: offline registration identifier -> initializer ->
exec wrapper -> class constructor/vtable -> native AttemptReload method proved.
Wrapper is no-param thiscall, whole EAX bool, derived slot0x4C4 in archived image.
Original Active.BeginFire calls AttemptReload on the empty-fire branch and has
WaitingForTriggerRelease refusal fallback. New OFF/observe/guard experiment
requires live reflection + wrapper/method-shape validation, player identity and
short-lived context, and grants one close-token before forwarding native call.
No native ammo writes, ProcessInternal misuse or Steam access. Reproducer and
falsifiers: docs/centurion-native-reload.md. Runtime/animation acceptance OPEN.

2026-10-16: Centurion `ch_inview_alt_pistol_mesh.psk`: measured 5799 wedges,
79 bones; rigid `clip` subtree yields 1067 UV vertices / 1252 triangles and
zero crossing triangles. Read-only native reload observation added; no direct
Reload ABI inferred from exported names. Evidence, hashes, falsifiers and
remaining gates: [Centurion reload](docs/centurion-manual-reload.md).

Continuation: inspected all three orthographic views of the private clip cache:
six-chamber cylinder. Validated RvGame exports name the pistol-cylinder component
and shared weapon Reload state/functions. Added read-only per-weapon UFunction
binding identity, shared-interpreter discriminator and ammo-change records;
no direct native ABI/call or inventory write inferred. Synchronized current
MAIN db4a261 into this own branch, preserving both sides of documentation conflicts.

Continuation: original Alt_pistol Reload PSA is 53 frames /24 Hz; clip local
motion returns to bind-relative start. Track 54 differs from PSK bone 57;
name-map through original parent cog_alt_pistol, never PSA placeholder parents.
Original mesh resolves material AR_Pistol and d/n/s/rim textures; offline parent
shader imports remain missing, so no claim of complete native material parity.
Validated Reload notification array has AnimNotify_RefillClip at 1.47052908 s;
RvGame names its owner RvSharedPawn. Added player-pawn-only read-only observation,
not a direct call or inventory write. Bounded PSA/tag readers + synthetic tests
and exact asset roundtrip; private original assets/reports remain ignored.
HUD freshness/identity/capacity guards and physical-left ownership strengthened;
360000 deterministic lifecycle frames pass, without Steam or headset claims.

PR37 base advanced during continuation. Merged its committed f0b96ad only into
own branch; resolved one input conflict preserving both left-grip arbitration
and MAIN's new knife impulse attack window. No accepted-source rollback:
canonical INI, knife, cinema and muzzle policy files match new base exactly.
Clean 9c164eb /O2 x86 PE32 rebuilt and candidate hash verified; receipt in doc.

Continuation: independent OFF CenturionClipPreview renders measured original
cylinder/albedo while Carrying only. No native cylinder removal, animation or
inventory writes. Original UV/albedo CPU preview inspected and exact cache
pair loaded; BGRA/alpha preserved. Existing knife renderer reused with explicit
albedo API, legacy default unchanged; SYSTEMMEM->DEFAULT upload/private depth,
Reset and nested state restoration. Authored controller mount is NOT UE3 socket
proof. 100 ms pawn/weapon snapshot gate and 50653 quaternion test cases added.

# Native cuff correction — SIDE, 2026-10-16

Branch fix/native-glove-cuffs from origin/main 644be25. No Steam operations.
Route: existing native hand draw; original wedge/skin/material retained, only
full finger-connected cuff indices and cap triangles substituted. Evidence,
rejected unconstrained triangulation, source hashes and test route are in
docs/native-glove-cuffs.md. Private geometry/report/preview:
build/gloves/cuff-repair; extracted assets/cache never committed.
