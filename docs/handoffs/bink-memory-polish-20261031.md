# In-game Bink Polish routing — 2026-10-31

Baseline: accepted MAIN `2e9d5f32`, installed DLL SHA `147C3297211CFA6CA6B964E29FBBD79538B7EE69D7BC155861B66F67B24BDC96`. Branch `fix/bink-memory-polish-audio`. Seeker remains on its independent integration branch; no unaccepted reload changes imported.

## Two proven defects

Actual archived gameplay log repeatedly records native selection `[0,1,2,3]`, followed by BinkOpen flags **04004400** and `<not a readable filename>`. `BinkObserveName` deliberately refuses BINKFROMMEMORY, and `BinkPlanIntroAudio` previously required **01004400**. Expanded movie-name routing never executes for these memory-backed game films. Observation does not identify which movie each historical anonymous buffer contained.

The old map also confused audio ordinal with native track ID. Actual installed BIKi headers and ffprobe agree:

- Eleven-track story movies: IDs `0,1,2,3,4,5,6,7,8,9,10`.
- Barisov film: IDs `0,1,3,4,5,6,7,8,9,10`; Polish ordinal 9, **ID 10**.
- MA2/MA6/PR99/VI2 reels: IDs `0,1,4,5,6,7,8,9,10`; Polish ordinal 8, **ID 10**.

The already supplied language audit locates Polish at the last audio ordinal. ID 8/9 is not the Polish identifier in the smaller files. Installed codec `_BinkGetTrackID@8` disassembly indexes its ID array by ordinal, and the actual offline codec fixture confirms MA2 ordinal 8 returns 10, ordinal 6 returns 8. No guessed BINK struct access is introduced into runtime code.

## Minimal correction and falsification

Before enabling existing Bink hooks, cache exactly 512 bytes from each of the twelve known local story movies. Validate actual file size, BIKi signature, bounded track table and presence of ID 10. Prefix includes audio IDs and frame index. During observed memory Open, compare a fault-contained read against that exact local catalog. Unknown, missing, corrupt, ambiguous or inaccessible input preserves native playback. No pathname inference from raw movie bytes, no distributed proprietary bytes, no files/allocations in Open, no full-movie hashing.

Only identified memory sources using the observed exact flags can use the existing selection transaction. All twelve map to ID 10. Preserve native tracks 0/2/3, single Open call, native success/failure post-selection, error state, lock/thread, intro menu latch, original movies, and existing Polish/off configuration. No dubbing files, movie replacements or new audio mixer.

Falsifier: catalog ready but the next affected real film reports unidentified memory, or reports `source=verified-memory track1->10` but still plays the wrong language. In that case inspect that precise producer/volume/content path; do not map unclassified buffers globally.

## Verification and remaining gate

`tools/tests/test-bink-polish.ps1 -GameRoot <owned game>` runs optimized x86 fixtures for all movie plans, OFF/unsupported/native layouts, failed Open restoration, filename/OS-handle observation, inaccessible memory, missing/ambiguous/different-prefix rejection. Against the installed files it validates all twelve exact identities and opens real MA2 bytes with the installed codec using the production selection helper. Native GetTrackID proves the ordinal/ID distinction. No game process, rendering or playback is launched by this fixture.

Build and broader regression identities are recorded after completion below. Steam remains unchanged. Physical audio acceptance PENDING: play one affected projector/laptop film and confirm Polish narration with music/effects retained; intro remains the accepted control. Expected bounded receipt: `bink-audio: Polish movie <name> track1->10 ... source=verified-memory`.

## Built candidate and exact test results

- Production source `039b0a0f522cba7d008504ede8c35020a5fce7f8`, x86 PE32 `/O2` build PASS.
- DLL SHA-256: `221297C345FB33AB0B81F351EF64024D7E42466DCC84ACE78448E77392AEA775`.
- Private candidate/receipt: `build/next-weapon-reload/bink-memory-20261031/`; installed Steam DLL is unchanged `147C3297...`.
- Focused Bink fixtures including installed codec/native ID query and all twelve local movie identities: PASS.
- Real D3D9 and D3D9Ex HUD occlusion pixels: PASS. Actual compiled Locker factory literal `[eax+20Ch]` and FName stack regression: PASS.
- Thirteen post-opening C++ fixtures: PASS. Runner's desktop mirror initially lacked `user32.lib` (`EqualRect`); corrected test linkage and included the new Bink fixtures.
- Broader runner's **subsequent legacy source-policy check FAILS** at `validate-runtime-policy.ps1:12`: it expects the obsolete literal `VrUiProjectEye(eye,point,projected)`, while accepted MAIN already uses `VrUiProjectPointerPoint`. `vr_menu_overlay.h` is byte-for-byte unchanged against origin/main. No UI code or policy gate was bypassed to conceal that existing failure; do not describe this whole script as PASS.
- No Steam deployment/game launch, no film replacement, no synthetic voice generation. In-game language and affected-film identity receipt remain PENDING.

Rollback switch remains `[Localization] PolishIntroAudioFix=0`; native selection is forwarded unchanged. This does not disable unrelated Polish text/dubbing.
