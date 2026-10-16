# Frontend background isolation and comfort

SIDE, base origin/main e7e9095. No Steam writes. Symptom: foreground menu too
close; scene behind frontend often corrupt before gameplay and after returning.

Source evidence: DrawVrMenuPointer draws captured GFx with alpha over the native
UE3 backbuffer. It does not own/sanitize the backdrop. SnapshotMenuOnSlot can
then select CinemaMenuIdenticalCrop: discards the right-eye panel projection,
so setting panel distance cannot restore its binocular depth. Existing physical
receipts in cinema-menu-pair-and-revoice.md describe single UI + differing
background after return. This does not identify every corrupt native scene draw.

Hypothesis: isolate the existing current-token class-2 provider over an opaque
neutral background and keep its actual per-eye projections. This deliberately
REPLACES the animated 3D backdrop; it is not a repair of arbitrary UE3 rendering.
Falsifiers: corrupt pixels remain with successful composition; intended menu
texture is missing/incomplete; laser/panel disagree; other menus change.

Experiment is default-OFF `FrontMenuCleanBackdrop=1`, class2 focused MenuFlat
only, never boot/pause/dialogue/Bink/cutscene/mod-panel/modal save confirmation.
Requires current capture ownership and valid geometry for BOTH eyes before
touching pixels. Full original SBS backup, isolated RT/DS/MRT and D3DSBT_ALL,
opaque dark backdrop, same texture/crop and alpha convention, separate eye
scissors. Every failed stage restores state/original pixels; restore failure
latches this experiment OFF until Reset. No CPU readback, new hook, UObject
offset, shader constant or per-frame heap allocation. Cached DEFAULT-pool RT
is released at Reset/menu resource release/dimensions change. Allocation failure
is not retried every frame. One full-backbuffer GPU backup copy per eligible
frame; two menu draws replace the older two provider draws.

The per-captured-frame successful result alone bypasses the old identical-crop
stamping; OFF/failure leaves its old submission policy. No live-state-only
override for a pipelined slot. Gameplay camera/eyes/hand/input/audio unchanged.
The flag itself grants class2 frontend scene-mode ownership (including native
cinematic backdrops), without changing native cinematic state; confirmed video
still has priority. It does not require a fourth INI change. Successful output
uses current XR eye poses, not alternate-frame world poses; a menu generation
token change invalidates the composition receipt.

Comfort uses EXISTING shared render/pointer/fallback helpers, no second producer
inset: `FrontMenuScalePct=80`, `FrontMenuDistanceMm=1800` (relative scale vs the
legacy class2 size, not extra shrink atop the previous 88%). These are three
scoped Render changes; never install this branch's INI wholesale. No new default
for users or changes to holsters. MAIN must bind its reviewed candidate change
plan to the full accepted profile, preserving all calibration/localization.

L1/L2 receipts and MAIN's short physical route follow after verification.

## Verification and integration

`tools/tests/test-frontend-menu-comfort.ps1` compiles/runs six x86 `/O2 /W4 /WX`
CPU executables. New checks cover exact opt-out and eligible class, stale
generation receipt, nine failure stages including partially written pixels,
rollback-failure reporting, Begin/End pairing, scene-mode lifecycle and
1.8m/80% corner laser agreement + actual distinct eye parallax. Existing menu
scale/submit, pause, scene-mode and cinematic fit regressions also PASS.
This is not a native GPU pixel/state test, Steam execution or visual acceptance.

MAIN should integrate the small source delta onto its CURRENT integration
branch; do not deploy this standalone older-base DLL over newer MAIN work
(camera/reload changes exist outside origin/main). Rebuild and bind reviewed
source/INI differences to the accepted-profile deployment guard; inventory all
frozen assets and keep native language/hands/knife settings. Do not bypass the
guard or use the now-disabled build -Install route.

One uninterrupted headset route after MAIN preflight: inspect the main menu
before gameplay (readable title/options, all entries, dark background, laser
alignment); load the accepted save, pause/Options/resume, then return to main
menu and inspect again; exit normally and report comfort/eye agreement/missing
content/FPS. No mid-test chat. MAIN checks `frontend-clean: composed=1` and
`menu-submit-edge: class=2 ... crop=0` plus existing placement evidence. A
successful draw is only route evidence; a wrong/incomplete captured UI source
must reject this candidate. Do not call the native animated backdrop repaired.

L2 receipt 2026-10-16: clean source
`9b23e3ff7de9519216a09859211d836db03d9776`, production x86 PE32 `/O2`,
instrumentation OFF, manifest workingTreeStatus empty. DLL SHA-256
`681D3A790951E9D192377CDD4AC7433F8652A01F7FE2B7D9649315805E878C1E`.
No new frontend-header analyzer warning in the full build; existing unrelated
warnings remain. No Steam deployment/launch, no package/INI/audio asset edits,
no approved-profile promotion. L3 and physical PSVR2 acceptance UNKNOWN.
