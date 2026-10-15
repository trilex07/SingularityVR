# Left-hip knife: controlled pre-TMD gesture candidate

SIDE only: no installed-game reads/writes, launches, deployment or L3/L4 claim.
Independent of glove PR #30, based on origin/main 72ad59d.

## Mechanism and falsifier (before integration)

Observed source: `RvInput.ini` in the existing offline Coalesced inventory maps
RightShoulder to `GBA_Impulse`, then `PressChainButton CB_IMPULSE` / release.
The existing mod emits that native XInput button and documents a verified melee
animation before TMD. Do not generalize it to after TMD acquisition.

Offline `RvGame.decompressed.xxx`: export 34081, path
`ch_inview_arms_fightstyles.player_fs.RvGameCombatNode.RvCE_AttackMelee`.
Its parent node's `mNodeName` is `melee`; `mDamageType=RvDamageType_Knife`,
`mDamage=50`, `mLength=96`, `mStartBone=inview_camera`,
`mbTraceUsingBoneRotation=true`, `mEventTag=DoMeleeTrace`.
These are parsed tagged properties, not inferred from an effect name.

Hypothesis: before TMD, a bounded RightShoulder press from a deliberate physical
swing requests this native knife chain, retaining native damage/impact/audio.
Falsifier: a logged SWING/press produces an impulse, no native knife response,
or a nearby target inside the normal camera-based native melee range is not hit.
L3 must establish this; static data alone does not prove the input-to-node link.

**Not physical blade collision:** UE3 still traces from `inview_camera`. The
controller gesture requests that attack; it does not move the CPU trace to the
rendered tip. A sideways stab outside the native view cone will not be a hit.
No synthetic health writes, new damage values, guessed exec calls, animation
graph edits, camera writes or gun-aim edits. After TMD / unknown state: fail closed.

## Implemented experiment

`[Input] HipKnifeEnabled=0` default. Native knife mesh is baked offline into a
local cache (never committed). Neutral vertex colours, not a fabricated native
texture. A tracking-space left belt anchor is head-position-relative and uses
recentre yaw, not current head pitch/yaw. This is a configurable belt approximation,
not body tracking. Free left hand, or right hand when no gun is held / holstered,
can acquire with a fresh grip edge inside the hip radius. Hold grip to keep it;
release returns it to the belt. One knife, no throwing/duplication.

Swing threshold + travel + pickup grace + cooldown + slow rearm + tracking
jump/time-gap guards. Pause, focus loss, death, camera/pawn transitions or TMD
cancel; a fresh grip is required afterward. Holding reserves only that hand's
use/support/trigger inputs; default OFF does not change those paths.

Render once per eye at the final overlay seam, using actual per-eye poses and
the existing symmetric rendered projection. Preserve RT/MRT/DS/viewport and
ALL state; bypass hooked GPU setters so CPU-side shader/pass caches cannot drift.
Private depth self-occludes the knife, but does not occlude it against world geometry.

## Evidence gates

L1_PASS: 147 CPU checks, 360000 state updates in 17.483 ms total (synthetic,
not frame time), five Python parser/axis/format tests, decoder and fake-D3D
RT/MRT/DS/viewport/state/refcount/capture-failure tests. Existing weapon holster
and hand-anchor math regressions pass. Eager CPU mesh load 0.177 ms; no claim
of GPU performance or no runtime hitch. Cache: 662 vertices, 1010 triangles,
2 indexed draws per stereo frame. No per-frame mesh loading or heap allocation.

Local source mesh SHA256:
`DA9303A792361EB735124E620F147E490F26EF5A49D4E6C2E897F0B337F5FB5D`.
Cache SHA256:
`6BC784FB31D19062EB1B35735AB8CD04DBA4B666CE14B61317D06EFDF81598DF`.
Source bounds in source units: X [-1.13583,1.01526], Y [-31.64331,5.00777],
Z [-2.48718,3.34248]; explicit visual length 0.30 m, not a measured physical mount.

Offline tools (run in this checkout, never with installed Steam paths):

```powershell
python tools/knife/prepare_knife.py --mesh <offline-export>/2010_knife.pskx --output build/knife/prepared
python tools/knife/inspect_native_knife.py <offline>/RvGame.decompressed.xxx --output build/knife/native-evidence.json
tools/knife/test-knife.ps1 -PreparedMesh <absolute>/build/knife/prepared/knife.svrk
spikes/view_matrix/build.ps1 -GameBinaries <absolute>/build/never-deploy
```

Build has succeeded x86 PE32 /O2; final clean-source L2 receipt follows separately.
Existing unrelated compiler/static-analysis warnings remain; no new knife-header
warnings in this build. No Steam, real GPU pixels or headset validation.

## MAIN integration boundary

Local candidate will contain DLL, provenance, this cache under
`vr-assets/knife/knife.svrk`, L1 log and offline identity report. It is not a
deployment script and does not overwrite any accepted files. MAIN must use
current installation backup/hash/lease tooling, enable **only**
`[Input] HipKnifeEnabled=1`, and retain all other current INI settings.
Left/Down/Forward centimetre keys tune the head-relative belt approximation;
the blade's grip mounting rotation and native-animation duplication remain
perceptual test debt. This is not ready to enable globally or ship as verified.
Do not replace MAIN's current DLL with this base-main build if that would remove
unmerged plate/wheel/localization work: integrate the PR first and rebuild the
combined branch with a fresh provenance manifest. The cache is game-derived
local data, **not in Git**; release delivery must bake the user's own asset.

MAIN test, one session without chat switching: before acquiring TMD, pick the
knife from the left hip and rotate each free hand, then release to return it.
Make one slow motion (no attack), then a deliberate swing at a close target in
front of the camera; verify one native knife hit, no gun shot or duplicate blade.
Pause/resume, holster/draw the gun and exit normally; check no stuck inputs,
stereo regression, unexpected TMD impulse or frame stalls.
