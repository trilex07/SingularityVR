# Wrist visibility and Centurion follow-up — 2026-10-30

Physical result for DLL `3FE6FE8EAEAF8D74C4256A8E76306B33302F84FD5C047550CE35D8D9BAA9C87F`: wrist panel invisible; Centurion reload completes, carried cylinder flickers, firing crashes. These results supersede the previous offline acceptance. Save ownership is not established as the crash cause.

## Evidence and limited corrections

- WER report `e42152ed-6401-4345-be15-a202be866b1c`, 10:45:13: execute/read AV `c0000005` in XAudio2_5.dll 9.27.1734.0 at `+4DE73`. No surviving dump/caller stack. This locates the fault, not its originating call.
- Installed `wp_SP_ARPistol_SF.xxx` SHA `A5FA0D72BB6B32D845E6D0FC89990F18D3A87A9FAC444D698689D74233756D39` contains `arPistolFire03` with an invalid padded Vorbis vendor field. The game's actual Vorbis library rejects it (`-133`); the repaired header opens (`0`). Candidate bank SHA `502AAEC045B7C221DAC4E9891F829F7370E8431DB14FAAA4031A414731FA701D` preserves every compressed audio packet and all non-target logical bytes. Independent package unpack matches; decoded PCM is identical: 240000 bytes, SHA `8A143FC1D4454EB456B9EC37260925562590988E49290BE42F8044519766C7ED`. Header defect PROVEN; crash causality INFERRED; crash-free gameplay PENDING.
- Centurion animation previously exhausted four verification attempts on index-buffer identity changes. It now reuses the production retained-COM-resource `centurionhd::IndexWitness`, including the original cooked-content hash and Reset cleanup. Actual D3D9/Ex fixture alternates two valid buffers 24 times without losing eligibility. This is a proven source defect; this session's MATCH receipt alone does not establish it caused the physical cylinder flicker.
- Wrist screen-mask composition incorrectly applied perspective interpolation to UVs that already represent screen pixels. Composite RHW is now 1; native projected Z and compatibility checks remain intact. A tilted-mask four-stripe pixel fixture covers both eyes on D3D9 and Ex. Source/math defect PROVEN; total runtime invisibility remains UNPROVEN.
- Actual session reaches wrist submission in both eyes. Retained world D24S8 `38B0CEE0` and private foreground D24S8 `656F71A0` are 5640x2876; native foreground clear follows world draws. This is submission evidence, not visible-pixel proof. New trace uses four asynchronous occlusion queries once per process, flags 0, no flush/wait, maximum 120 later polls. It records world-mask and alpha-qualified foreground-composite sample counts plus native/XR projection comparison. This distinguishes world-depth rejection, foreground rejection and projection mismatch without changing depth policy or guessing placement.

## Verification

- HUD math, placement, final palette and D3D9/D3D9Ex world/foreground pixels: PASS, including tilted screen-mask sampling and asynchronous query lifetime.
- Centurion original asset hashes, native ABI, tracked cylinder (2916 cases), palm/right-hand cases (216), reload controls and actual D3D9/D3D9Ex index-resource alternation: PASS.
- Fixed-allocation Vorbis tests: 11 PASS; independent native header and identical PCM/package checks: PASS.
- Further build, critical regression and exact install identities are recorded in `wrist-centurion-followup-candidate-20261030.json` when completed. Private evidence: `build/wrist-main/session-20261030-centurion-crash/`.

No controller offsets, saves, weapon acquisition, native ammunition, other weapon audio, HUD health suppression or user NIS/OLED settings are changed. Existing Kasimov sound-header correction remains preserved. Legacy health display stays active.

## One physical acceptance session

After a fresh start signal: raise and rotate the left wrist, checking panel readability and glove/weapon/magazine overlap in both eyes; perform one Centurion shoulder acquisition, carry and insertion, then fire several rounds. Stop if a crash or rendering regression appears. Check the bounded GPU receipt and native reload acknowledgement after normal exit. Do not mark production HUD, cylinder visibility or crash-free firing PASS before this result. No automatic launch, timeout or baseline promotion.
