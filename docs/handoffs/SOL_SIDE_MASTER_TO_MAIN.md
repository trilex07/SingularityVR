# SOL master SIDE integration → Cursor MAIN

Branch: `integration/sol-side-master-package-20261023`, based on fetched
`origin/main` `702f0c32adaa7e9d65df45da3f11ffafec9d89f6`. Resolve the
final exact HEAD after fetch (`git rev-parse HEAD`); do not deploy an old DLL
from an earlier branch. SOL did not launch the game, build against the Steam
installation or deploy. Do not merge this branch to `main` before staged
physical acceptance. Details:
[`SIDE_MASTER_INVENTORY.md`](../integration/SIDE_MASTER_INVENTORY.md),
[`MASTER_INTEGRATION_MANIFEST.json`](../integration/MASTER_INTEGRATION_MANIFEST.json),
[`MAIN_PSVR2_TEST_MATRIX.md`](../integration/MAIN_PSVR2_TEST_MATRIX.md),
[`BUILD_AND_REGRESSION_EVIDENCE.md`](../integration/BUILD_AND_REGRESSION_EVIDENCE.md).

## What actually changed

- **M01, runtime code, offline PASS, PSVR2 pending:** `d897dc4` refreshes the
  native TMD ownership read before the reload dispatcher. For Valkyrie alone,
  a knife already disabled by `HipKnifeWithTMD=0` no longer blocks the left
  shoulder magazine gate. A bounded receipt reports the first gate when a
  Grip attempt fails. The existing 800 ms squeeze-before-reach behavior,
  native ammo transfer and Volk/Kasimov reload paths are unchanged. This
  removes one proven contradictory gate; it does **not** establish why every
  past physical pickup failed. The first MAIN test should be M01 only.
- **M02, source assets only:** 3 bilingual/neutral atlases, 163 source sprites,
  generation and pixel QA imported from SIDE. None of these are wired into
  current GFx/menu/HUD rendering; current accepted UI remains unchanged.
- **M04/M06, manifests only:** Bink and DDS private candidates are SHA-checked
  but proprietary binaries are not in this Git branch. Their current engine
  override behavior is not validated; no automatic asset install.
- **M05, tools/docs only:** eight audio comparison sets can be checked
  read-only. Seven cue identities are referenced; ocean/storm remains a
  heuristic match. No UE3 sound-package patcher is integrated.
- **M07, text data only:** 64 unique Polish note IDs staged, without altering
  the installed voicepack, font, note renderer or active localization.

M03 diegetic HUD is **REWORK**, not silently shipped: current main already has
live native health/ammo-backed plates, while SIDE GLBs have static preview
numbers and unproven final bone attachment. M08 older renderer/input patches
were rejected as stale or duplicate. This is intentionally not an all-flags-on
candidate.

## Exact first controlled test (M01)

1. MAIN records current installed DLL/INI/asset SHA256 and obtains a verified
   rollback copy. The handoff historically names `514bb03` as restored accepted
   baseline, but MAIN must inspect the **actual** installation; later
   `05a94ab`/`4B34CE06…` was a partial/failed knife experiment, not a blanket
   acceptance. Do not infer acceptance from current Git `main`.
2. Build the branch x86 `/O2` using `spikes/view_matrix/build.ps1`; record
   `d3d9.build.json`, PE32 machine `0x014C`, optional header `0x010B` and DLL
   SHA256. Preserve all unrelated installed INI values. Keep
   `HipKnifeWithTMD=0` and the already-tested Volk settings; enable only the
   existing Valkyrie manual reload flags necessary for this save. Do not
   enable any SIDE art, DDS, Bink, audio or unverified input override.
3. In one short PSVR2 segment with TMD owned and Valkyrie: Circle eject;
   squeeze left Grip before reaching the left shoulder; reach; observe a
   single fresh magazine, carry and insert; verify native loaded count and
   R2 shot. Repeat once by squeezing inside the shoulder volume. Try one
   support Grip away from the pouch (must not create ammo). Do not prolong
   play on VR discomfort.
4. Close normally, archive the full `view_matrix.log` and user report before
   relaunch. Match `valkyrie shoulder gate` receipts and native ammo
   acknowledgement to physical result. PASS requires all: reliable grab,
   correct left hand mount, native ammo update and shot, no false pickup,
   no TMD/Volk regression. A successful synthetic test is not physical PASS.
5. On failure restore **only M01 candidate DLL/INI** to the recorded accepted
   hashes; keep all unrelated accepted assets. Verify actual restored hashes.

The other groups are independent and sequenced in the test matrix. MAIN must
not deploy M02/M03/M04/M05/M06/M07 merely because they are listed here.
Each requires its missing runtime/proprietary-asset gate first.

## Blocking risks requiring further engineering

1. Valkyrie physical shoulder pickup/left-wrist carry after TMD is still
   unverified; bounded receipts distinguish support, tracking, identity,
   shoulder radius and stale knife/TMD gates.
2. Current UI art has no renderer binding. A later default-OFF visual adapter
   must preserve existing menu geometry, laser hit test and stereo submission.
   Do not merge old Sense action names.
3. Diegetic GLBs have no proven native data/attachment path; current HUD is
   the safe fallback.
4. Audio requires a verified UE3 package replacement mechanism; Bink needs
   runtime decoder validation; DDS needs exact original/override match and
   resource-lifetime proof. Treat these as **separate** MAIN packages.
5. Polish note data is not installed. Font package/glyph coverage and actual
   SPNotepad/GFx ownership must be verified without changing accepted audio.

SOL's one accidental offline font test included a read-only optional check of
the installed `ui_GFx_fonts.xxx`; that assertion failed. The test wrote only a
temporary output, did not modify Steam and was **not** repeated. No font
acceptance is claimed. This violates the intended no-access boundary and is
called out in regression evidence rather than concealed.
