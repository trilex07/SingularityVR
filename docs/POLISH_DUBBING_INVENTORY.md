# Polish dubbing inventory

Generated from the installed Singularity package inventory on 2026-10-08. This is a planning manifest; no game files were changed.

- Audio-related package files indexed: **785**
- Single-player-looking packages (`SP_`): **722**
- Multiplayer-looking packages (`MP_`): **47**

## Package classes

| Class | Files | Total size | Dubbing use |
|---|---:|---:|---|
| localized package | 676 | 795.7 MiB | highest priority: localized dialogue and scene-specific voice candidates |
| streamed audio (XSA) | 53 | 123.9 MiB | inspect for streamed dialogue; may also contain music/effects |
| music | 6 | 0.2 MiB | exclude from dubbing; preserve original music |
| audio scene/container | 50 | 11.7 MiB | inspect with package/export listing before recording |
| other audio-related | 0 | 0.0 MiB | inspect with package/export listing before recording |

## Localized language packages found

- `INT`: 169 files
- `FRA`: 169 files
- `ITA`: 169 files
- `ESN`: 169 files
- `DEU`: 0 files
- `RUS`: 0 files
- `POL`: 0 files

No `POL` voice package is present in the installed game. The Polish target therefore needs a new external working set and, later, either a safe UE3 package replacement or a runtime audio redirect.

## Production order

1. Extract/list exports from `*_LOC_INT.xxx` packages and identify SoundCue/Wwise/stream identifiers.
2. Match each voice identifier to the existing English text draft and scene.
3. Create Polish scripts with timing notes and actor/voice assignment.
4. Record or synthesize dry Polish takes, then edit breaths, loudness and duration.
5. Encode to the exact source codec/sample rate after the asset format is confirmed.
6. Test an external runtime redirect on a tiny sample set before any package repack.

Cutscene video audio must be checked separately: dialogue embedded in Bink/video cannot be replaced by a normal localized sound package.
