# Combined MAIN candidate — 2026-10-19

Status: offline gates, **not Steam or PSVR2 acceptance**. No game files were
written while preparing this candidate. MAIN waits for a new user `start`.
Branch `fix/vr-settings-rifle-reload`; renderer/settings source includes
`541079f`, with the final clean x86 PE32 `/O2` build receipt staged alongside
the DLL. Do not replace this with a SIDE DLL or a historical audio installer.

## Included and deliberately unresolved

- Minimal VR Settings, physical-right laser/R2 using aggregate OpenXR action
  queries, common draw/hit/reticle geometry. Optional PL/EN reload instructions
  start OFF; test changes are session-only to preserve exact INI rollback.
- Valkyrie ray origin translates both endpoints, original-palm magazine mount;
  observed-identity weapon wheel, original paired Volk grip, NPC name size60%.
  These are new L1/L2 candidates, not claimed physical fixes.
- Explicit `-CombinedSession` opts in Valkyrie/Volk/Kasimov together, instead of
  the default isolated-weapon experiment. The dispatcher selects exactly one
  held weapon. The shared CPU FSM cancels on identity changes; a 90-transition
  regression checks carrying-state isolation and fresh Circle edges. Accepted
  Centurion configuration remains unchanged.
- Accepted native pistol VFX correction, mirror/screenshots, knife, gloves,
  shadows, fonts and calibration are preserved. **Rifle VFX is still unproven**:
  its native effect differs from the audited pistol family. The existing bounded
  idle/three-shot trace is enabled; no speculative shader alias/custom flash.

## Audio, text, textures: actual delivery

Source: SIDE `SingularityVR-dubbing-corrections`, `C:\Dubbing-aktualny`, current
installed banks, and the existing local upscale research. Intake verifies all
1875 unique WAV files (the 1949-row delivery repeats74 identical declarations).
1466 campaign source files staged;409 MP sources audited but never deployed.
Four unchanged float intro WAVs are quantized in PRIVATE copies, gain1, same
samples/rate/timing. Original sources, Bink movies and UI894 audio are untouched.

116 native campaign banks examined;114 replacement banks pass repack-only AND
changed-bank independent C++ LZO decoder roundtrips. **878 native waves updated**;
380 retain their exact baseline payloads:294 exceed safe full-utterance tempo,
80 fail exact encoded sample budget,5 already have a baseline decode/metadata
discrepancy,1 is stereo. No speech cropping, cue/timing/property edits, global
mute or simultaneous duplicate overlay. The initial VI5 failure was a128-frame
baseline discrepancy; the retry retains that wave and updates75 other waves.
Do not describe the entire new pack or all game dialogue as installed/approved.
ASR/formats and byte checks do not prove pronunciation, translation or acting.

The source story lookup is ALREADY identical to the installed SIDE metadata;
almost all1084 standalone story WAVs also match. The narrow delta changes only
one standalone WAV, actual native banks and their exact native-ownership lookup.
Unrelated and retained story/MP lookup rows stay byte-identical.

Text:72 Notes +32 ChronoNote entries merged into the CURRENT English-keyed text
lookup, preserving tokens/paragraphs/unrelated rows. Three source-whitespace or
hash discrepancies were reviewed against the original English inventory; two
leading newlines and the male signature `Dr Pietrow` are retained. Existing
accepted Polish glyph package is protected; no stale Coalesced replacement.

Only actual delivered8192x8192 enemy assets: **Revert diffuse + normal**, BC1,
12 mip levels. Research stores them in a mislabeled mixed tier; source SHA and
DDS bytes/dimensions, not the tier label, establish identity. Current accepted
injector routes these names through `texture-override-remaster`, so that is the
target. No injector/slot65 hook, shader, texture configuration, weapon or other
enemy asset changes. Loader accepts8192 and its existing D3D9Ex upload path is
preserved. Real upload, appearance and VRAM/frame-time impact remain L3/L4.

## Private payload and safe lease

Inside this checkout `build/next-weapon-reload/`:

- `combined-assets-20261019/intake.json`: all sources/SHA/format/text evidence.
- `session-native-audio-20261019/bank-report.json` plus
  `session-native-audio-vi5-retry-20261019/bank-report.json`: native cook evidence.
- `sealed-session-assets-20261019/asset-manifest.json`:119 exact changed targets,
  about253MB, plus15 protected dependency pins; `native-coverage.json` lists
  every retained native wave and its reason. Proprietary assets stay private.
- `npc-name-60pct-oneblock-20261019/`: exact RvGame package pair; only font
  height340->204twips changed. Independent decoder and fake-install restore PASS.

Use `tools/deploy-singularityvr.ps1 -SessionAssetsManifestPath ...` for the narrow
asset delta, NEVER the old broad installer. The DLL/INI accepted-profile lease,
nine reload files, NPC package and campaign assets have disjoint write ownership.
All backups are made and hashed before the first write; actual running-process
visibility is required. Refuse live drift, missing/corrupt backup, path escape,
reparse points or protected-file drift instead of overwriting another change.

`seal_session_plan.py` binds the clean runtime receipt, actual combined INI,
asset manifest, NPC pair and accepted profile with exact hashes into one private
`session-plan.json`. It never installs or launches. Revalidate these identities
at the next MAIN `start`, not just the folder's timestamp.

`watch_rifle_runtime_restore.ps1` receives the session asset manifest/backup and
NPC pair. **Pass `-LaunchWindowSeconds 7200`**:
this is a two-hour BEFORE-launch no-show bound, NOT a running-game timeout.
There is no10-minute rollback. Active sessions are unlimited; after normal exit
and30seconds without a game process, restore all declared before-states and hash
them. Never restore while running or silently promote the experiment to stable.

Offline fake installs prove successful deployment, full verification, running-
game refusal, corrupt-backup refusal, protected/foreign edit preservation and
exact restore (including removal of a newly added, hash-matched file).

## One headset route (no chat/alt-tab during test)

2026-10-19 pre-launch integration correction: the first deployment was fully
restored before any game launch because the core postflight correctly rejected
the changed Polish text/story manifests. The combined watcher now composes an
exact temporary allowance for only those two profile-pinned manifest targets
from the sealed asset delta, with source/before/after SHA checks. It does not
edit the accepted profile or the core install/rollback plan. Campaign assets
are restored before the original core rollback. Synthetic tests reproduce the
old rejection, accept the exact composed state, reject unrelated dependency
drift/bad baseline pins/broader allowances, and verify exact baseline restore.

1. VR Settings: aim at rows and use R2, enable reload hints, switch PL/EN text.
2. Valkyrie: idle5s, three short bursts3s apart into free space; inspect origin,
   VFX and both grips. Circle eject -> LEFT shoulder grip -> insert/release;
   inspect spare near the face, rotate it, reload and fire again.
3. If owned in this save, Volk/Kasimov: same Circle/left-shoulder/insert sequence,
   **no closing flick**. Centurion keeps Circle/open -> muzzle UP/spent cases ->
   shoulder/new ammo -> insert -> closing flick. No item/save edits for access.
4. Weapon wheel, NPC name size, note/temporal glyphs, nearby Polish dialogue,
   enemy surface quality, pause/options, screenshot/mirror. Close normally.

MAIN collects logs automatically, including native ammo acknowledgement,
draw/restore-qualified palm/muzzle witnesses, complete VFX traces and existing
texture substitution logs. A big combined trial is acceptance coverage, not
causal proof of every flag; isolate only a failure found in its actual evidence.
