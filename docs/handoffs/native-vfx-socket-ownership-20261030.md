# Native weapon VFX: world muzzle versus native socket ownership

Branch: `fix/native-vfx-final-palette-and-pool`, based on `e9e4eaba`.
Physical baseline B9ADCFED failed Valkyrie, Kasimov and Centurion alignment;
Volk remains the unchanged working control. This correction is NOT yet a
physical PASS. Unfinished wrist-HUD edits are excluded from its candidate.

## Evidence and first violated contract

The archived game text is
`build/native-muzzle/live-20261030/Singularity.text-live.bin` (private).
`verify_native_muzzle_attachment_contract.py` verifies complete native bodies
and the actual argument-to-descriptor chain; it never reads a live process.

1. RvWeaponFX's audited +130 helper forwards arguments 8..11 into the pooled
   effect descriptor's base actor, skeletal component and two-word attach name.
   The prior correction changed world location/direction but retained these.
2. ObjectPool GetEmitterEx first writes world Location/Rotation, then calls
   RvEmitter's attach adapter with those fields. Null base skips attachment;
   it does not skip particle creation, template, owner, DPG or native lifetime.
3. RvEmitter dispatches native SetBase. When its name resolves to a socket,
   Actor SetBase **overwrites** relative position and rotation from that socket
   (8966A4..89678D). The alternative bare-bone branch calculates a preserving
   world-to-bone transform instead. These branches must not be conflated.
4. Original machinegun/sniper property exports include `muzzle_flash_fixed`
   and `muzzle_flash_fixed_iron` on `Spine_4`, as well as the actual muzzle bone.
   The B9 log confirms the retained Valkyrie emitter had a nonzero native base.
   This proves competing attachment ownership exists, not its final pixels or
   which socket was selected in every failed shot. The candidate's bounded
   receipt records the exact base/component/FName sent by each selected spawn.
5. Independently, Valkyrie's muzzle was staged before the later dual-TMD bone
   correction. Kasimov already had a final-body re-read. The final Valkyrie
   witness now uses that same final-palette route, excluding attachment draws.

## Minimal changes

- Only a fresh, exact local first-person measured muzzle call removes its four
  native attachment arguments. All template/owner/DPG/flag arguments survive.
  AI, shell ejection, impacts, third person and Volk are not selected.
- The existing owned Valkyrie loop is detached through the full-code-verified
  native SetBase adapter (84 bytes, ret32), then the existing native movement
  methods follow the measured rendered muzzle. No new pool hook or direct
  UObject transform write. Original particles, materials and lifetime remain.
- Loop rotation uses the same witness as its origin; no separate aim offset.
- Final body sampling adds Valkyrie to the existing Kasimov route. Successful
  stereo submission/restoration still controls witness publication.

## Validation and falsifier

- Optimized x86 geometry tests exercise translation and four wrist headings,
  body/attachment signatures, with Volk and Centurion excluded from this new
  slot-zero route.
- Actual x86 ABI fixture repeats 1000 spawn and detach calls, checks ESP,
  ECX, argument values, unchanged template/owner/flags and ret cleanup.
- Native disassembly verifier proves descriptor layout, attachment overwrite
  branch and full SetBase adapter. Compiled-DLL verifier checks the new actual
  MSVC forwarding sequence and restored nonvolatile registers.
- Native muzzle/stereo/state, measured aim, TMD and shadow regression suite.
- Optimized PE32 build and literal WeaponLocker `[eax+20Ch]` regression.

Runtime/HMD acceptance is PENDING. Expected bounded receipts are
`world muzzle attachment ... -> none` for selected one-shot effects and
`native base detached; measured world muzzle owns pose` for Valkyrie loops.
Neither receipt alone proves visible alignment. If the real effect remains
offset after these receipts, this correction is insufficient: correlate its
final PSC/particle draw with the final body witness rather than adding offsets.

One consolidated test: Valkyrie, Kasimov, Centurion single shots with wrist
translation/rotation, then a Valkyrie burst and L2 -> fire; verify flash AND
smoke at the visible muzzle in both eyes. Volk is a short unchanged control.
Check no displaced shadows, double effects, hand/grip or reload regression.
Keep native damage, range, spread and ammo untouched. Back up/hash before MAIN
deployment; restore the verified baseline on a new regression.

## Controlled candidate receipt

- Source: `72a1871342bc73b29059b15d8b5f56b94f56d7c4`, pushed.
- Clean tracked-source x86 `/O2` build (only ignored/untracked private files).
- DLL: `B7481E527AB0C5B751A4CC8ABC3DAD736A8ADE8FFAB65B4A053AC49762DD3CEF`.
- Native contract verifier, 17 optimized C++ fixtures, Python regression suite,
  compiled forwarding verifier and WeaponLocker compiled ABI: PASS offline.
- MAIN deployed DLL only after the profile preflight, game closed. All 46
  protected non-DLL files retained; INI unchanged:
  `A6A80E686A4F799F99301B9D5F179C6386AF1832993438B3F6769E48CAC3CF22`.
- Backup: `deployment/install-backups/20261030-135402`; previous DLL hash
  `B9ADCFED061B707B26BA9C5B0C817A30060934C67A0750DCAC5DB2CD80AD09FC`.
  An exact INI copy is also stored as `SingularityVR.ini.preserved`.
- Deployment report: `deployment/install-reports/20261030-135403.json`.
- Installed DLL/INI and backup identities independently rehashed: PASS.
- Steam runtime, final particle pixels and PSVR2: **PENDING**. Candidate retained
  awaiting the requested consolidated session; game not launched by this step.
