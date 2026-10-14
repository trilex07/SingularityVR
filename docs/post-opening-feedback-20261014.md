# Post-opening feedback — 2026-10-14

## Reported physical result

The user reports acceptable volume, correct pistol grip after pickup, acceptable
FPS, working UI NARRATOR OFF/ON, and no overlapping save-selection dialogs.
Another MAIN model briefly took over between preparation and this feedback.
Its private candidate metadata lists DLL `999F7A46DFA740CECCEF3CFA409D1177F878F65BABDAAB7199A9D9E44CBD745C`.
Do not conflate this test with the earlier `1454A62B...` candidate or infer
loaded-module identity solely from that metadata.

Open user observations:

- Right glove remains too far away before the first pistol; pistol grip is good.
- Expected SINGULARITY VR branding is absent; user sees Singularity.
- Remaining story NPC dialogue is English; all-game localization is not accepted.
- Notes lack Polish glyphs; current encoding conversion is not a font fix.
- Replace the menu cursor with a concentric-dot pointer.
- Cinema screen should stay at its anchor rather than follow head motion.

## Preserved evidence and cleanup

Latest private lease:
`deployment/hands-dubbing/post-opening-live-20261014-142015`.
Game confirmed closed in full Windows process context. All three guards reported
restoration. MAIN independently checked 4 core files, 34 assets and 1886 WAVs
(1924 total): exact baseline hashes, zero mismatches. The backup leases bind to
the same SHA-256 manifests used for this comparison. Actual texture_override.ini
also returned to `146140CD8DAB8867CBAA15BD13BC983EE90E30EE73E80A6325E83FE2962E5093`.
See private `independent-restore-verification-main.json`.

The substituted model's observer did not start: LaunchWindowSeconds=1800 exceeded
its maximum of 600. Preserve this limitation. MAIN retained the existing current
11,046,306-byte native log as `session-main-preserved.log`; do not claim that the
failed observer independently captured the run.

## Narrowed mechanisms / next experiments

1. Free right glove: the uncommitted pistol change copies `aimTarget` from
   `primary` before the wrist-anchor block replaces `primary` with targetC.
   LocalDeltaPair then consumes stale aimTarget. In no-weapon samples 1/2,
   moved=1 and posedRight51 is unchanged from nativeRight51 while the left
   changes; the camera ride and stale aim target match. This supports a
   right-target ordering hypothesis, not a full world-joint position proof.
   Falsifier: consume the corrected right target for free-hand draws and still
   observe the same offset. Preserve the separately accepted pistol target.
2. Branding: the declared candidate UI hash includes an offline title patch,
   but physical rendering still shows the old title. Resolve the live GFx
   field/producer and any runtime assignment; do not use global substring
   replacement or call deployment itself visual validation.
3. Font: native log reached SPNotepad; send SIDE the bounded glyph task in
   `docs/handoffs/side-polish-note-fonts.md`. No Steam rights for SIDE.
4. Pointer: existing DrawVrMenuPointer renders its hit marker; native cursor
   suppression remains explicitly unresolved in source. Draw a ring/dot only
   after proving how to hide the original cursor without losing input/hit tests.
5. Cinema: current composer fits the image into an SBS half, and capture stamps
   non-stereo poses from the current eye poses. This is not a fixed world-space
   screen. Investigate a scene-entry anchor and submission geometry, keeping
   native cinematic camera, aspect and return-to-gameplay policy unchanged.

Preserve the substituted model's four dirty tracked files. This handoff neither
edits nor commits those changes. No new candidate deployed or game launched.

