# Final SIDE integration — 2026-10-24

Branch: `integration/astra-final-side-candidate`. No Steam launch, deployment or configuration writes. This is an offline integration candidate, not an accepted release.

## Actual integration

Started at HUD d79ad975. Merged 09887b18 without conflicts (ad4f5b15), preserving detailed icons, native spread policy and the single-owner slot-65 bridge. Cherry-picked reload comfort 54bd946 as 56a157ab; imported hires tools/manifests 132d24d1 and d837d68 as 35ab3b33/c936f7f1. HUD draw/projection headers remain unchanged. No input, aiming or TMD redesign.

Valkyrie/Volk insertion radius is 18 cm; Kasimov remains 16 cm and Centurion separate. Native Grip release and ammo transactions remain authoritative. Accuracy defaults remain 100%; optional balance settings are separate from the candidate's normal configuration.

Added fail-closed private staging for only four first-gate DDS maps. All seven private candidate hashes, source hashes/FNV, layouts and complete mip chains verified. First four copied to the private path in the JSON manifest. No proprietary DDS/WAV/package content is committed.

NIS source-wiring regression still expected pre-OLED g_frameSrc11. Updated it to assert actual displaySrc consumption; renderer unchanged. NIS pixel tests already passed. OLED processing precedes NIS; do not bypass displaySrc or combine independently allocated targets.

## Validation

Unified x86 PE32 /O2 build PASS. Actual DLL disassembly retains literal [eax+20Ch] and FName stack setup. Dual TMD/reload suite PASS, including 30,000 reload transactions and 4,000 carry samples. HUD actual D3D9/Ex atlas tests: 108 cases each; 24 projection cases. Texture bridge D3D9/Ex, UpdateTexture invalidation, Reset, COM ownership tests PASS. TMD material replacement submits 2x7110 triangles with restored IB in offline D3D9/Ex test. OLED CPU/GPU comparison PASS. Haptics 7 tests, voice coverage 4, native-bank tools 4, voice staging 3, note manifest and icon tests PASS. See hashed log receipts in JSON; historical failing NIS log is retained and superseded only by the passing wiring rerun.

Font synthetic checks PASS; the optional existing-package check fails the patcher's original-package pin. This does not prove missing Polish glyphs: the package may already be patched. Do not replace it with the old patcher's input. MAIN must reconcile its accepted hash before any font work.

## Audio and remaining scope

ba8774c audio tools/inventory are already byte-identical on this base. Eight private targets / 24 audition WAV variants passed PCM rate/channel/duration/metadata checks. This is NOT SoundCue playback, true-peak or listening acceptance. Existing prepare_xtts_native_banks.py and stage_xtts_native_delta.py preserve cue properties and require accepted profile hashes and explicit package selection. No new native bank produced: reviewed current profile/bank pins and selected take manifest are not established for this candidate. Follow-up: MAIN supplies those inputs, tool stages outside Steam, review package diffs, then separate audio test.

602a5c Pocket-TTS QA is historical, superseded by XTTS tooling; do not overwrite current voices. c3c911d note data is already present; corpus generation claims are not current installed-playback proof. Bink original Polish track routing is retained; old intro-only pilot is not reintegrated. Preserve working language INI. Accepted UI/location behavior is preserved; no new location patch justified by this task. Do not reopen accepted VFX/shadows/cutscenes.

## Review outcome

GO for source review and staged offline candidate; NO-GO for promotion as a stable release until MAIN reconciles its physically accepted HUD tip. User-reported mirrored counter and left-hand offset are unresolved, owned by MAIN. No physical PASS claimed. Valkyrie/Kasimov hires remain staged, not enabled: their old broad remaster scope is not an independent safe gate. Centurion and Kasimov diffuse remain blocked by original top-mip/identity evidence.

Binary hashes, source commits, assets, configuration deltas and test receipts: ASTRA_FINAL_CANDIDATE.json.
