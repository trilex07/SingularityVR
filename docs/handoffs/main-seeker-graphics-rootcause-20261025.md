# MAIN: Seeker resolver and tracked foreground depth prime

Branch: `fix/main-seeker-graphics-rootcause`.
Parent: `218dea7344bcb89cbe630e06141121e0f9007a59` from
`integration/full-psvr2-20261025`. Accepted installation checkpoint:
`610F4C21C4E610C40CDF5381B8D8E98675482E409669329C3EDEF23CCD8AEFF1`.
This is a candidate, not a new physically accepted baseline.

## Latest physical failure and follow-up (supersedes the old test below)

The user tested `F20194FD...` from source `0d871fe1`, session
`build/main-seeker-evidence/session-20261025-121754/`. TMD Square FX did not
originate at the glove; black offset silhouettes remained; Seeker back retrieval
and practical accuracy failed. The installed DLL/INI were restored to accepted
`610F4C21...` / `C5D24E16...` and independently hashed after exit. No new Steam
deployment or game launch was performed during this follow-up.

### Actual Square/Age producer: owned curved-beam attachment

This supersedes the socket-only consumption uncertainty below. Original package
reflection identifies `RvTMD.mAgingBeam` as a component of
`RvCurvedBeamComponent : RvSharedBeamComponent : StaticMeshComponent`.
It is not an ordinary particle emitter. Native Age creates it, assigns its
original mesh/material, then attaches it to the owned skeletal mesh using
`mMuzzleSocket`. The subsequent skeletal child update composes its parent from
the original **CPU SpaceBases**. Independently tracked glove rendering changes
the **GPU palette**. This is a proven transform ownership mismatch in the code
path, not proof that the new hook has run in a physical session.

Read-only `tools/fx/audit_tmd_beam.py` reproduces the following from owned EXE,
unpacked text and original package; full disassembly stays in ignored evidence:

- Age constructor `1019410`, vtable `19B7C18`; virtual +144 `FCFEB0` (209 bytes,
  FNV `E896EBDDC7AA69D1`) creates/stores the beam and calls `B0BE40` at `FCFF77`.
  The caller reads owned mesh +204, socket FName +45C/+460 and aging beam +530.
- `B0BE40` (123 bytes, `92248BACBFC2BF33`) resolves the original socket and its
  relative bone transform, then attaches the child through `B0BCE0`.
- Skeletal virtual +138 `B036F0` (396 bytes, `750AA01ABF6A6D77`) walks attached
  children, reads CPU SpaceBases +218, and composes socket/bone/local matrices.
  At `B0384F`, child virtual +154 receives scene, owner, parent FMatrix*, 0.
  Return identity is exactly `children+161`.
- Curved-beam virtual +154 is `897240` (124 bytes, `81209A2B80795CAE`), RET16.
  Its virtual +120 `7C0E30` (68 bytes, `0DBA87D91A4465CC`) synchronously copies
  all 64 parent bytes using MOVAPS: replacement storage must be 16-byte aligned.
- Age TickFX `FE4570` (618 bytes, `A7CF9086B0FCC73F`) consumes that component's
  world start. Native curve endpoint and gameplay target remain authoritative.

`native_tmd_beam.inl`, enabled only by **`[Render] TmdTrackedBeam=1`** (default 0),
hooks this existing native child update. Exact local TMD/aging component identity,
original class/vtable fingerprints, caller return site, engine thread, active VR
TMD, no support/menu/cinema, and a <=150ms pawn-matched both-eye wrist are required.
It passes original socket * final rendered wrist as the aligned parent, then
invokes the original update **once**, forwarding scene/owner/collision unchanged.
Other components, powers, heal beam and callers retain their original parent.
An unreadable context disables this feature and forwards the native call.
There are no weapon-ray, energy, animation, mesh, material or endpoint writes.
No GPU synchronization/readback is introduced; logs stop after twelve successful
updates and twelve gate changes. Hook installation retries are also bounded.

Actual `/O2` callback accounting (auditor checks emitted instructions, not source):
entry S has return; S+4 scene; S+8 owner; S+12 parent; S+16 collision; ECX child.
EBP=S-4, A=align_down(EBP,16), C=A-60h after local storage and ESI/EDI saves.
Collision remains at C-4 across the four-argument cdecl Prepare call; its 16-byte
cleanup returns ESP to C-4. The native call sees ESP=C-16 containing scene,
owner, selected parent and collision, with ECX restored to child. Native RET16
returns to C; EDI/ESI and entry EBP restore, callback RET16 ends at S+20.

The CPU fixture executes the **production hook** 100 times across four wrist
rotations. It checks ESP, every argument, 16-byte alignment, unchanged input
parent, exact child/caller, stale/pawn/context/AV/OFF fallbacks and original once.
Dual-TMD (20 programs plus wiring/WeaponLocker), muzzle/TMD/aim (15 programs plus
Python), factory `[eax+20Ch]` and OLED CPU/D3D11 regressions pass. The production
PE32 `/O2` and both native/compiled ABI fingerprints pass. Existing D3D9 HAL
CreateDevice failure `8876086A` is still a separate unexecuted pixel/state gate.

**L3/physical acceptance PENDING.** Suggested next single-variable TMD check:
`TmdTrackedBeam=1`, `TmdTrackedMuzzle=0`, `TmdNativeAim=0`; other accepted settings
remain as recorded in the candidate plan. Raise/move the left hand, use Square
on a valid target and L2, then close normally. Expected bounded receipts:
`TMD tracked beam: ... armed`, `... owned aging attachment ... nativeParent=...
trackedParent=...`. If the actual owned attachment is rewritten and FX still
originates elsewhere, this hypothesis is falsified for that visible effect;
trace the remaining draw/FX producer instead of moving the glove arbitrarily.
Black silhouette and Seeker practical accuracy are **still unresolved** and do
not inherit a PASS from this attachment correction. No Steam files changed.

### TMD socket correction: implemented, physical FX consumption pending

The original native `GetSocketWorldLocationAndRotation` reads the original CPU
skeletal pose. Independent VR tracking changes the final GPU palette instead.
Consequently, the original socket result does not include the displayed wrist
delta. The exact named socket is original `TMD_Muzzle`, bone `l_arm_wrist` (9),
relative location `(5.02409,-7.532,-0.710843)` UU, rotator
Pitch/Yaw/Roll `-2495/-3631/1137`. These are original socket properties, not a
guessed controller offset or raw ActorX point conversion.

Default-OFF `[Render] TmdTrackedMuzzle=1` publishes that wrist only after both
material-0 eye draws succeed and index/hand/reload palettes restore. It composes
original bind * final consumed skin * local * actual camera ride + camera world
translation once. The socket's original relative transform is then applied.
No hand mesh, input, power/energy, projectile, camera or particle assets change.
Only the successful original query for that socket on the exact locally owned
TMD component is rewritten. Unknown names, other components, non-engine threads,
failed original queries, stale (>150ms) wrists, support, cinema and menus forward
unchanged. Missing evidence cannot produce an AIM-space fallback.

Native helper contracts verified against the original executable/text:
wrapper `AF8880` 257 bytes FNV `3BF1257B240D565F`, socket helper `AF85E0`
304 bytes `2D4ED554DDC30DA5`, original matrix-to-rotator `493D90` 282 bytes
`BF658FB17AAD44DA`. Native caller pushes rotator*, position*, FName.number,
FName.index with component in ECX. Native RET16 owns cleanup. The actual compiled
MSVC callback preserves ESI/EDI, forwards those four dwords, balances its optional
cdecl rewrite and returns RET16. `tools/fx/audit_tmd_socket.py` records complete
native/compiled stack accounting in ignored private evidence.

Historical socket-only limit (the attachment producer is now audited above):
original script `BeginAgingFX` uses `mMuzzleSocket` for sound;
`AttachComponentsToOwnerMesh` attaches `mPalmLight` to it. That does **not** prove
the Square beam consumes the world-query helper. Its runtime consumption remains
UNPROVEN. Do not call Square FX fixed from this implementation or CPU tests.
Bounded receipts distinguish: hook armed, both-eye wrist published, successful
TMD_Muzzle query rewrite, other original socket names (unchanged), or first gate.
If the query is not consumed during Square, trace that actual producer next;
do not redirect GetAgingVectors' cached target as though it were an emitter.

The local TMD owner resolver now uses the existing reciprocal native player
chain; this session had cameraPawn=0 with a valid native pawn. Existing
`[Input] TmdNativeAim=0` remains OFF for this experiment.

### Black duplicate: first refusal probe, no speculative broader suppression

The session learned eight restored color siblings, but omitted **zero** primes
and logged **zero** kept exact siblings. The automatic trace had only phase-0
draws and no weapon/TMD body ranges; it cannot identify the silhouette producer.
Therefore the black-pixel cause remains UNPROVEN and the existing Skip predicate
is unchanged.

Default-OFF `[Diagnostic] TrackedDepthPrimeProbe=1` inspects only previously
learned ranges without changing graphics state. At most eight weapon/hand/support
contexts x 32 inspections and 64 unique receipts prevent idle weapon draws from
using up the entire budget before TMD is raised. Receipts include first failed
gate, exact VB/IB/range, requested/actual viewport, RT/VS/PS identities, depth,
color and stencil. COM references are released immediately; no GPU readback,
flush, new pose or broad shadow/foreground mask is introduced.

### Seeker: user-authorized native lifecycle and post-Spawn observation

The latest session resolved a valid owned Seeker and retained it. Actual Grip
attempts failed the existing zone or busy gates (18.1/47.5/43.7 cm versus 16 cm,
or hip busy). That is different from the earlier ownership refusal. No guessed
back-zone movement is justified.

The user permits native heavy-weapon behavior. Candidate `[Input]
SeekerBackSlot=0` disables the optional retention/retrieval hook: original
SwitchWeapon can clear/drop the heavy weapon on switching away. There is no
inventory grant, fake ownership, save edit or added ammunition. Ordinary weapon
retention remains unchanged. Reacquire a legitimately dropped Seeker if needed.

Twelve earlier launch arguments matched the calculated muzzle, but did not prove
the initialized flight vector. A new read-only receipt inspects the actual native
Spawn result after original initialization: Location, Velocity and Rotation,
only for the first twelve scoped launches. Original Engine.u independently
declares all three StructProperty; Velocity is child 431/458. Only this diagnostic
opts into a 4096-field typed lookup budget; the RTT helper's default stays 400.
The native rocket's drunk/guided behavior, recoil and spread remain authoritative.
Practical accuracy is still FAILED/PENDING, not declared repaired by an input log.

### Follow-up validation and next controlled session

- PASS: x86 socket geometry/adapter (four rotations, both-eye publication,
  stale/identity/context/failure/OFF gates, 100 calls with preserved ESP).
- PASS: Seeker production walker/selector/projectile adapter, including bounded
  post-Spawn observation and byte-for-byte unchanged native object.
- PASS: 20 dual-TMD CPU programs and Python wiring/WeaponLocker preview checks;
  15 muzzle/TMD/aim/shadow/holster programs and associated Python regressions;
  HUD geometry/orientation/replayed poses; TMD runtime CPU checks.
- PASS: OLED CPU and real D3D11 GPU comparisons, all presets within 1 LSB.
- BLOCKED in this session: D3D9/Ex pixel tests (depth prime, HUD and SVTG/Seeker
  indices), HAL CreateDevice `8876086A` (D3DERR_NOTAVAILABLE). These were compiled,
  but their pixel/state assertions did not execute. Earlier passes are not a
  pass for this new binary. No test gate was weakened.
- Production PE32 `/O2`, native socket/Seeker machine-code audit and literal
  WeaponLocker `[eax+20Ch]` check: see the final clean-source candidate manifest.
- L3 and PSVR2: NOT RUN for this follow-up. No physical PASS.

On a new user `start`, MAIN deploys through the existing guarded tooling with
accepted DLL/INI/profile hashes recorded. One headset session, no chat switching:
hold a normal weapon and look at floor/wall; raise TMD, use Square and L2 a few
times while moving only the left hand; shoot 2–3 ordinary shots. If Seeker is
available, pick it up legitimately and fire in free space, then switch away
(native drop is expected; do not test back retrieval). Close normally. MAIN
correlates the socket/refusal/post-Spawn receipts with the user's result.

## Earlier candidate research (historical; acceptance superseded above)

## Seeker: first proven refusal

The latest failed physical-session log, archived privately as
`build/main-seeker-evidence/full-session.log`, SHA256
`7AF4D931833FFA220F04C98F9ACF664A9C68DD475978665BDBF06BFA99EA0437`,
reports `stage=instigator-offset`, `offset=FFFFFFFF`, `hook=00000000` and
`available=00000000` (e.g. lines 598, 12226, 23009).
Both retention and projectile hook installation depend on this resolver.
An earlier in-zone Grip was 10.6 cm from the existing 16 cm slot. Moving the
back slot does not correct the failed ownership producer.

The production `PropOffsetUnder` traversed at most 400 direct UFields. Actor's
serialized direct chain has **458** members; `Instigator` is **417**, while
`Location` is 304. The reproducible read-only package audit is
`tools/reload/audit_actor_property_chain.py`. Private `Engine.u` SHA256:
`B6B481EFDC1FFDA164A24FB25F2FDF57783F2860D36EE1925A1DE6770B971A34`.
This establishes serialized source order, independently of live UObject order.

The minimal correction extends the same readable-pointer, name-index and
offset-checked walk to 4096, matching the existing typed resolver's corruption
backstop. The production function is in `native_property_list.inl` so the x86
test executes the real implementation. There are no inventory writes, grants,
new selectors, guessed owners, altered gestures or relaxed ABI fingerprints.

The named native Actor serializer at `008B64FB` and PhysicalFireStart at
`00E6A1FB` independently use **+CC**. This audited offset is preserved, as are
SwitchWeapon's exact heavy-query caller gates and SelectOwned's FName/ret8 ABI.
Native spread, recoil, original projectile class and guided steering remain.
Valkyrie's accepted factors remain 70% base x 50% supported = 35% effective.

A resolver-only MAIN Steam smoke with DLL `E8BE676F...` progressed to
`stage=no-owned-seeker ... count=2 valid=1`; the previous Instigator refusal is
gone. The loaded save had no Seeker. HMD rendering/controller validation was
unavailable in that smoke. **This does not prove retention, retrieval, launch,
support or save/load.** The accepted DLL and INI were restored and hashed.

## Black silhouette: bounded, independently switchable correction

Confirmed facts:

- Latest screenshots contain a solid black weapon/arm-shaped region behind the
  tracked weapon, with world decals over it. A screenshot alone does not prove
  whether its producer is a shadow, depth mask or foreground composite.
- In the same failed log, frame 6521 has a Valkyrie right-hand consume with
  `applied=0 sourceFrame=6488 valid=0`, followed by another consume in frame
  6521 with `applied=1 sourceFrame=6521 valid=1` (lines 14829–14832).
  The current-frame wrist source arrives between these submissions.
- `docs/handoffs/sol-dual-hand-action-pose.md` explicitly records this ordering
  limitation. Reusing an old pose is forbidden and remains forbidden.
- `HISTORY.md`, “The first-person pass, and how it was found”, records original
  near-plane `SetViewport MinZ=MaxZ=0` depth primes and the black hole produced
  when their pose differs from the late foreground color. The later ClearGate
  investigation proved exact retained-buffer prime/color siblings in a real
  capture (`docs/shadow-clear-gate-experiment.md`).

Hypothesis: the current independent-pose prime/color disagreement can leave
an obsolete near-plane depth mask in the world image. **The latest session
does not yet identify the exact black-pixel producing draw.** Do not promote
this hypothesis to a physically proven cause.

`[Render] TrackedDepthPrimeFix=1` enables the controlled correction, default
**0**. It omits only a depth-only native near-plane prime of an exact retained
late-color sibling. All of these gates are mandatory:

- VR gameplay on the render thread, current pawn/weapon, both tracked hands,
  independent action-pose feature enabled; no pause, cinema, native animated
  hands, holster or menu.
- Successful native **requested** viewport 0/0 and unchanged measured actual
  viewport. D3D9/Ex on this host canonicalizes it to 0/.001: no guessed epsilon.
- Indexed triangle list, scene target, non-shadow layout, outside the confirmed
  late foreground pass, depth test and depth writes ON, color writes **zero**,
  stencil **OFF**.
- Identical retained VB, IB, stream offset/stride, base vertex, minimum index,
  vertex count, first index and primitive count. Vertex count alone cannot match.
- Sibling learned after two successful late tracked-color eye submissions and
  successful IB/hand/reload-palette restoration; same pawn/weapon and <=2 frames
  old. COM references prevent address recycling. Reset/owner change clears it.

This writes no transforms or graphics state. World color, world shadow maps,
the native color pass, alpha/stencil writers, unmatched geometry, native scope,
and OFF behavior are preserved. Unknown first-frame geometry is forwarded.
The existing independent-hand pose, shader math and shadow fixes are unchanged.

At most 8 receipts of each type, no readback or GPU synchronization:

```
tracked-depth-prime: learned restored color sibling ... eyes=2
tracked-depth-prime: omitted exact late-color sibling ... depthOnly=1 nearZ=0 stencil=0
tracked-depth-prime: kept exact sibling ... z=... write=... color=... stencil=...
```

Falsification: if the artifact remains after the exact affected sibling is
omitted, the prime hypothesis does not explain that artifact. If siblings are
kept for color/stencil/state gates, the correction has deliberately not engaged;
use that evidence to identify the other producer, do not broaden the filter.

## Offline verification

Commands from this worktree; private inputs are not committed:

```
tools/reload/test-seeker.ps1 -IndexAsset ../SingularityVR-sol-seeker/build/seeker-private/seeker.ib
tools/reload/test-tracked-depth-prime.ps1
tools/tests/test-dual-weapon-tmd.ps1 -SkipOled
tools/gloves/test-tmd-runtime-regressions.ps1 -Asset <private tmd_short_gauntlet.svtg>
tools/hud/test-diegetic-hud.ps1
tools/test-texture-bridge-main.ps1 -OutputDirectory <repo>/deployment/hands-dubbing/main-seeker-bridge-test
spikes/view_matrix/build.ps1
python tools/reload/audit_actor_property_chain.py <private Engine.u> --report <private report.json>
python tools/reload/audit_seeker_native.py --exe <original exe> --text <unpacked text> --dll spikes/view_matrix/d3d9.dll --report <private report.json>
python tools/reload/test_locker_compiled_abi.py spikes/view_matrix/d3d9.dll
```

The new actual D3D9/Ex pixel test reproduces old-pose depth prime -> world ->
new-pose color in both eyes. OFF leaves black pixels; ON restores exact world
color `204080`, while weapon pixels remain `FFFFFF`. It also tests context,
identity/range/state refusal, missing bindings, expired/changed owners, failed
viewport observation, failed restoration proof, COM release and Reset/recreation.
These are synthetic pixels, **not the user's native game pipeline**.

Seeker tests execute the production property walker, actual native adapter
(100 owned-selector calls, byte-exact ammo conservation), actual projectile
adapter, and D3D9/Ex source-index identity/cache/Reset. Existing tests protect
TMD/support, independent action pose, reload, accuracy, HUD, OLED, texture bridge
and WeaponLocker. The emitted x86 factory literal `[eax+20Ch]` remains checked.

L1/L2 results and exact final artifact hashes are recorded in the adjacent
candidate manifest after the final clean-source build. Physical acceptance is
**PENDING** for the combined candidate.

## One PSVR2 acceptance session (MAIN / user)

Use a save where Seeker is legitimately owned or can be picked up again; this
patch cannot restore a Seeker previously dropped/lost in the save.

1. Hold Seeker, switch to an ordinary weapon, release Grip, then use right Grip
   at the existing lower-right back slot. Release and repeat to return to the
   previous ordinary weapon. Check ammunition and native rocket steering.
2. With Seeker out, left Grip at its support area: check the complete left glove;
   release it, then try L2/Square with the right weapon still tracked and R2 with
   the TMD still visible.
3. Move weapon/hands while looking at the floor/wall: check the solid black
   silhouette, normal world shadows, stereo and frame rate. Close normally.

Do not type in the headset. MAIN reads the bounded receipts after exit.
Support, save/load and physical rendering remain unverified until their actual
acceptance observations are recorded. Keep the accepted 610F installation
checkpoint and exact deployment change plan for rollback.
