# Opening-gameplay hand deformation — controlled candidate

Observed: stretched gloves after the helicopter cinematic, independent of the
now-repaired L2/R2 texture-cache churn. Existing independent-hand counters prove
execution, not visual correctness. Current user settings ShowControllerArms=1,
ArmIKMode=0, GloveIsolate=2, GloveFaceFilter=0 are retained.

## Producer identity and falsifier

Fresh offline decoding of the installed RvGame package matches the prior mesh
research exactly: logical SHA256
`09DE450D9F620952C3701D874BEB90A8BD9235263975157A641B9E3441336C92`.
The noTMD LOD0 section first=0, triangles=4830 owns chunk0: 2924 vertices
(3314 including separate sleeve), 35 GPU palette slots. The source's legacy
45-entry list disagrees starting at slot18: the actual left wrist is bone9 at
slot18, not the source's clavicle bone6. A palette is an ordered slot map, not
a set that can be extended with supposedly missing skeleton bones.

Independent ActorX export SHA256
`78DFF6DBD7CB1BED569F3C6C6118C47D1BF82812F08BDC5CBF11BA35F2513FFA`
has exactly these 35 weighted contributors in section0, 2586 weighted points.
281 points blend wrist/finger and forearm contributors. With ShowControllerArms=1
and ArmIKMode=0 the old policy leaves forearm native while wrist follows the
controller. This independently creates a deforming seam, even with a correct
slot map. The noTMD `tmd_base` placeholder contributes left cuff weights; it is
not the separately drawn TMD hardware.

Hypothesis: wrong GPU slot order and split skin-weight transforms are sufficient
to produce the reported glove deformation. Falsifiers: another live section/map
is used; correctly mapped rigid skin still stretches; controller validity or
camera transition changes remain responsible. No L4 success claimed.

## Narrow implementation

`[Render] NoTmdPaletteFix=0` defaults OFF. ON selects the exact 35-entry chunk0
map only for the existing noTMD section and shader/scene gates. With existing
GloveIsolate=2 all known same-side skin contributors receive the same controller
delta and optional left shift, preserving authored finger animation. Lost-pad
handling, palette save/restore and support-grip deltas are unchanged.
TMD sections, gun transforms, shadows, world draws, ShowControllerArms and user
settings are not globally redefined. No new hooks or runtime asset reads.

L1_PASS: fresh cooked-map/PSK verification; weighted split reproducer;
affine rigid-blend regression; exact slots and both handedness modes.
Build/Steam/physical gates remain separate. Test candidate is offline until
the user writes a fresh `start`; test must restore both DLLs and any owned
INI/audio-package changes after exit.
