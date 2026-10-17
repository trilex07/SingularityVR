# Centurion physical follow-up — 2026-10-17

SIDE, zero Steam operations. User accepts new visibility but reports carried
clip through glove, health plate covering it, wrong downward case-ejection
gesture/direction, native reload on holstering and full reload after close.

Evidence: last log12:55:09→12:56:28 has native-space cache READY, Loaded at
8201812, ONE close permit8202750, exact conserved ACK8204250. Thus successful
manual backend intentionally invokes UE3's full reload; do not replace it with
guessed inventory writes. Later STORED is followed by clip6/reserve7; native
guard diagnostics saturated at48 before store, so no exact later call identity
is proved. Source DOES prove f.context/guard.valid are cleared on holstering,
which makes the native hook fail open. This is a real coverage hole.

Plan, default-OFF CenturionPhysicalPolish (requires TrackedCylinder): maintain
fresh identity-bound blocking context for holstered Centurion, disallow any
permit there while cancelling gestures/tap; upward muzzle dwell only for this
mode; case travel from chamber toward shooter/gravity, not muzzle. Hide health
plate while carrying ammo (not globally). Fit original cylinder to actual
rendered native palm/wrist, same centre for insertion. Hold right native hand
controller-relative during transaction/tail so ammo authority stays native but
the visible action is only cylinder close. No engine animation-state writes.

Falsifiers: store permits/refills without completed manual sequence, NPC/other
weapons get blocked, downward pose dumps in new mode, case initial velocity
goes toward muzzle, carried target differs from input target, glove/material
state leaks, held right-hand palette drifts under moving native component.
All new results require L1/L2 then MAIN L3/L4; counters are not visual proof.
Read-only frozen log: `build/centurion-reload/feedback-20261017/session-1256.log`,
SHA256 `1096972D274150E5C0F65CB4F541DA25C77AB0771B4ECDA097518F23D14B2EF4`.

## Implemented candidate

One new switch: `[Input] CenturionPhysicalPolish=1` (default0), alongside the
CURRENT tested TrackedCylinder/VisualFollowup/native guard2/tap dependencies.
It requires the existing original noTMD native-glove renderer, independent
hands/isolate2/NoTmdPaletteFix; missing dependencies disable the new mode with
an explicit startup warning, preserving the older accepted visible route.
No new game assets, texture replacement, inventory writes, animation-tree
patches, hooks or Steam/INI edits. Native ammo/notifies still execute.

- Circle opens LEFT as accepted. UPWARD muzzle beyond~33degrees for100ms dumps;
  downward pose does not. Cases leave the cylinder BACK toward shooter/gravity
  (+controller Z, opposite muzzle); existing original mesh/UV/albedo retained.
- Original cylinder at native wrist-local(6,-7,0)UU with original curl45%:
  deterministic fit uses1482 hand triangles/1252 cylinder triangles, ZERO
  surface intersections, no sampled cylinder points buried. Five finger gaps
  are0.0107–0.3928UU. Actual SVNI curl coefficients roundtrip to the original
  source within2e-6, SHA256
  `B4564A69AF904DDE7BE9077EBBC96677D93B42E104DEDD93F94B6BF8ACE46C04`.
  Private fit/orthographic preview: `build/centurion-reload/palm-fit-final/`.
  Original hands PSK SHA78DFF6DB...2513FFA; gun4E18A25D...8FE73D4.
- Pose is transferred through the ACTUAL drawn left wrist, LocalToWorld and
  current draw's camera-ride matrix, not an aim/controller offset. Both native
  clip replay and insertion use the SAME100ms identity/transaction-bound
  rendered palm witness; no fresh witness => no unseen insertion. Publication
  follows successful draw AND palette/index restoration. Clip keeps the
  observed native GUN scale, not a guessed GLB scale. Health plate hides ONLY
  during a fresh Carrying phase and returns on release/cancel/exit.
- Right-hand native shape is captured before close, controller-relative,
  recomposed through CURRENT local/ride/tracking matrices during transaction
  and existing bounded2600ms close tail. Left fingers get the measured partial
  grip only while carrying; original retuned fist/knife thumb stays unchanged
  outside this route. Gun still uses the accepted full-chain hold, so visible
  change should be only cylinder sliding closed280ms, not the reload sweep.
- Store immediately cancels the synthetic tap/revokes its token. The fresh
  owned HOLSTERED Centurion keeps a block-only native context (cannot arm a
  manual permit), while the gesture itself cancels. Unknown/stale contexts,
  lock contention and NPC/foreign objects still take the documented safe
  forwarding fallback; this is not an all-states never-refill guarantee.
  An already admitted native reload is not undone by inventing ammo writes.
  Closing also needs a genuinely quick local sideways/roll sample (>=.45m/s
  or~2rad/s over <=100ms), in addition to the existing7cm/roll excursion and
  100ms Loaded dwell. Slow placement or moving the gun toward the belt does
  not by itself authorize reload. These are authored VR gesture thresholds,
  not guessed engine offsets; legacy OFF controls are unchanged.

New bounded input counters: `centurion physical: ... rightHeld/palmPublished/
palmReplays` (path evidence only). Separate eight-record stored-hook budget:
`centurion native stored: decision=2` means BLOCK. Old48-record diagnostics
cannot hide the first stored-weapon result anymore. No per-draw log/file I/O.

## Regression and physical gate

CPU physical fixture:216 moving-component/controller transforms, preserved left
palette, no late capture, bad-matrix transactional refusal, native size match,
holster token revoke/cannot-arm/NPC forwarding, upward-only dwell, rearward
velocity and stale palm refusal. Exact original native-VB GPU fixture also
exercises the new direct desired-matrix replay in BOTH synthetic eye targets.
Original native-glove asset/101-level pose checks and existing native guard,
reload lifecycle, input routing, original shell GPU/state/Reset checks remain
required. L3/L4 for THIS combined follow-up are UNKNOWN until MAIN's test.

MAIN: integrate SOURCE delta onto CURRENT integration (not this older SIDE DLL),
preserve VFX/Polish/HUD assets/calibration/restore baseline, reviewed build/hash/
lease as usual; add only the new switch. User stays in PSVR2, no mid-test chat:
fire3 shots → Circle → raise muzzle UP and see cases fall toward your side →
take/hold left-shoulder ammo (health plate clears, clip seated between fingers)
→ insert/release → wait1s → flick close (only cylinder moves; right glove/gun
stay tracked; ONE refill). Then fire2 shots, Circle, store WITHOUT reloading,
wait3s and draw: ammo must NOT refill. Close normally; report grip/occlusion,
cases/side, unwanted sweep, store counts, HUD restoration, stereo/FPS. Stop
early if uncomfortable. Native engine reload still exists underneath; do not
call its visible suppression physically accepted merely from compile/counters.

## Final SIDE receipt

Implementation commits9ccbe96 +d078491 on `fix/centurion-visual-feedback`;
**both** are needed (source range6e03006..d078491), not the last small gesture
commit alone. Draft PR48 updated, no merge. L1_PASS24 reload CPU/ABI/resource/
GPU programs +53 Python checks; all repeated on final source. Six glove CPU
programs also pass with the actual original SVNI, including45% carry pose
without changing right-hand palette; glove Python7 PASS/2 optional asset tests
SKIPPED, explicitly not an all-private-asset test claim. Three MAIN compatibility
fixtures rebuilt/reran (ammo layout, scripts readiness, holster marker). All33
retained own-process executables checked PE32/x86. Independent actual original
mesh/curl fit and preview inspected. New desired-matrix GPU replay yields
12974/12975 pixels BOTH eyes, old exported-space control remains0/0.

L2_PASS **clean** source `d078491ed88034ba54cdced7e15bd76a5f86cc9b`, x86 PE32
`/O2`, no performance instrumentation. Existing unrelated warnings remain.
DLL3198464 bytes, SHA256
`9F60D72CA0AC05F17E4356F65F14119B90453622301A08C37E7E3F54605E792B`.
Build receipt SHA256
`34A6291B9D739C5BB314BF82EB79BFD0D258FCDD9061ADA129ABEF8F779EBB10`.
Private candidate:
`C:/Users/Trilex/vr-mods/SingularityVR-centurion-visual-feedback/build/centurion-reload/physical-candidate-d078491/`.
DLL + build receipt + explicit manifest; no INI or original assets bundled.
Earlier private9ccbe96 build is superseded, not the candidate for this route.

Logs: `build/centurion-reload/physical-tests-rapid-final.log`,
`physical-build-final.log`, `physical-glove-original-tests.log`;
fit report SHA256 `0549030BFA81C0D1DAF30A0F8D08BAE62C2BFE549647A52A70C340006E9A0F0A`.
These are NOT installed/accepted/restore identities. Subsequent docs-only
commits do not change the clean source/binary receipt. New L3/L4 UNKNOWN;
whole-game animation suppression, perceptual grip and shadow behavior still
need MAIN's physical route. ZERO Steam launch/deploy/restore/INI/asset writes.
