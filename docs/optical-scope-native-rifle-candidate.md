# Optical Kasimov scope + native Valkyrie emitter candidate

MAIN branch: `fix/rifle-vfx-sniper-grip-scope`. Supersedes the native-fullscreen
scope proposal in rifle-vfx-sniper-optics.md, not the accepted runtime profile.
All new behavior is default OFF. No new game assets, package edits, custom
flash replacement, inventory changes, or source from another worktree.

## Observations, hypotheses, falsifiers

Physical test5aa5f33: rifle VFX absent; native scope visible but unacceptable.
Lens proximity was stable (no repeated ADS feedback oscillation). Four rifle
captures completed without shader/group/witness overflow; accepted pistol
immediate-UP particle families were absent. Count differences do NOT identify
a visible particle. Do not broaden the accepted pistol shader list.

Read-only original Valkyrie package inspection identifies a different producer:
RvWeaponFX's **looping first-person GPU-cloud emitter**. Native PlayMuzzleFX
receives `(const FVector&, UBOOL)` (old bool/vector comment was reversed), reads
the weapon owner and creates the looping emitter at the supplied location.
The native weapon has not been moved to the VR controller; visual gun/root and
the proven shot origin already use a separately measured muzzle witness.

Hypothesis: the rifle's original looping emitter starts at the unmoved native
weapon muzzle, not the relocated VR muzzle. Smallest experiment forwards the
original call once, replacing ONLY that location for local first-person
Valkyrie with a fresh pawn/weapon-matching original-mesh muzzle witness.
No AI, pistol, impact, tracer, template, shader or inventory edit. Refuse stale,
invalid or different-origin coordinates. Mode1 observes; mode2 changes origin.
This does not assume that origin alone fixes GPU-cloud stereo or orientation.

Falsifiers: hook not reached; owner/first-person/witness guards refuse; old and
new locations already coincide; corrected origin still leaves VFX absent or
wrong per eye. Bounded logs report source/destination and guard results, **not
pixel proof**. Next action must follow those observations, not another guessed
shader register. The former pistol seam diagnosis does not prove the cause of
this different rifle producer.

## Scope implementation

Own native SceneCapture2DComponent and TextureRenderTarget2D,512x512,
PF_A8R8G8B8,20-degree optical view. Resolve constructors/view/projection/
attach/detach through reflected native wrappers, exact whole-wrapper and
helper fingerprints, and reflected property offsets. A mismatch refuses before
allocation. No hardcoded callable executable VA. Offline read-only audit:
`tools/fx/audit_rifle_scope_backend.py`; private receipt and original inputs
stay under ignored build/research storage, not Git.

Create/update/attach only from the existing local PlayerTick engine callback;
no render-thread UObject construction or nested PlayerTick work. Capture is
strongly referenced through the pawn's native Components array while attached;
TextureTarget retains the RT. Detach outside the near-eye interaction. Validate
classes, outers and target identity before reuse; GC invalidation drops pointers,
never calls a stale component, and permits at most3 allocations per pawn.

View origin/direction/up come from the original97-point lens's measured
controller-local mount, including authored skin/local/controller transforms.
Inverse XR/world basis has a360-yaw round-trip oracle. Freeze mount through
aim dwell to avoid ADS feedback; retain context/identity/proximity cancellation.
Disable native LT fullscreen ADS only for eligible Kasimov PiP mode. Other
weapons keep accepted input behavior.

Reuse the existing audited UE3 target/resource/RHI/COM identity checks. Only
successful draws while the exact own RT is bound, followed by unbind, advance
the completion lease. Never equate a counter with useful RGB. Three bounded
512-square readbacks sample RGB range/nonblack/alpha into logs (no files);
this may cause a short initial synchronization hitch. If rendering uses an
unobserved copy/resolve route, there will be no presentation rather than a crop
of an unrelated surface. A missing lease is evidence to inspect that producer.

Project a circular textured aperture through the accepted per-eye matrices at
the actual lens plane. Only the aligned eye receives magnification; the other
gets opaque lens glass. A scope-local crosshair uses the same projection.
Outside the aperture the regular stereo scene remains untouched. D3D stateblock
restores state; temporary COM references release same frame; Reset releases the
stateblock and clears completion. The prototype has no world-depth occlusion
for geometry crossing the aperture; verify this perceptually.512 and20degrees
are explicit initial quality/magnification choices, not calibrated optics.

## Configuration / protected baseline

Use offline `prepare_combined_followups.ps1 -RifleSniperFollowups
-OpticalScopeAndRifleOrigin`. Carries authored Kasimov grip / near-eye gate;
adds Render/SniperPictureInPicture=1 and Diagnostic/NativeRifleMuzzleOrigin=2.
Explicitly turns off the old four-frame NativeMuzzleProbe for this test to avoid
large diagnostic dumps. Existing accepted particle stereo, shadows, Polish,
reloads, gloves/knife, screenshot/mirror and145 protected files are preserved.
No installation or accepted-profile mutation in preparation. Require clean
x86 PE32 /O2 DLL+manifest, fresh current-profile change plan and hash preflight.

No Steam launch until a fresh user **start**. Deploy only with the repository
deployer while game absent; verify installed DLL/INI; arm post-exit restore.
No active-game timeout, no forced game exit or background file replacement.

## Single headset route, no chat switching

1. Load the known save, equip Valkyrie, fire3 short bursts into open space;
   rotate the gun and check whether flash/smoke originate at the barrel in
   BOTH eyes (distinguish wall impacts). Pistol remains the regression control.
2. Equip Kasimov. Raise scope to either eye, hold briefly, lower, repeat with
   the other eye. Magnification must remain inside glass and follow the gun,
   with normal outside stereo, correct image direction and no fullscreen zoom.
3. Check firing/reload, pause/resume and FPS, then close normally. Only then
   report. MAIN reads emitter/engine/RT/RGB/aperture receipts and verifies actual
   restored hashes before deciding the next step.

L1 covers deterministic coordinate/lease/failure guards and existing regressions;
L2 covers actual x86 /O2 build and provenance. Neither proves a live native call,
useful texture, correct per-eye alignment, or working rifle VFX. No L3/L4 PASS
for this candidate until the corresponding evidence exists.
