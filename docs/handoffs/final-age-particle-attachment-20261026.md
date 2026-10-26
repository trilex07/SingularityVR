# Age particle attachment — controlled candidate

## Evidence and limits

AB348E06 physical result: Age FAIL; L2, hands/support and black-artifact removal
PASS. The archived session's accepted Age action starts around line7258.
Owned beam post-update receipts subsequently differ from the rendered wrist's
TMD_Muzzle socket by roughly0.1UU in steady samples. This disproves treating a
corrected curved-beam source alone as proof that all visible Age FX are fixed.

Original RvTMDPower_Age effect objects reference ParticleSystem assets
`fx_FP_TMD_ageOld` and `fx_FP_TMD_ageNew`, separately from mAgingBeam.
Original RvEffectObject attaches effects to the owner's bone/socket. Existing
correction admits only the exact curved beam stored at RvTMD+530h, so these
particle components could never receive its CPU-parent correction.

**Proven code defect:** the attached Age particle path is excluded from the
rendered-wrist correction. **Still a runtime hypothesis:** these particles are
the specific displaced glow seen by the user. This candidate has a distinct,
bounded receipt to falsify that hypothesis, rather than moving the beam again.

## Original x86 contracts

`tools/tmd/verify_age_particle_attachment.py` reads the original PE and the
hash-pinned archived decoded .text, not generated C++ source assertions.

- PSC constructor78A880 publishes vtable17EF2B8.
- PSC +154 points to897240, same124-byte/RET16 update as the beam;
  +120 points to7C0E30, same68-byte parent-copy routine.
- SetTemplate754480 stores its actual pointer at component+1D4.
- SkeletalMeshComponent child updaterB036F0 enumerates pointer+27C/count+280,
  stride30h, child at row+0, and calls that child's +154 at return+161.
- The entire396-byte updater and update/copy bodies retain runtime fingerprint
  gates. No new engine patch or global aiming change is introduced.

## Minimal runtime change

Default-OFF `[Render] TmdTrackedAgeParticles=1` independently admits only exact
Age templates, PSC class, compatible update/copy methods, and exactly one
membership in the local TMD mesh's bounded attachment list. A template name by
itself never proves ownership. Foreign/duplicate/unreadable components pass
through unchanged. Ownership must be rechecked on every eligible call.

The existing final both-eye wrist witness supplies the delta:
`nativeChildParent * inverse(nativeWrist) * displayedWrist` (row-vector convention).
This preserves child/socket/animated transforms, uses one aligned local matrix,
and forwards the native callback exactly once. Native particles, endpoints,
power/energy rules, stereo and D3D state are untouched. No guessed offsets.

Installation reuses the existing fingerprinted beam update hook; it waits for
an owned Age beam to exist. If no beam appears, no particle hook is installed.
The controlled candidate retains the working beam/muzzle settings. Missing
installation or particle receipts is an evidence gap, never a visual PASS.

Receipt (maximum8): `TMD Age particle: exact owned attached ageOld/ageNew ...`
No per-frame GPU synchronization or unbounded logging.

## Verification and grouped physical gate

The real production callback runs100 particle updates across four wrist
rotations in x86 tests: output matches the existing rigid delta, original
parent remains intact, stack is balanced and native callback runs once.
Negative tests cover wrong class/template/method, foreign or duplicate
attachment, oversized attachment array and OFF. Existing beam/socket tests
continue covering pause/cinema/support/stale/wrong-pawn/thread and AV fallback.
Original machine-code contract check PASS. Build x86 PE32 /O2 PASS.

MAIN physical session: Square with visible TMD at two wrist orientations;
repeat L2 then support Grip while turning head; carry/insert magazine. Require
Age at glove in both eyes and preservation of accepted hands/weapon/artifact
behavior. Seeker remains a separate pending check if legitimately available.
If the exact particle receipt executes but the glow remains displaced, do not
increase offsets: next discriminator is the particle's own world-space versus
local-space simulation/render producer. No physical PASS claimed here.
