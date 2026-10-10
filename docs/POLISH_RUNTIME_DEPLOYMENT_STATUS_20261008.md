# Polish text and dubbing deployment status — 2026-10-08

The Polish working sets are complete enough to package, but they are not yet a
safe runtime replacement for the installed game.

## Ready artifacts

- Text draft: `C:\Users\Trilex\vr-mods\SingularityVR-localization-local\polish\full_draft.jsonl`
  (7,056 records; the TOP-300 review list remains reserved for the user's review).
- UI voice working set: `C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\full_voicepack\`
  (894 manifest entries).
- Story voice working set: `C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack\`
  (992 manifest entries).
- NLLB comparison set: `...\audio\polish\story_voicepack_nllb\`.
- QA report: `C:\Users\Trilex\vr-mods\SingularityVR-cutscene\research\localization\polish_dubbing_qa_final.json`.

## Why it is not copied into Steam

Singularity reads text from `Coalesced_INT.bin` and `*_LOC_INT.xxx` packages;
it does not load loose `.int` files. The installed game has no Polish package,
and the current dubbing report marks 1,573 of 1,886 generated lines for manual
listening review. The generated WAVs are therefore a reviewable working set,
not a finished release. `Coalesced_INT.bin`, game packages, and
`Singularity.exe` remain untouched.

Activating Polish text/audio requires either a tested UE3 package replacement or
a runtime redirect hook. Neither is present in the production DLL, so copying
the working sets into the Steam folder would have no effect and would obscure
the tested rollback state.

## Next safe integration step

Finish the listening queue, then test a tiny runtime redirect with one menu
string and one short voice line. Only after that sample survives startup,
pause/menu transitions, and rollback should the full Polish pack be enabled.
