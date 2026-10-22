# Short native TMD gauntlet — SIDE L1/L2 candidate

Observed problem: the accepted cut shows all TMD hardware on the tracked left
wrist, but the glove stops too close to the wrist and offers no convincing
short sleeve behind the device. The original mixed material-1 section contains
both hardware and arm, so simply restoring the section would reveal the long
forearm again.

Hypothesis: an index-only composite of the original TMD glove section, all
audited hardware faces and only the short original cuff/attachment islands
will retain the game's skinning, moving parts, UVs and material while removing
the long arm. Falsifiers: a gap between cuff and glove at some wrist angle,
loss of moving TMD parts, wrong material, duplicate arm/hand, one-eye geometry,
or a visible opening when looking down the forearm axis.

The private ActorX PSK index array round-trips uniquely to the cooked package.
Material 1 has 6527 hardware triangles and 1177 arm triangles. Its measured
left-side arm islands have 280/261/176/64/47/6/6 faces. The 280-face island
extends 42.9 UU behind the wrist and is excluded. The other 560 original faces
reach 19.3 UU behind it. A 25-vertex proximal rim on the 176-face sleeve
receives a 23-triangle cap made from original skinned wedges. The original
hardware faces are copied in identical order; no vertices, weights, textures,
shader constants, weapon paths or hardware animation tracks are rewritten.

The generated `SVTG` file is a real runtime index asset, not a preview. The
hook substitutes it for the exact TMD mixed section only inside the original
stereo draw, binding the same native material and bone palette. It reduces the
hardware section from 49 source runs to one indexed draw per eye. The original
TMD glove uses the existing complete native cuff asset (`SVNC`). The new path
also omits the separately submitted 17-face material-2 right-forearm tail
(source weights verified to be exclusively right elbow/forearm, no hardware).
requires `[Render] TmdShortGauntlet=1` plus existing native-glove/cuff gates.
Flag 0, missing/corrupt SVTG, unavailable GPU index buffer, loss of gameplay
focus or a different section preserves the accepted original-hardware cut.
The index buffer is DEFAULT-pool and released on Reset.

## Runtime routing correction

Review found that the generic native-index branch ran before the SVTG branch:
`IndexGuard::valid` was true for both, so the generic glove triangle count
won and the intended 7110-triangle draw was unreachable. The bound-index
dispatch now gives the short TMD buffer priority; the generic cuff/glove path
is unchanged. A missing/corrupt asset or failed allocation/bind uses the
original 49-run hardware-only cut. The separate 17-face tail is omitted only
after two successful short draws and successful index-buffer restoration in
the same stereo frame. Failed draws, stale frames and Reset keep the original
tail. The x86 test calls the same dispatch helper as the hook with a fake
D3D9 index device; it checks two 7110-face eye draws, native flag-OFF routing,
fallback, bind/restore failure and Reset invalidation. This remains L1/L2,
not a physical PSVR2 result.

## Private generation and offline tests

Use original, locally owned PSK and decompressed RvGame; never commit them or
the generated `SVTG` and previews. Output **outside** this repository:

```powershell
py -3 tools/gloves/prepare_tmd_short_gauntlet.py --tmd <private/ch_inview_tmd_mesh.psk> --cooked <private/RvGame.decompressed.xxx> --output <private-output-folder> --preview
.\tools\gloves\test-tmd-short-gauntlet.ps1 -Tmd <private/ch_inview_tmd_mesh.psk> -Cooked <private/RvGame.decompressed.xxx> -Asset <private-output-folder/tmd_short_gauntlet.svtg>
.\spikes\view_matrix\build.ps1
```

MAIN would deploy the exact cache to
`Binaries/vr-assets/hud/katorga/glove-animation/tmd_short_gauntlet.svtg`
through its reviewed exact-file backup/restore process. This SIDE branch did
not touch Steam. `NativeGloveCuffClosure=1`, `GloveNativeEnabled=1` and the
existing native hand mode must already be available. `TmdShortGauntlet` stays
OFF by default. Do not enable from the UI until a controlled physical pass.

L1 on the private source: cooked index identity, all 6527 hardware faces,
560 short-sleeve faces, proximal cap, no long island, five rigid wrist
rotations and fail-closed cache decoder. L2: x86 PE32 `/O2` build. The three
geometry-only PNG previews show outer, inner and axial views; they do not
validate the game's diffuse/normal/specular response, full animation, stereo,
tracking or frame time. No L3/L4 claim.

MAIN physical script, one continuous PSVR2 session: Turn the empty left wrist
through palm-up, palm-down and a 90-degree roll; verify glove, short sleeve and
animated TMD hardware move together without an open seam or long elbow.
Look along the sleeve end, then toggle the flag OFF in a separate controlled
session to compare original material, scale and device motion in both eyes.
Finally use a TMD power and pause/resume to check tracking, device animation,
normal gun aim and FPS; stop and restore the accepted profile for any defect.
