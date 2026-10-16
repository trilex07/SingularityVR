# Centurion: native reload seam, not another gesture prototype

2026-10-16 SIDE continuation, `fix/centurion-grip-shoulder`. ZERO Steam/process
access. This extends the shoulder/control work; it does not replace its assets.

## Observation and falsifiable hypothesis

User's old-prototype feedback: an empty Centurion auto-reloads, so gestures
alone do not control the native inventory. Names/probe callbacks were not proof
of a callable native ABI, and the existing ProcessInternal hook may miss native
functions. Instead of suppressing an arbitrary script event, follow the actual
native registration, class construction and virtual dispatch.

Hypothesis: the audited native `RvWeaponShared::AttemptReload()` is the seam
shared by the script's empty-fire automatic request and a deliberate reload.
Returning native bool false without calling the original rejects the request;
one completed manual close can permit exactly one ordinary engine request.
Falsifiers: automatic ammo gain occurs with no native-hook hit; a blocked request
still refills; the dedicated-identity caller reaches a different override; the
engine does not recover after trigger release; or a completed close does not
reach the allowed native request. Counters/result true alone are NOT ammo proof.

## New offline evidence

An owned offline PE was matched to its archived original `.text` capture.
`tools/reload/inspect_native_reload.py` independently reconstructs this chain:

1. Original wide registration identifier `intARvWeaponSharedexecAttemptReload`
   -> static lookup row -> initializer writing the native wrapper pointer.
2. `RvWeaponShared` class registration -> constructor thunk -> constructor's
   final primary vtable -> the slot DERIVED from that wrapper's instructions.
3. Wrapper consumes no function parameters, performs the VM end/debug handling,
   calls virtual method with ECX=this, stores whole EAX in RESULT, `ret8`.
4. Derived method has no stack arguments and returns native32-bit bool, with
   clip/capacity/reserve guards and the original state/reload dispatch. It does
   not inline the actual later ammunition refill.
5. Original package `RvWeaponShared.Active.BeginFire` export16192 calls HasAmmo,
   then its empty-fire branch calls HasAnyAmmo and AttemptReload. The refused
   attempt expression leads immediately to WaitingForTriggerRelease. Narrow
   bytecode inspection, not a general decompiler or a live execution trace.

Private receipt: `build/centurion-reload/native-seam/evidence.json`.
Inputs SHA256:

- Owned offline EXE: BD459ACD87D4B82693928DEE4ED437C086D5928A8E0428F3AD54B6FC114C0F51.
- Archived text, exact section length20447514:
  858F2FC33B1C99B3A4A27A817A11205DA66414EE1B030A03DF821F1AD6A0C78A.
- Original decompressed package:
  09DE450D9F620952C3701D874BEB90A8BD9235263975157A641B9E3441336C92.

Recorded historical addresses ONLY for reproducibility, NEVER runtime constants:
wrapper0x00F60E40 -> slot0x4C4 -> method0x00EA3710, method467bytes.
Constructor thunk0x00EC38E0 -> constructor0x00EC14A0 -> vtable0x0192F200.
613 remains a PACKAGE NAME index, not a native opcode. No engine body/asset is
redistributed in Git; the tool emits evidence metadata, not decompiled bodies.

## Implemented default-OFF experiment

`[Input] CenturionNativeReloadGuard`:

- 0: no new native hook, exact native behavior retained (default).
- 1: native seam observation, ALWAYS forwards original, bounded48records.
- 2: conditional suppression experiment; requires ManualReload+ControlLayout+
  CylinderGestures. Otherwise downgraded to1. No inventory writes or interpreter
  calls. This is IMPLEMENTED_NOT_RUNTIME_VERIFIED, not an accepted fix.

Resolver uses current player's reflected `AttemptReload` binding, exact owner
`RvWeaponShared`, executable wrapper shape, derived vtable slot and audited
method-shape anchors. Unknown versions/overrides/third-party patched entry refuse
installation. Shape anchors are not a complete cryptographic method signature.
No runtime absolute addresses, no shared vtable edits, no direct invocation.
The isolated MinHook detour forwards the natural method call using its proven
no-param thiscall ABI. Create/enable failure leaves the game unchanged; never
null/reinstall a previously working trampoline.

Mode2 applies only to the CURRENT pawn/held right-hand Centurion, exact installed
method, fresh identity-bound HUD sample, tracking and gameplay/focus context.
The input thread publishes the policy through a nonblocking lock. On contention,
missing data or context older than250ms, the hook forwards native behavior;
that safety fallback can allow auto reload and must not be hidden as success.
Other players/NPCs/weapons forward unchanged even when they share this method.

No permit -> return0 before original execution. Completed insert+close with
NativeReloadTap enabled arms a single-use500ms permit, then sends the existing
ordinary Use/reload tap. Consume the permit BEFORE forwarding, preventing nested
or repeated calls from using the same close. Context/pawn/weapon change cancels
it. A refused original attempt consumes the permit; retry needs a new sequence.
Actual native ammo/animation still belongs to UE3. Mode2 intercepts deliberate
keyboard/native reload too while eligible: use the physical sequence instead.

## L1/L2 and next MAIN gate

New CPU policy tests reject malformed wrapper/slot/body, validate the actual
archived ORIGINAL wrapper/body (read-only), and replay100000 identity/token
sequences. A separate `/O2` x86 test executable uses the SAME MinHook and calling
convention on its OWN fake method: ECX/EAX/stack, original side effects, NPC
forwarding,10000 permit/block pairs and clean detour removal. This is CPU/ABI
evidence only, NOT a Steam session. Unchanged upstream MinHook uses production
`/W3`; our test/policy sources retain `/W4 /WX`.

Run `tools/reload/test-centurion.ps1 -NativeEvidence <private-evidence.json>`;
it checks capture hash before comparing original bytes. All sources build with
`spikes/view_matrix/build.ps1` WITHOUT `-Install`; L2 receipt appended afterward.

MAIN must integrate/rebuild onto CURRENT accepted MAIN, preserve INI/Polish/
glove/knife/menu assets and verify exact hashes/backup. No deployment authorized
by this document. One native mode per session:

1. Mode1 observation, old manual backend disabled: perform ordinary Centurion
   reload/empty-trigger in existing save, prove current object hits the audited
   method and conserved HUD transfer. Reuse the agreed complete route with user
   staying in headset; do not ask them to switch out just to read a log.
2. Only after that evidence, mode2 with the existing validated gesture flags and
   NativeReloadTap=1. Fire to empty: no automatic refill; release trigger, perform
   Circle/open/down/shoulder/insert/close; verify one original permitted attempt,
   conserved ammo transfer and resumed firing; also test partial reload and
   pause/options/context cancellation. Native reload animation still plays.
3. Record actual pixels/behavior, normal exit and safe restore. If no native hit,
   installation mismatch or policy fallback, stop the gate experiment and use
   that evidence to find the remaining producer, not a speculative ammo write.

Actual animated cylinder opening/ejected cases/fitted hand/scene-lit preview
remain OPEN. This seam does not itself complete those visuals.

## Clean build receipt

L1_PASS:13 optimized CPU executables,31 Python checks, original archived
wrapper/body validation and own-process x86 MinHook ABI replay PASS.
L2_PASS: source `a2b77ac5f16fc8218799443ce26e17644ea49e71`, clean working tree,
`/O2`, x86 PE32 (014C/010B). DLL SHA256:
`89A5209153F194ED858553EAE2C3B99CC1BC2E52C9DD9B1DEBD9C43977458339`.
Private candidate/manifest and existing original cylinder/albedo hashes checked.
No INI staged or installed files changed. Existing unrelated compiler/analyzer
warnings remain, not a warning-free build. L3/L4 still UNKNOWN.
