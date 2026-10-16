# Held knife: blade size, thumb contact and melee display

2026-10-16 SIDE follow-up, `fix/knife-shading-thumb`, PR #35. The user reports
that empty fist, knife colour/shadows and general grip are now good; requests
a slightly larger/longer knife, thumb on the handle, and no left-attack rotation
towards the camera. This is scoped physical feedback, not a SIDE-verified
installed DLL identity. Accepted fist/lighting/material/cuff/mount stay frozen.
No Steam access, launch, deployment or restore by SIDE.

## Mechanism and smallest experiment

`RefreshArmsAnimActive` combines heal and native melee into a display window;
`PrepareIndependentLeft` rejects controller retargeting during that window.
Several foreground-hide/ride rules consume the same cache. Merely bypassing
one early return would leave conflicting ownership. Moreover the raw native
wrist palette still contains its attack rotation: `TargetWrist` corrects the
centre, not this rotation, and the joint knife follows that palette.

New **default-OFF** `[Render] KnifeTrackedMelee=1` keeps the tracked joint display
only for a visible held knife, eligible native gloves, valid owner tracking
and no heal. Other native melee/healing retain their old display policy.
The last non-melee native wrist basis is captured per pawn/pickup/hand; while
attacking, `referenceBasis * inverse(rawBasis) * trackedSkin` removes native
wrist rotation while preserving controller rotation/scale and the already
anchored wrist centre. No reference, nonfinite, sheared or changed-scale input
fails closed; no guessed pose. Reset/regrab/hand/pawn changes invalidate it.
No new hook, engine offset, allocation or file IO in this path.

Native RB attack, cooldown and damage trace are UNCHANGED. This is a visual
ownership experiment, not blade collision or a rewrite of native hit direction.
Bounded notes distinguish kept display from unavailable wrist correction.

Falsifiers: left attack still turns the assembly towards the camera, controller
roll/pitch is lost, a stale pose survives regrab/pawn change, healing/pistol or
empty-fist behaviour changes, or no held-knife render reaches this path. MAIN
must observe the actual palette path before extending scope after a failure.

## Asset changes and measured scope

`tools/knife/retune_knife_grip.py` takes explicit private offline inputs only.
Knife is the original 662-vertex/1010-triangle textured SVRKv2. Only blade points
below knife-local Z=-0.08 m change: blade length +15%, width/thickness +8% with a
2 cm transition. Whole length becomes 0.32685146 m (was 0.29999999 m). Hilt,
guard, vertex colours, UVs, topology and accepted wrist mount are unchanged.
Regenerate SVNL tangent data for the changed blade, not the lighting shader.

Only the held-knife endpoint of accepted thumb cache v3 is re-fitted. Every
empty-fist key/time/quaternion remains byte-identical; other four finger bends,
original native rig/material/scale/cuff and mount remain unchanged. The contact
metric uses a 15-vertex distal pad patch and the HANDLE band Z=(-0.08,-0.04),
not a single corner near the guard. Mean pad gap 0.3584 -> 0.2040 native units;
minimum gap 0.00477/0.00500. Zero measured full hand/knife triangle crossings,
thumb/four-finger crossings and knife vertices inside the glove, each hand.
This is sampled geometry evidence, not proof of convincing headset anatomy.

Private new hashes (SHA-256):

* `knife.svrk`: `D85342AD01CBE6A19192A2634469185DF14924FB8E55E63D975A36DB00F242A0`
* `hand_thumb.svnt`: `E841FF6F5F482D6D687F0461EDBC7E2DD4F0F45703911FD8F39D1DA78F304A3F`
* `knife.svnl`: `FCD8A2D86C3340B176C7D92FE349EDA8C35D13AF4EF837958D76B00E3FFB7474`

Preparation: run `retune_knife_grip.py --psk ... --native ... --mesh ... --grip ...
--thumb ... --golden ... --output ...`, then existing `prepare_knife_lighting.py`
on the new mesh with the same original normal/specular textures. Outputs are
private game-derived assets, never commit/distribute them. CPU preview is not
a GPU or lighting validation.

## Validation and MAIN handoff

L1: blade-only byte invariants, malformed inputs, deterministic resizing;
512 display-policy combinations; wrist rotation through 0..180 degrees each
side, compound axes/nonidentity baseline, preserved controller scale/centre;
reference lifecycle and fail-closed transaction. Existing native 13308-vertex
thumb golden replay, 101 analog levels, shader identity/SM3 compile,
upload/state/holster/anchor regressions pass. The original joint replay also
passes (2218 hand + 1324 knife vertices), separate from the retuned hand golden.
L2_PASS: clean source `6936fd55e55cffaa637847911d450d18b9359490`, `/O2`, x86
PE32 (014C/010B), instrumentation OFF, manifest working-tree status empty.
DLL SHA-256 `0D46D0BA9A13C0F51A7CEECFB836FE88E2BDC7047CB6E6B9522A3F24B44CB43A`.
Existing unrelated compiler warnings remain. L3/GPU and L4 PSVR2 are UNKNOWN.

Private payload and provenance: `build/knife-shading-thumb/retuned-candidate`.
Canonical hand cache path: `vr-assets/hud/katorga/glove-animation/`; knife
caches: `vr-assets/knife/`. New mesh/SVNL/thumb hashes are above. The five
other staged caches match the prior accepted-input bundle byte-for-byte:
original diffuse/normal/specular, original native SVNI and accepted mount SVNG.
Receipt-only commits after this build do not change compiled source.

MAIN must integrate ONLY this follow-up onto CURRENT accepted source and build
there, not deploy this branch's older standalone source over newer menu/Bink
work. Preserve accepted `HandThumbPoseFix=1` / `KnifeSceneLighting=1` and all
calibration; enable only new `KnifeTrackedMelee=1`. Replace only new mesh/SVNL
and thumb cache, retaining original diffuse/normal/specular/native/mount hashes.
No wholesale INI is supplied. MAIN owns backups, exact installed hashes and
restoration of the accepted build if the physical test fails.

Three-sentence physical route (stay in headset until normal exit):

1. Close/release both empty fists, pick up the knife in the left hand, inspect
   blade size and thumb pad on the handle in both eyes, then rotate the wrist.
2. Swing at a nearby target with the left hand, repeat with the right after
   docking/regrabbing; the assembly must follow the pad without a camera-facing
   snap, while native damage is checked separately.
3. Heal, draw the pistol, pause/resume, then exit normally; accepted empty fist,
   cuff, grip, lighting/shadows, stereo and frame rate must remain unchanged.
