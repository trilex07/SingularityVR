# Failed-hud-session evidence (milestone-2 diegetic candidate) — 2026-10-24

## What this archive is

Complete `%LOCALAPPDATA%\SingularityVR\view_matrix.log` of the user's run
started `2026-10-24 12:09:40` on the milestone-2 diegetic candidate
(dll `B5F2A2AD4ADB…`, INI `257F9DFB…`, atlas `9ED4A5C…81F9`). 954 496 bytes,
7856 lines; local-only per the `*.log` gitignore (this analysis is the
committed record; same policy as the other evidence dirs).

## Receipts of PHYSICAL FAIL — NOT VISIBLE

- `diegetic hud: ENABLED (weapon counters centurion/valkyrie/spikeshot,
  default off, atlas vr-assets/hud/diegetic)` — the milestone-2 build's own
  startup receipt (new mount table present in the live dll).
- `diegetic hud: atlas unavailable; diegetic counter hidden` — logged once.
  The diegetic DRAW RAN (mount lookup passed, gates open), then the atlas
  load FAILED. User verdict: "I cannot see the new weapon HUD, only the old
  one."
- `ammo plate: HUD cache clip+1404 bin+1400 size+1408 on RvHUD` +
  `ammo plate: drew plate=2 clip=6 bin=59 506x168 at 1754,2050 (backbuffer
  5640x2876)` — the legacy plate 2 (Spikeshot, LIVE clip/reserve values)
  kept rendering, exactly matching the user's "only the old one".
- `diegetic hud: gate blocked (flag=1 wheel=1 armed=1 hand=1 eyes=1)` — the
  wheel opened later; the diegetic gate correctly blocked (bounded line).
- Zero `diegetic hud: drew …` lines for the whole session.

## Root cause (proven against code + deploy record)

`LoadAmmoPng` built the runtime path from `g_ammoAssetRoot`
(`vr-assets/hud/katorga/ammo` — the legacy plates' directory) for EVERY
caller. The diegetic atlas was deployed to
`Binaries/vr-assets/hud/diegetic/diegetic_hud_atlas.png` (whitelist-verified
during the milestone-1 session-assets delta), so `CreateDecoderFromFilename`
opened a nonexistent file, `artState := -1` (sticky), the draw returned
before any submission, and the plate back-off (keyed on artState==1, not on
an actual render) suppressed nothing. Both the path and the back-off
semantics were corrected in `39f3a6f2` (per-loader roots + per-frame
per-plate atomic drew-latch; diegetic runs before the plates in the frame).

## Deployment state at archive time (verified)

- dll live `B5F2A2AD4ADB53BBA4DEC5369CD5F2A5FD1EA06373C6F1E1E58E374633B58613`
- INI live `257F9DFB14A4483AA81A0CB99EEB74EC1FC45A23192067AEE29355B129741301`
- diegetic atlas `9ED4A5C365E5521C12EFFC7E3E7C9DF7833EAC2B4B71D45FCAB18ADD4B8D81F9`
- accepted M02/M09 fallback preserved (install-backups chain from the
  combined acceptance; dinput8 pair untouched).
