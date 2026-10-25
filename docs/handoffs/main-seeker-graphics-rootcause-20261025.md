# MAIN: Seeker resolver and tracked foreground depth prime

Branch: `fix/main-seeker-graphics-rootcause`.
Parent: `218dea7344bcb89cbe630e06141121e0f9007a59` from
`integration/full-psvr2-20261025`. Accepted installation checkpoint:
`610F4C21C4E610C40CDF5381B8D8E98675482E409669329C3EDEF23CCD8AEFF1`.
This is a candidate, not a new physically accepted baseline.

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
