# Dual TMD follow-up: bounded first-failure evidence

## Accepted physical observations

The combined AC3EA5B3 candidate removed the solid black foreground shape. The
ordinary left Grip now reaches Volk/Valkyrie. The shoulder magazine no longer
flickers. The remaining PSVR2 observations are: Square/Age effect starts beside
the visible TMD, the gun follows head motion after L2, and the Seeker left glove
flickers slightly during two-hand support. These three are not accepted fixes.

## Existing log evidence

`build/main-seeker-evidence/session-20261025-213034-AC3b/view_matrix.log`:

- At frame 1849 the same-frame dual gun correction submitted a final 3D gun
  position `(19.77,10.51,-107.97)` in both gun-body draws. At L2 transition
  frame 2394 it refused both body draws despite `refValid=1`, `refSame=1`,
  `refAge=31ms`, then forwarded the native palette. The old log recorded no
  mathematical rejection stage or failing source matrices.
- The native Age beam attachment received a tracked parent in frames 1865–1875.
  `RvTMDPower_Age::TickFX` at `FE4570` actually reads the child component's
  `LocalToWorld` translation at `beam+0xA0` as beam start. Prior logging was
  before the component update, so it did not prove this consumed value moved.
- During sampled Seeker Grip frames 10864–10866, the material-0 glove drew
  `3994` SVNC faces in both eyes with a current target frame, while material-1
  drew `7110` SVTG faces in both eyes. The bounded sample cannot exclude an
  intermittent lost tracking/target/palette frame later in the hold.

## One controlled measurement

The next default-OFF code change adds only bounded receipts; it changes no
pose, gameplay input, shader constants, GPU state or native beam arguments.

- First refusal per `dualtmdgunpose::CorrectGate`, with the exact root, local,
  ride, raw controller and cached reference matrices if the refusal is
  mathematical. The gate distinguishes stale tracking, non-rigid source and
  failed final witness. No broad adjustment should precede this distinction.
- After the existing audited Age attachment update, read `beam+0xA0` and the
  tracked TMD socket in world coordinates for at most 12 calls. If the consumed
  source equals the corrected parent yet appears beside the glove, the source
  socket/visual ownership is at fault; if it stays native, the child update did
  not propagate the parent into `LocalToWorld`.
- Log only Seeker support eligibility/target transitions and first palette
  fallback. This distinguishes a missing wrist/pose from actual draw flicker.

One PSVR2 run: load the same save, hold Volk or Valkyrie, press L2 and move the
head with the right controller held steady; use Square once on a visible target;
briefly hold Seeker with left Grip if it is available. Exit normally. MAIN reads
the bounded receipts and hash-verifies restoration before changing the three
paths. Do not call this diagnostic DLL a visual fix.
