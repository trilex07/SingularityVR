# Volk S4 carried magazine — SIDE investigation

Base: the physically tested `af580e6` assembly candidate. Steam was not touched.

## Observation and hypothesis

The 2026-10-23 gameplay log records kind 2 (Volk), `magazine-ejected`, three
`grab-fresh` events, `inserted/request`, and `native-ammo-ack` (5/63 to 6/62).
This is the mod's manual FSM, not merely an automatic native reload. The
aggregate `nativeMagazineDraws` rises to 1512, but mixes dropped, shoulder,
and carried copies and does not establish a visible magazine in the palm.

`ReloadPalmGrip=1` and `ReloadNativeMagazine=1`. In this mode the Carrying
copy is submitted only if `g_nextWrist.Current()` matches transaction/weapon,
age <=150 ms, and its physical matrix is valid. The existing wrist publisher
requires `gloves` (the noTMD material-0 mesh, 3314/2924 vertices) and a native
glove pose. After TMD acquisition, the live material-0 glove can instead be
the TMD mesh (7444/2626 vertices, 4402 faces), for which this producer never
stages a wrist. The same session proves the TMD material-0 mesh was drawn in
both eyes with SVNC, but its one-shot receipt predates the reload. Thus the
missing TMD wrist producer is a source-level contract violation; whether it
was the first rejection of this particular carry still requires a bounded
runtime receipt.

**Falsifier:** if the next Volk Carrying interval has a fresh same-transaction
wrist witness and a successful carry replay in both eyes, the invisible copy
is farther downstream (palette, depth/material, or placement); no AIM-space
fallback should be added. A rejected/missing wrist with a drawn TMD glove
supports the identified producer gap.

## Isolated correction

The material-0 TMD palette maps slot 1 to the original left-wrist bone 9.
For **Volk only**, Carrying may now take the post-pose skin matrix from this
section and the original `armnatural::TmdBind().bind[9]`. It passes through the
same `WristTracking` coordinate conversion and `Wrist::Current` identity/age
checks as the accepted noTMD route. A TMD witness is published only after a
nonempty stereo draw and successful index/palette restoration. No mesh,
TMD hardware, two-hand support, native ammunition, or AIM-space fallback was
changed. Valkyrie/Kasimov remain on their previous noTMD route.

Bounded receipts for the next MAIN test:

- `Volk carry wrist: TMD material-0 published...` means a wrist was generated
  from a submitted TMD glove section, not that the magazine was visible.
- `next reload draw gate: stage=10 kind=2` means the weapon draw could not use
  a current same-transaction wrist; stage 11 means the wrist existed but the
  physical carry matrix failed; stage 12 means the carried spare palette was
  queued. The existing replay draw counter should then rise during Carrying.
- If stage 12 and replay draws occur but the magazine is invisible, investigate
  texture/depth/stereo or placement. Do not claim this correction physically
  fixed the symptom without PSVR2 confirmation.

Offline checks: `test-weapon-carry-ray` (/O2 x86, including two TMD wrist
rotations and stale/foreign refusals), `test_carry_ray_wiring.py`, short TMD
gauntlet suite, dual-weapon/TMD regression suite, and optimized PE32 DLL build
all passed. The build does not replace the accepted Steam DLL.
